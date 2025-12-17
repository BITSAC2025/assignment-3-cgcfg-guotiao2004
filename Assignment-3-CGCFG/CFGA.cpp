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
        for (auto snk : sinks)
        {
            // TODO: DFS the graph, starting from src and detecting the paths ending at snk.
            // Use the class method 'recordPath' (already defined) to record the path you detected.
            //@{
            std::vector<unsigned> trace;
            std::vector<unsigned> contextStack;
            std::set<unsigned> activeNodes;

            std::function<void(unsigned)> traverse = [&](unsigned u) {
                // Cycle detection
                if (activeNodes.find(u) != activeNodes.end()) {
                    return;
                }

                activeNodes.insert(u);
                trace.push_back(u);

                if (u == snk) {
                    this->recordPath(trace);
                } 
                else {
                    const ICFGNode* node = icfg->getICFGNode(u);
                    for (const auto* edge : node->getOutEdges()) {
                        unsigned v = edge->getDstNode()->getId();

                        if (edge->isCallCFGEdge()) {
                            contextStack.push_back(u);
                            traverse(v);
                            contextStack.pop_back();
                        }
                        else if (edge->isRetCFGEdge()) {
                            if (!contextStack.empty()) {
                                unsigned caller = contextStack.back();
                                contextStack.pop_back();
                                traverse(v);
                                contextStack.push_back(caller);
                            } else {
                                traverse(v);
                            }
                        }
                        else {
                            // Handle IntraCFGEdge implicitly here
                            traverse(v);
                        }
                    }
                }

                // Backtracking
                trace.pop_back();
                activeNodes.erase(u);
            };

            if (src) {
                traverse(src);
            }
            //@}
        }
}
