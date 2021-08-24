/** Facade/adapter to TBB for source node category. */
#ifndef WIRECELLTBB_SOURCECAT
#define WIRECELLTBB_SOURCECAT

#include "WireCellIface/ISourceNode.h"
#include "WireCellTbb/NodeWrapper.h"

namespace WireCellTbb {

    using input_node = tbb::flow::input_node<boost::any>;

    // adapter to convert from WC source node to TBB source node body.
    class SourceBody {
        WireCell::ISourceNodeBase::pointer m_wcnode;

       public:
        ~SourceBody() {}

        SourceBody(WireCell::INode::pointer wcnode)
        {
            m_wcnode = std::dynamic_pointer_cast<WireCell::ISourceNodeBase>(wcnode);
        }
        boost::any operator()(tbb::flow_control& fc) {
            boost::any out;
            if ((*m_wcnode)(out)) {
                return out;
            }
            fc.stop();
            return {};
        }
    };

    // implement facade to access ports for source nodes
    class SourceNodeWrapper : public NodeWrapper {
        input_node* m_tbbnode;

       public:
        SourceNodeWrapper(tbb::flow::graph& graph, WireCell::INode::pointer wcnode)
          : m_tbbnode(new input_node(graph, SourceBody(wcnode)))
        {
        }
        ~SourceNodeWrapper() { delete m_tbbnode; }
        virtual void initialize() { m_tbbnode->activate(); }
        virtual sender_port_vector sender_ports()
        {
            auto ptr = dynamic_cast<sender_type*>(m_tbbnode);
            return sender_port_vector{ptr};
        }
    };
}  // namespace WireCellTbb
#endif
