/**
 * ICFG.cpp
 * @author kisslune 
 */

#include "CFGA.h"

using namespace SVF;
using namespace llvm;
using namespace std;

int main(int argc, char **argv)
{
    auto moduleNameVec =
            OptionBase::parseOptions(argc, argv, "Whole Program Points-to Analysis",
                                     "[options] <input-bitcode...>");

    LLVMModuleSet::buildSVFModule(moduleNameVec);

    SVFIRBuilder builder;
    auto pag = builder.build();
    auto icfg = pag->getICFG();

    CFGAnalysis analyzer = CFGAnalysis(icfg);

    // TODO: complete the following method: 'CFGAnalysis::analyze'
    analyzer.analyze(icfg);

    analyzer.dumpPaths();
    LLVMModuleSet::releaseLLVMModuleSet();
    return 0;
}


void CFGAnalysis::analyze(SVF::ICFG *icfg)
{
    // Sources and sinks are specified when an analyzer is instantiated.
    for (auto src : sources)
    {
        for (auto snk : sinks)
        {
            // TODO: DFS the graph, starting from src and detecting the paths ending at snk.
            // Use the class method 'recordPath' (already defined) to record the path you detected.
            //@{
            
            // Core data structures for DFS state
            std::vector<unsigned> currentPath;
            std::vector<unsigned> callHistory;
            std::set<unsigned> recursionGuard;

            // DFS Helper Lambda
            std::function<void(unsigned)> explore = [&](unsigned currID) {
                
                // Avoid infinite loops by checking if node is already on the current stack
                if (recursionGuard.find(currID) != recursionGuard.end()) {
                    return;
                }

                // Register state
                recursionGuard.insert(currID);
                currentPath.push_back(currID);

                // Goal check
                if (currID == snk) {
                    this->recordPath(currentPath);
                } 
                else {
                    // Expand neighbors
                    auto* currNode = icfg->getICFGNode(currID);
                    for (auto* edge : currNode->getOutEdges()) {
                        unsigned nextID = edge->getDstNode()->getId();

                        if (edge->isCallCFGEdge()) {
                            // Logic: Enter function, push context
                            callHistory.push_back(currID);
                            explore(nextID);
                            callHistory.pop_back();
                        } 
                        else if (edge->isRetCFGEdge()) {
                            // Logic: Exit function, handle context matching
                            if (callHistory.empty()) {
                                // No context to match (entry point return), just proceed
                                explore(nextID);
                            } 
                            else {
                                // Validate Return Site against Call Site
                                unsigned pendingCallID = callHistory.back();
                                auto* callNode = SVFUtil::dyn_cast<CallICFGNode>(icfg->getICFGNode(pendingCallID));
                                
                                // CRITICAL: Ensure we are returning to the correct call site
                                if (callNode && callNode->getRetICFGNode()->getId() == nextID) {
                                    callHistory.pop_back();   // Consume context
                                    explore(nextID);          // Continue path
                                    callHistory.push_back(pendingCallID); // Restore context (backtrack)
                                }
                                // If mismatch, we simply ignore this edge (prune path)
                            }
                        } 
                        else {
                            // Logic: Intra-procedural edge
                            explore(nextID);
                        }
                    }
                }

                // Clean up state (Backtracking)
                currentPath.pop_back();
                recursionGuard.erase(currID);
            };

            // Start search if source is valid
            if (src) {
                explore(src);
            }
            //@}
        }
    }
}
