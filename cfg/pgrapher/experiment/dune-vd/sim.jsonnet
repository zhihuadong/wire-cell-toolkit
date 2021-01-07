local wc = import "wirecell.jsonnet";
local g = import "pgraph.jsonnet";
local f = import "pgrapher/common/funcs.jsonnet";
local sim_maker = import "pgrapher/common/sim/nodes.jsonnet";


// return some nodes, includes base sim nodes.
function(params, tools) {
    local sim = sim_maker(params, tools),

    local nanodes = std.length(tools.anodes),


    local ductors = [sim.make_depotransform("ductor%d"%n, tools.anodes[n], tools.pirs[0]) for n in std.range(0, nanodes-1)],

    local reframers = [
        g.pnode({
            type: 'Reframer',
            name: 'reframer%d'%n,
            data: {
                anode: wc.tn(tools.anodes[n]),
                tags: [],           // ?? what do?
                fill: 0.0,
                tbin: params.sim.reframer.tbin,
                toffset: 0,
                nticks: params.sim.reframer.nticks,
            },
        }, nin=1, nout=1) for n in std.range(0, nanodes-1)],


    local digitizers = [
        sim.digitizer(tools.anodes[n], name="digitizer%d"%n, tag="orig%d"%n)
        for n in std.range(0,nanodes-1)],

    ret : {

        signal_pipelines: [g.pipeline([ductors[n], reframers[n],  digitizers[n]],
                                      name="simsigpipe%d"%n) for n in std.range(0, nanodes-1)],


    } + sim,      
}.ret
