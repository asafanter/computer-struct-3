/* 046267 Computer Architecture - Spring 2019 - HW #3 */
/* Implementation (skeleton)  for the dataflow statistics calculator */

#include "dflow_calc.h"

#include <vector>
#include <algorithm>

using uint = unsigned int;

class Dependency
{
public:

    enum class Type
    {
        RAW,
        WAR,
        WAW
    };

    enum class Source
    {
        SOURCE1,
        SOURCE2,
        DEST
    };

    bool operator==(const Dependency &dep) const
    {
        return type == dep.type && reg == dep.reg && source == dep.source && dependent == dep.dependent;
    }

    bool isReal() const
    {
        return type == Type::RAW;
    }

    Type type;
    uint reg;
    uint source;
    uint dependent;
    uint max_depth;
    uint son_opcode;
    Source source_dependency;
};

class Node
{
public:

    Node() :
        _id(0),
        _opcode(0),
        _dst(0),
        _addr1(0),
        _addr2(0),
        _dependencies(),
        _max_depth(0),
        _is_exit(true)
    {

    }

    Node(const uint &id, const uint &opcode, const uint &dst, const uint &addr1, const uint &addr2) :
        _id(id),
        _opcode(opcode),
        _dst(dst),
        _addr1(addr1),
        _addr2(addr2),
        _dependencies(),
        _max_depth(0),
        _is_exit(true)
    {

    }

    std::vector<Dependency> checkDependenciesWith(const Node &node) const
    {
        std::vector<Dependency> deps;

        if(_dst == node._dst)
        {
            deps.push_back({Dependency::Type::WAW, _dst, _id, node._id,
                            node.getMaxDepth(), node.getOpcode(), Dependency::Source::DEST});
        }

        if(_dst == node._addr1)
        {
            deps.push_back({Dependency::Type::WAR, _dst, _id, node._id,
                            node.getMaxDepth(), node.getOpcode(), Dependency::Source::SOURCE1});
        }

        if(_dst == node._addr2)
        {
            deps.push_back({Dependency::Type::WAR, _dst, _id, node._id,
                            node.getMaxDepth(), node.getOpcode(), Dependency::Source::SOURCE2});
        }

        if(_addr1 == node._dst)
        {
            deps.push_back({Dependency::Type::RAW, node._dst, _id, node._id,
                            node.getMaxDepth(), node.getOpcode(), Dependency::Source::SOURCE1});
        }

        if(_addr2 == node._dst)
        {
            deps.push_back({Dependency::Type::RAW, node._dst, _id, node._id,
                            node.getMaxDepth(), node.getOpcode(), Dependency::Source::SOURCE2});
        }

        return deps;
    }

    Node &addDependency(const Dependency &dep)
    {
        _dependencies.push_back(dep);

        return *this;
    }

    bool hasDependencies() const
    {
        return _dependencies.size() > 0;
    }

    Node &updateMaxDepth(std::vector<uint> &durations)
    {
        if(_dependencies.size() == 0)
        {
            return *this;
        }
        else if(_dependencies.size() == 1)
        {
            auto dep = _dependencies[0];
            _max_depth = dep.max_depth + durations[dep.son_opcode];
            return *this;
        }

        auto dep1 = _dependencies[0];
        auto dep2 = _dependencies[1];
        auto max_depth1 = dep1.max_depth + durations[dep1.son_opcode];
        auto max_depth2 = dep2.max_depth + durations[dep2.son_opcode];

        _max_depth = std::max(max_depth1, max_depth2);

        return *this;
    }

    bool containsRealDependency(const Dependency& dep)
    {
        for(auto &dependency : _dependencies)
        {
            if(dependency.reg == dep.reg && dependency.source_dependency == dep.source_dependency)
            {
                return true;
            }
        }

        return false;
    }

    uint getId() const {return _id;}
    uint getMaxDepth() const {return _max_depth;}
    Node &setMaxDepth(const uint &new_max_depth) {_max_depth = new_max_depth; return *this;}
    uint getOpcode() const {return _opcode;}
    Node &setIsExit(const bool &is_exit) {_is_exit = is_exit; return *this;}
    bool isExit() const {return _is_exit;}
    std::vector<Dependency> getDependencies() {return _dependencies;}
    uint getAddr1() const {return _addr1;}
    uint getAddr2() const {return _addr2;}


private:
    uint _id;
    uint _opcode;
    uint _dst;
    uint _addr1;
    uint _addr2;
    std::vector<Dependency> _dependencies;
    uint _max_depth;
    bool _is_exit;
};

