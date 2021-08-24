#ifndef WIRECELLTBB_FANOUTCAT
#define WIRECELLTBB_FANOUTCAT

#include "WireCellIface/IFanoutNode.h"
#include "WireCellUtil/TupleHelpers.h"
#include "WireCellUtil/Testing.h"
#include "WireCellTbb/NodeWrapper.h"

#include <iostream>

namespace WireCellTbb {

    using seq_any_t = std::pair<size_t, boost::any>;

    // Body for a TBB split node.
    template <typename std::size_t N>
    class FanoutBody {
        WireCell::IFanoutNodeBase::pointer m_wcnode;
        mutable size_t m_seqno{0}; // FIXME WARNING: mutable

       public:
        typedef typename WireCell::IFanoutNodeBase::any_vector any_vector;
        typedef typename WireCell::type_repeater<N, seq_any_t>::type TupleType;
        // typedef typename WireCell::tuple_helper<TupleType> helper_type;

        FanoutBody(WireCell::INode::pointer wcnode)
        {
            m_wcnode = std::dynamic_pointer_cast<WireCell::IFanoutNodeBase>(wcnode);
            Assert(m_wcnode);
        }

        TupleType operator()(boost::any in) const
        {
            any_vector anyvec;
            // bool ok = (*m_wcnode)(in, ret);
            (*m_wcnode)(in, anyvec);  // fixme: don't ignore return code
            // return ih.from_any(vectorToTuple<N>(ret));
            std::vector<seq_any_t> savec;
            for (auto& a : anyvec) {
                savec.push_back(seq_any_t(m_seqno, a));
            }
            ++m_seqno;
            return WireCell::vectorToTuple<N>(savec);
        }
    };

    typedef tbb::flow::sender<seq_any_t> seq_sender_type;
    typedef std::vector<seq_sender_type*> seq_sender_port_vector;
    
    typedef tbb::flow::receiver<seq_any_t> seq_receiver_type;

    template <typename Tuple, std::size_t... Is>
    seq_sender_port_vector seq_sender_ports(tbb::flow::split_node<Tuple>& sp, std::index_sequence<Is...>)
    {
        return {dynamic_cast<seq_sender_type*>(&tbb::flow::output_port<Is>(sp))...};
    }
    /// Return sender ports of a split node as a vector.
    template <typename Tuple>
    seq_sender_port_vector seq_sender_ports(tbb::flow::split_node<Tuple>& sp)
    {
        return seq_sender_ports(sp, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
    }


    template <std::size_t N>
    sender_port_vector build_fanouter(tbb::flow::graph& graph, WireCell::INode::pointer wcnode,
                                      std::vector<tbb::flow::graph_node*>& nodes)
    {
        typedef typename WireCell::type_repeater<N, seq_any_t>::type TupleType;

        // this node takes user WC body and runs it after converting input verctor to tuple
        auto* fn = new tbb::flow::function_node<boost::any, TupleType>(graph, 1 /*wcnode->concurrency()*/, FanoutBody<N>(wcnode));
        // Below requires first to be the WCT body caller
        nodes.push_back(fn);

        // this node is fully TBB and splits N sender ports into a tuple
        auto* sp = new tbb::flow::split_node<TupleType>(graph);
        nodes.push_back(sp);

        tbb::flow::make_edge(*fn, *sp);

        sender_port_vector outports;

        // The split_node can introduce out-of-order as it has
        // unlimited concurency.  After each sender_node output port
        // we add a sequencer based on the seqno added by the caller
        // followed by a function to strip the seqno and pass on the
        // any from the original function.  Whew.
        for (seq_sender_type* one : seq_sender_ports<TupleType>(*sp)) {

            auto* seq = new tbb::flow::sequencer_node<seq_any_t>(graph, [](const seq_any_t& sa) { return sa.first; });
            nodes.push_back(seq);
            tbb::flow::make_edge(*one, *dynamic_cast<seq_receiver_type*>(seq));

            auto* strip = new tbb::flow::function_node<seq_any_t, boost::any>(graph, 1, [](const seq_any_t& sa) { return sa.second; });
            nodes.push_back(strip);
            tbb::flow::make_edge(*dynamic_cast<seq_sender_type*>(seq), *dynamic_cast<seq_receiver_type*>(strip));
            
            outports.push_back(dynamic_cast<sender_type*>(strip));
        }
        return outports;
    }

    // Wrap the TBB (compound) node
    class FanoutWrapper : public NodeWrapper {
        std::vector<tbb::flow::graph_node*> m_nodes;
        sender_port_vector m_sender_ports;

       public:
        FanoutWrapper(tbb::flow::graph& graph, WireCell::INode::pointer wcnode)
        {
            int nout = wcnode->output_types().size();
            // an exhaustive switch to convert from run-time to compile-time types and enumerations.
            Assert(nout > 0 && nout <= 10);  // fixme: exception instead?
            if (1 == nout) m_sender_ports = build_fanouter<1>(graph, wcnode, m_nodes);
            if (2 == nout) m_sender_ports = build_fanouter<2>(graph, wcnode, m_nodes);
            if (3 == nout) m_sender_ports = build_fanouter<3>(graph, wcnode, m_nodes);
            if (4 == nout) m_sender_ports = build_fanouter<4>(graph, wcnode, m_nodes);
            if (5 == nout) m_sender_ports = build_fanouter<5>(graph, wcnode, m_nodes);
            if (6 == nout) m_sender_ports = build_fanouter<6>(graph, wcnode, m_nodes);
            if (7 == nout) m_sender_ports = build_fanouter<7>(graph, wcnode, m_nodes);
            if (8 == nout) m_sender_ports = build_fanouter<8>(graph, wcnode, m_nodes);
            if (9 == nout) m_sender_ports = build_fanouter<9>(graph, wcnode, m_nodes);
            if (10 == nout) m_sender_ports = build_fanouter<10>(graph, wcnode, m_nodes);
        }

        virtual sender_port_vector sender_ports()
        {
            return m_sender_ports;
        }

        virtual receiver_port_vector receiver_ports()
        {
            auto ptr = dynamic_cast<receiver_type*>(m_nodes[0]);
            return receiver_port_vector{ptr};
        }
    };

}  // namespace WireCellTbb

#endif
