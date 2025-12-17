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
            
            // DFS 状态的核心数据结构
            std::vector<unsigned> currentPath;
            std::vector<unsigned> callHistory;
            std::set<unsigned> recursionGuard;

            // DFS 辅助 Lambda 函数
            std::function<void(unsigned)> explore = [&](unsigned currID) {
                
                // 检查节点是否已在当前路径中，避免无限循环
                if (recursionGuard.find(currID) != recursionGuard.end()) {
                    return;
                }

                // 记录当前状态
                recursionGuard.insert(currID);
                currentPath.push_back(currID);

                // 终点检查
                if (currID == snk) {
                    this->recordPath(currentPath);
                } 
                else {
                    // 扩展邻居节点
                    auto* currNode = icfg->getICFGNode(currID);
                    for (auto* edge : currNode->getOutEdges()) {
                        unsigned nextID = edge->getDstNode()->getId();

                        if (edge->isCallCFGEdge()) {
                            // 逻辑：进入函数，压入上下文
                            callHistory.push_back(currID);
                            explore(nextID);
                            callHistory.pop_back();
                        } 
                        else if (edge->isRetCFGEdge()) {
                            // 逻辑：退出函数，处理上下文匹配
                            if (callHistory.empty()) {
                                // 无上下文匹配（可能是从入口点返回），直接继续
                                explore(nextID);
                            } 
                            else {
                                // 验证返回点是否匹配调用点
                                unsigned pendingCallID = callHistory.back();
                                auto* callNode = SVFUtil::dyn_cast<CallICFGNode>(icfg->getICFGNode(pendingCallID));
                                
                                // 关键：确保返回到正确的调用点
                                if (callNode && callNode->getRetICFGNode()->getId() == nextID) {
                                    callHistory.pop_back();   // 消耗上下文
                                    explore(nextID);          // 继续路径
                                    callHistory.push_back(pendingCallID); // 恢复上下文（回溯）
                                }
                                // 如果不匹配，直接忽略此边（剪枝）
                            }
                        } 
                        else {
                            // 逻辑：过程内边
                            explore(nextID);
                        }
                    }
                }

                // 清理状态（回溯）
                currentPath.pop_back();
                recursionGuard.erase(currID);
            };

            // 如果源节点有效，开始搜索
            if (src) {
                explore(src);
            }
            //@}
        }
    }
}