class Dflow
{
public:
    Dflow() :
        _nodes(),
        _durations(),
        _false_dependencies(32)
    {

    }

    void init(const std::vector<uint> &durations, const std::vector<Node> &nodes)
    {
        _durations = durations;

        for(auto &node : nodes)
        {
            addNode(node);
        }
    }

    int getInstDepth(const uint &id)
    {
        if(id >= _nodes.size())
        {
            return -1;
        }

        return static_cast<int>(_nodes[id].getMaxDepth());
    }

    uint getProgDepth()
    {
        std::vector<uint> exists_nodes_id;

        for(auto &node : _nodes)
        {
            if(node.isExit())
            {
                exists_nodes_id.push_back(node.getId());
            }
        }

        std::vector<int> max_depths;
        for(auto &id : exists_nodes_id)
        {
            max_depths.push_back(getInstDepth(id) + static_cast<int>(_durations[_nodes[id].getOpcode()]));
        }

        auto res = *std::max_element(max_depths.begin(), max_depths.end());

        return static_cast<uint>(res);
    }

    uint getNumFalseDependenciesOfReg(const uint &reg)
    {
        return _false_dependencies[reg];
    }

    int getInstDeps(const uint &id, int &src1DepInst, int &src2DepInst)
    {
        if(id >= _nodes.size())
        {
            return -1;
        }

        auto deps = _nodes[id].getDependencies();
        uint addr1 = _nodes[id].getAddr1();
        uint addr2 = _nodes[id].getAddr2();

        src1DepInst = -1;
        src2DepInst = -1;

        for(auto &dep : deps)
        {
            if(dep.reg == addr1)
            {
                src1DepInst = static_cast<int>(dep.dependent);
            }
            if(dep.reg == addr2)
            {
                src2DepInst = static_cast<int>(dep.dependent);
            }
        }


        return 0;
    }

private: //methods

    Dflow &addNode(const Node &node)
    {
        Node tmp_node = node;

        for(uint i = tmp_node.getId() - 1; static_cast<int>(i) >= 0; i--)
        {
            auto deps = tmp_node.checkDependenciesWith(_nodes[i]);
            bool is_already_add_false_dep = false;

            for(auto &dep : deps)
            {
                if(dep.isReal() && !tmp_node.containsRealDependency(dep))
                {
                    tmp_node.addDependency(dep);
                    _nodes[dep.dependent].setIsExit(false);
                }

                else if(tmp_node.getId() - _nodes[i].getId() == 1 && !dep.isReal() && !is_already_add_false_dep)
                {
                    _false_dependencies[dep.reg]++;
                    is_already_add_false_dep = true;
                }
            }
        }

        tmp_node.updateMaxDepth(_durations);
        _nodes.push_back(tmp_node);

        return *this;
    }


private: //members
    std::vector<Node> _nodes;
    std::vector<uint> _durations;
    std::vector<uint> _false_dependencies;
};





ProgCtx analyzeProg(const unsigned int opsLatency[], InstInfo progTrace[], unsigned int numOfInsts) {

    std::vector<uint> durations(MAX_OPS);
    std::vector<Node> nodes;

    for(uint i = 0;i < MAX_OPS; i++)
    {
        durations[i] = opsLatency[i];
    }

    for(uint i = 0; i < numOfInsts; i++)
    {
        Node node(i, progTrace[i].opcode, static_cast<uint>(progTrace[i].dstIdx),
                  progTrace[i].src1Idx, progTrace[i].src2Idx);
        nodes.push_back(node);
    }

    Dflow *flow = new Dflow;
    flow->init(durations, nodes);

    return static_cast<ProgCtx>(flow);
}

void freeProgCtx(ProgCtx ctx) {

    delete static_cast<Dflow*>(ctx);
}

int getInstDepth(ProgCtx ctx, unsigned int theInst) {

    return static_cast<Dflow*>(ctx)->getInstDepth(theInst);
}

int getInstDeps(ProgCtx ctx, unsigned int theInst, int *src1DepInst, int *src2DepInst) {

    return static_cast<Dflow*>(ctx)->getInstDeps(theInst, *src1DepInst, *src2DepInst);
}

int getRegfalseDeps(ProgCtx ctx, unsigned int reg) {

    return static_cast<int>(static_cast<Dflow*>(ctx)->getNumFalseDependenciesOfReg(reg));
}

int getProgDepth(ProgCtx ctx) {

    return static_cast<int>(static_cast<Dflow*>(ctx)->getProgDepth());
}


