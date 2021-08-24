/** This adapts the queued out node category to a TBB multifunction node.
 */

#ifndef WIRECELLTBB_QUEUEDOUT
#define WIRECELLTBB_QUEUEDOUT

#include "WireCellTbb/NodeWrapper.h"
#include "WireCellIface/IQueuedoutNode.h"

#include <iostream>             // debug

namespace WireCellTbb {

    using queuedout_node = tbb::flow::multifunction_node<boost::any, any_single>;
    using queuedout_port = queuedout_node::output_ports_type;


    class QueuedoutBody {
        WireCell::IQueuedoutNodeBase::pointer m_wcnode;

       public:
        ~QueuedoutBody() {}
        QueuedoutBody(WireCell::INode::pointer wcnode)
        {
            m_wcnode = std::dynamic_pointer_cast<WireCell::IQueuedoutNodeBase>(wcnode);
        }
        void operator()(const boost::any& in, queuedout_port& out)
        {
            WireCell::IQueuedoutNodeBase::queuedany outq;
            bool ok = (*m_wcnode)(in, outq);
            if (!ok) {
                std::cerr << "TbbFlow: queued node return false ignored\n";
                return;
            }  // fixme: do something better here!
            for (auto a : outq) {
                // does not block.
                bool accepted = std::get<0>(out).try_put(a);
                if (!accepted) {
                    std::cerr << "UNACCEPTED try_put " << m_wcnode->signature() << std::endl;
                }
            }
        }
    };
    class QueuedoutWrapper : public NodeWrapper {
       public:
        QueuedoutWrapper(tbb::flow::graph& graph, WireCell::INode::pointer wcnode)
          : m_tbbnode(new queuedout_node(graph, wcnode->concurrency(), QueuedoutBody(wcnode)))
        {
        }
        virtual ~QueuedoutWrapper()
        {
            delete m_tbbnode;
            m_tbbnode = nullptr;
        }

        virtual receiver_port_vector receiver_ports()
        {
            auto ptr = dynamic_cast<receiver_type*>(m_tbbnode);
            return receiver_port_vector{ptr};
        }

        virtual sender_port_vector sender_ports()
        {
            auto ptr = &tbb::flow::output_port<0>(*m_tbbnode);
            return sender_port_vector{ptr};
        }

       private:
        queuedout_node* m_tbbnode;
    };

}  // namespace WireCellTbb
#endif
