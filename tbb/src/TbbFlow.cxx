#include "WireCellTbb/TbbFlow.h"

#include "WireCellUtil/NamedFactory.h"
#include "WireCellUtil/String.h"
#include "WireCellUtil/Persist.h"

#include <string>

<<<<<<< HEAD
WIRECELL_FACTORY(TbbFlow, WireCellTbb::TbbFlow, WireCell::IApplication, WireCell::IConfigurable)
=======
WIRECELL_FACTORY(TbbFlow, WireCellTbb::TbbFlow,
                 WireCell::IApplication, WireCell::IConfigurable)
>>>>>>> 4e01cf770602cf83886437e4773b2c722643c5d0

using namespace WireCell;
using namespace WireCellTbb;

TbbFlow::TbbFlow()
: l(Log::logger("tbb"))
{
}

TbbFlow::~TbbFlow()
{
}

Configuration TbbFlow::default_configuration() const
{
    Configuration cfg;

    cfg["edges"] = Json::arrayValue;
    return cfg;
}

void TbbFlow::configure(const Configuration& cfg)
{
    std::string type, name, desc = get<std::string>(cfg, "dfp","TbbDataFlowGraph");
    std::tie(type,name) = String::parse_pair(desc);
    m_dfp = Factory::lookup<IDataFlowGraph>(type, name);

    m_dfpgraph.configure(cfg["edges"]);
    
}

void TbbFlow::execute()
{
    if (!m_dfp) {
    l->critical("TbbFlow: not configured");
	return;
    }

    l->info("TbbFlow::Execute");

    for (auto thc : m_dfpgraph.connections()) {
	auto tail_tn = get<0>(thc);
	auto head_tn = get<1>(thc);
	auto conn = get<2>(thc);

    l->debug("TbbFlow: Connect: {}:{} ( {} -> {} ) {}:{}",
    tail_tn.type, tail_tn.name,
    conn.tail, conn.head,
    head_tn.type, head_tn.name
    );

	INode::pointer tail_node = WireCell::Factory::lookup<INode>(tail_tn.type, tail_tn.name);
	INode::pointer head_node = WireCell::Factory::lookup<INode>(head_tn.type, head_tn.name);

	m_dfp->connect(tail_node, head_node, conn.tail, conn.head);
    }


    l->info("TbbFlow: run: ");
    m_dfp->run();
}

