// This provides some util functions.

local g = import 'pgraph.jsonnet';

{
    //  Build a depofanout-[signal]-[framesummer]-[pipelines]-fanin graph.
    //  FrameSummer add up the two "split" anodes into one frame.
    //  Each branch of the pipelines operates on the summed signal frame.
    fansummer :: function(fout, sigpipes, summers, actpipes, fin, name="fansummer", outtags=[], tag_rules=[]) {

        local fanoutmult = std.length(sigpipes),
        local faninmult = std.length(actpipes),

        local fanout = g.pnode({
            type: fout,
            name: name,
            data: {
                multiplicity: fanoutmult,
                tag_rules: tag_rules,
            },
        }, nin=1, nout=fanoutmult),


        local fanin = g.pnode({
            type: fin,
            name: name,
            data: {
                multiplicity: faninmult,
                tags: outtags,
            },
        }, nin=faninmult, nout=1),

        local reducer = g.intern(innodes=sigpipes,
                                 outnodes=actpipes,
                                 centernodes=summers,
                                 edges= 
                                 // connecting signal and summer
                                 [g.edge(sigpipes[0], summers[0],0,0)]
                                 + [g.edge(sigpipes[1], summers[0],0,1)]
                                 + [g.edge(sigpipes[2], summers[1],0,0)]
                                 + [g.edge(sigpipes[3], summers[1],0,1)]
                                 + [g.edge(sigpipes[4], summers[2],0,0)]
                                 + [g.edge(sigpipes[5], summers[2],0,1)]
                                 + [g.edge(sigpipes[6], summers[3],0,0)]
                                 + [g.edge(sigpipes[7], summers[3],0,1)]
                                 // connecting summer and the operator pipelines
                                 + [g.edge(summers[n], actpipes[n]) for n in std.range(0,faninmult-1)],
                                 name=name),

        ret: g.intern(innodes=[fanout],
                      outnodes=[fanin],
                      centernodes=[reducer],
                      edges=
                      [g.edge(fanout, sigpipes[n], n, 0) for n in std.range(0, fanoutmult-1)] +
                      [g.edge(reducer, fanin, n, n) for n in std.range(0, faninmult-1)],
                      name=name),
    }.ret,

}
