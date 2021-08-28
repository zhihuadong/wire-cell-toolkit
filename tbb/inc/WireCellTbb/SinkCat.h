#ifndef WIRECELLTBB_SINKCAT
#define WIRECELLTBB_SINKCAT

#include "WireCellTbb/NodeWrapper.h"
#include "WireCellIface/ISinkNode.h"

#include <iostream> // fixme: for non-error handling

namespace WireCellTbb {

    // adapter to convert from WC sink node to TBB sink node body.
    class SinkBody {
        WireCell::ISinkNodeBase::pointer m_wcnode;

        NodeMonitor m_nm;
      public:
        ~SinkBody() {}

        SinkBody(WireCell::INode::pointer wcnode, NodeMonitor nm)
            : m_nm(nm)
        {
            m_wcnode = std::dynamic_pointer_cast<WireCell::ISinkNodeBase>(wcnode);
        }
        tbb::flow::continue_msg operator()(const msg_t& in)
        {
            m_nm(NodeState::enter);
            bool ok = (*m_wcnode)(in.second);
            if (!ok) {
                m_nm(NodeState::error);
                std::cerr << "TbbFlow: sink node return false ignored\n";
            }
            m_nm(NodeState::exit);
            return {};
        }
    };

    // implement facade to access ports for sink nodes
    class SinkNodeWrapper : public NodeWrapper {
        sink_node* m_tbbnode;

      public:
        SinkNodeWrapper(tbb::flow::graph& graph,
                        WireCell::INode::pointer wcnode,
                        NodeMonitor nm)
            : m_tbbnode(new sink_node(graph, 1, SinkBody(wcnode, nm)))
        {
        }
        virtual ~SinkNodeWrapper() {
            delete m_tbbnode;
        }
        virtual receiver_port_vector receiver_ports()
        {
            auto ptr = dynamic_cast<receiver_type*>(m_tbbnode);
            return receiver_port_vector{ptr};
        }
    };
}  // namespace WireCellTbb
#endif
