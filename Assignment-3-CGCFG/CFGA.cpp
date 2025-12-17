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
            std::set<unsigned> activeNodes; // 用于当前路径的环路检测

            std::function<void(unsigned)> traverse = [&](unsigned u) {
                // 1. Cycle detection (Path-sensitive)
                if (activeNodes.count(u)) {
                    return;
                }

                // 2. Add to current path
                activeNodes.insert(u);
                trace.push_back(u);

                // 3. Check if we hit the sink
                if (u == snk) {
                    this->recordPath(trace);
                } 
                else {
                    const ICFGNode* node = icfg->getICFGNode(u);
                    
                    for (const auto* edge : node->getOutEdges()) {
                        unsigned v = edge->getDstNode()->getId();

                        // Case A: Function Call
                        if (edge->isCallCFGEdge()) {
                            contextStack.push_back(u); // Push CallSite ID
                            traverse(v);
                            contextStack.pop_back();
                        }
                        // Case B: Function Return
                        else if (edge->isRetCFGEdge()) {
                            if (!contextStack.empty()) {
                                unsigned callID = contextStack.back();
                                
                                // --- CRITICAL FIX: Context Matching ---
                                // 检查当前这条返回边(edge)是否对应栈顶的调用(callID)
                                // 只有当 edge->dst (返回点) 等于 callID 对应的 (RetICFGNode) 时才允许通过
                                const ICFGNode* callNode = icfg->getICFGNode(callID);
                                if (const CallICFGNode* cNode = SVFUtil::dyn_cast<CallICFGNode>(callNode)) {
                                    if (cNode->getRetICFGNode()->getId() != v) {
                                        continue; // 这是一个去往其他函数的返回边，跳过
                                    }
                                }

                                contextStack.pop_back(); // 匹配成功，弹出栈
                                traverse(v);
                                contextStack.push_back(callID); // 回溯恢复
                            } else {
                                // 栈为空（可能是从入口函数返回，或者是非敏感上下文），允许通过
                                traverse(v);
                            }
                        }
                        // Case C: Intra-procedural (Normal flow)
                        else {
                            traverse(v);
                        }
                    }
                }

                // 4. Backtracking
                trace.pop_back();
                activeNodes.erase(u);
            };

            if (src) {
                traverse(src);
            }
            //@}
        }
}
}
