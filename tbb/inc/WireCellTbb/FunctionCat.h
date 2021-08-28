#ifndef WIRECELLTBB_FUNCTIONCAT
#define WIRECELLTBB_FUNCTIONCAT

#include "WireCellIface/IFunctionNode.h"
#include "WireCellTbb/NodeWrapper.h"

#include <iostream>  // temporary, don't ignore the error code, chump!

namespace WireCellTbb {

    // Body for a TBB function node.
    class FunctionBody {
        WireCell::IFunctionNodeBase::pointer m_wcnode;

        mutable seqno_t m_seqno{0};
        NodeMonitor m_nm;

      public:
        FunctionBody(WireCell::INode::pointer wcnode, NodeMonitor nm)
            : m_nm(nm)
        {
            m_wcnode = std::dynamic_pointer_cast<WireCell::IFunctionNodeBase>(wcnode);
        }
        virtual ~FunctionBody() {}

        msg_t operator()(const msg_t& in) const
        {
            m_nm(NodeState::enter);
            wct_t out;
            bool ok = (*m_wcnode)(in.second, out);
            if (!ok) {
                m_nm(NodeState::error);
                std::cerr << "TbbFlow: function node return false ignored\n";
            }
            auto msg = msg_t(m_seqno++, out);
            m_nm(NodeState::exit);
            return msg;
        }
    };

    // Wrap the TBB (compound) node
    class FunctionWrapper : public NodeWrapper {
        tbb::flow::graph_node *m_fn, *m_sn;

      public:

        FunctionWrapper(tbb::flow::graph& graph,
                        WireCell::INode::pointer wcnode,
                        NodeMonitor nm)
        {
            auto fn = new func_node(graph, 1, FunctionBody(wcnode, nm));
            auto sn = new seq_node(graph, [](const msg_t& m) {return m.first;});
            tbb::flow::make_edge(*fn, *sn);
            m_fn = fn;
            m_sn = sn;
        }

        virtual ~FunctionWrapper()
        {
            delete m_fn;
            delete m_sn;
        }

        virtual receiver_port_vector receiver_ports()
        {
            return {dynamic_cast<receiver_type*>(m_fn)};
        }

        virtual sender_port_vector sender_ports() 
        { 
            return {dynamic_cast<sender_type*>(m_sn)}; 
        }
    };

}  // namespace WireCellTbb

#endif
