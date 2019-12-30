// This is a main entry point for configuring a wire-cell CLI job to
// simulate ICARUS.  It is simplest signal-only simulation with
// one set of nominal field response function. 

local g = import 'pgraph.jsonnet';
local f = import 'pgrapher/common/funcs.jsonnet';
local wc = import 'wirecell.jsonnet';

local io = import 'pgrapher/common/fileio.jsonnet';
local tools_maker = import 'pgrapher/common/tools.jsonnet';
local params = import 'pgrapher/experiment/icarus/simparams.jsonnet';

local tools = tools_maker(params);

local sim_maker = import 'pgrapher/experiment/icarus/sim.jsonnet';
local sim = sim_maker(params, tools);

local stubby = {
  tail: wc.point(1000.0, 3000.0, 1000.0, wc.mm),
  head: wc.point(1100.0, 3000.0, 2000.0, wc.mm),
};

local tracklist = [

  {
    time: 0 * wc.us, 
    charge: -2500, // 5000 e/mm
    ray: stubby,
  },

];

local depos = sim.tracks(tracklist, step=0.5 * wc.mm);

// local output = 'wct-sim-ideal-sig.npz';
// local deposio = io.numpy.depos(output);
local drifter = sim.drifter;
local bagger = sim.make_bagger();
// signal plus noise pipelines
local sn_pipes = sim.splusn_pipelines;

local multimagnify = import 'pgrapher/experiment/icarus/multimagnify.jsonnet';
local magoutput = 'icarus-sim-check.root';

local multi_magnify = multimagnify('orig', tools, magoutput);
local magnify_pipes = multi_magnify.magnify_pipelines;
local multi_magnify2 = multimagnify('raw', tools, magoutput);
local magnify_pipes2 = multi_magnify2.magnify_pipelines;
local multi_magnify3 = multimagnify('gauss', tools, magoutput);
local magnify_pipes3 = multi_magnify3.magnify_pipelines;
local multi_magnify4 = multimagnify('wiener', tools, magoutput);
local magnify_pipes4 = multi_magnify4.magnify_pipelines;
local multi_magnify5 = multimagnify('threshold', tools, magoutput);
local magnify_pipes5 = multi_magnify5.magnifysummaries_pipelines;

local perfect = import 'pgrapher/experiment/icarus/chndb-base.jsonnet';
local chndb = [{
  type: 'OmniChannelNoiseDB',
  name: 'ocndbperfect%d' % n,
  data: perfect(params, tools.anodes[n], tools.field, n),
  uses: [tools.anodes[n], tools.field],  // pnode extension
} for n in std.range(0, std.length(tools.anodes) - 1)];

// local nf_maker = import 'pgrapher/experiment/pdsp/nf.jsonnet';
// local nf_pipes = [nf_maker(params, tools.anodes[n], chndb[n], n, name='nf%d' % n) for n in std.range(0, std.length(tools.anodes) - 1)];

local sp_maker = import 'pgrapher/experiment/icarus/sp.jsonnet';
local sp = sp_maker(params, tools);
local sp_pipes = [sp.make_sigproc(a) for a in tools.anodes];

local main_pipelines = [
  g.pipeline([
               sn_pipes[n],
               magnify_pipes[n],
               // nf_pipes[n],
               // magnify_pipes2[n],
               sp_pipes[n],
               magnify_pipes3[n],
               // magnify_pipes4[n],
               magnify_pipes5[n],
             ],
             'parallel_pipe_%d' % n)
  for n in std.range(0, std.length(tools.anodes) - 1)
];
local outtags = ['raw%d' % n for n in std.range(0, std.length(tools.anodes) - 1)];
local manifold = f.fanpipe('DepoSetFanout', main_pipelines, 'FrameFanin', 'sn_mag_nf', outtags);


//local frameio = io.numpy.frames(output);
local sink = sim.frame_sink;

local graph = g.pipeline([depos, drifter, bagger, manifold, sink]);

local app = {
  type: 'Pgrapher',
  data: {
    edges: g.edges(graph),
  },
};

local cmdline = {
    type: "wire-cell",
    data: {
        plugins: ["WireCellGen", "WireCellPgraph", "WireCellSio", "WireCellSigProc", "WireCellRoot"],
        apps: ["Pgrapher"]
    }
};


// Finally, the configuration sequence which is emitted.

[cmdline] + g.uses(graph) + [app]
