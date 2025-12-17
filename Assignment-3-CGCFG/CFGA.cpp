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
    // 遍历所有的 Source 和 Sink 节点对
    for (auto src : sources)
    {
        for (auto snk : sinks)
        {
            // 每次针对一对 (src, snk) 重新初始化辅助容器
            std::vector<unsigned> path;        // 存储当前探测到的路径
            std::vector<unsigned> callStack;   // 模拟函数调用的上下文栈
            std::set<unsigned> visited;        // 防止在单次路径探测中出现死循环

            // 定义 DFS Lambda 函数
            // 使用 auto&& self 实现 Lambda 的递归调用
            auto dfs = [&](auto&& self, unsigned currId) -> void {
                
                // 1. 环路检测：如果当前节点已经在当前路径中，则停止，防止死循环
                if (visited.count(currId)) return;

                // 2. 记录状态：加入路径和已访问集合
                visited.insert(currId);
                path.push_back(currId);

                // 3. 终点检测：如果到达 sink 节点，记录这条路径
                if (currId == snk) {
                    this->recordPath(path);
                } 
                else {
                    // 获取当前 ICFG 节点
                    const ICFGNode* node = icfg->getICFGNode(currId);
                    
                    // 遍历所有出边
                    for (const auto* edge : node->getOutEdges()) {
                        unsigned nextId = edge->getDstNode()->getId();

                        // 情况 A: 过程内边 (Intra-procedural)
                        // 直接递归到下一个节点
                        if (edge->isIntraCFGEdge()) {
                            self(self, nextId);
                        } 
                        // 情况 B: 函数调用边 (Call Edge)
                        // 将当前调用点压入栈，模拟进入函数
                        else if (edge->isCallCFGEdge()) {
                            callStack.push_back(currId);
                            self(self, nextId); 
                            callStack.pop_back(); // 回溯：恢复栈状态
                        } 
                        // 情况 C: 函数返回边 (Return Edge)
                        else if (edge->isRetCFGEdge()) {
                            if (callStack.empty()) {
                                // 栈为空（可能是从入口函数返回，或者是上下文不敏感的路径），直接继续
                                self(self, nextId);
                            } else {
                                // 栈不为空，模拟从函数返回
                                // 临时弹出栈顶元素以匹配返回操作
                                unsigned callerId = callStack.back();
                                callStack.pop_back(); 
                                self(self, nextId); 
                                callStack.push_back(callerId); // 回溯：恢复栈状态
                            }
                        }
                    }
                }

                // 4. 回溯：在退出当前节点前，将其从路径和访问集合中移除
                // 这样该节点可以在其他路径中被再次访问
                path.pop_back();
                visited.erase(currId);
            };

            // 如果 src 节点有效，开始 DFS 搜索
            if (src) {
                dfs(dfs, src);
            }
        }
    }
}
