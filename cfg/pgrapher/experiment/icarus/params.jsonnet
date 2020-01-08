// This file specifies the paramter configuration for the ICARUS detector. It
// inherit from the base params.jsonnet and override the relevant paramters

local wc = import "wirecell.jsonnet";
local base = import "pgrapher/common/params.jsonnet";

base {

    det: {

        // Each wires gets identified with the same number that identifies it in
        // the geometry files. The horizontal induction is split in two, both
        // sides are enumerated by s, while n counts the physical anodes.
        // NOTE:the actual physical volumes in ICARUS are only 4

        local xanode = [365.33*wc.cm, 0, 0, 365.33*wc.cm],
        local offset_response = [if a%2==0 then +10*wc.cm else -10*wc.cm for a in std.range(0,3)],
        local xresponse = [xanode[a] + offset_response[a] for a in std.range(0,3)],
        local xcathode = [-149.1*wc.cm, -149.1*wc.cm, 149.1*wc.cm, 149.1*wc.cm],
        volumes : [
            {
                local world = 100,  // identify this geometry
                local split = s*10, // identify anode side (1 left, 2 right)
                local anode = a,    // physical anode number
                wires: (world+split+anode),
                name: "anode%d"%(world+split+anode),
                faces: [
                        {
                            anode: xanode[a],
                            response: xresponse[a],
                            cathode: xcathode[a],
                        },
                        null
                ],
            } for a in std.range(0,3) for s in std.range(1,2)
        ],
    },

    daq : {

        // ICARUS has a sampling frequency at 0.25 MHz
        tick: 0.4*wc.us,
        nticks: 4096, //two drift times

        readout_time: self.tick*self.nticks,
        nreadouts: 1,

        start_time: 0.0*wc.s,
        stop_time: self.start_time + self.nreadouts*self.readout_time,

        first_frame_number: 1, // <<< I DON'T UNDERSTAND IT
    },

    adc: super.adc {

        //don't know this information (keep standard values)
        baselines: [900*wc.millivolt,900*wc.millivolt,200*wc.millivolt],

        // From ICARUS papers: https://iopscience.iop.org/article/10.1088/1748-0221/13/12/P12007/pdf

        resolution: 12, // #bit

        //check (values taken from the FE calibration shown in pg. 7 of the paper)
        fullscale: [0.8*wc.millivolt, 390*wc.millivolt],
    },

    // Leave it blank for now
    elec: super.elec { },

    sim : super.sim {

        // For running in LArSoft, the simulation must be in fixed time mode.
        fixed: true,

        // ductor logic adapted from PDSP
        // Assume 1.28 m of drift (coherent with the sampling time and window chosen)
        local tick0_time = -820*wc.us,

        local response_time_offset = 0.0*wc.us,  // modify to add a delay
        local response_nticks = wc.roundToInt(response_time_offset / $.daq.tick),


        ductor : {
            nticks: $.daq.nticks + response_nticks,
            readout_time: self.nticks * $.daq.tick,
            start_time: tick0_time - response_time_offset,
        },

        // If a ductor's time acceptance is increased then a Reframer
        // can be used to chop off the early excess to meet readout
        // assumptions.  Depending on the form of the ductor, the
        // reframer will likely need it's "tags" configured.
        reframer: {
            tbin: $.elec.fields.nticks,
            nticks: $.daq.nticks,
        }

    },

    files : {

        wires: "icarus-wires-dualanode.json.bz2",

        fields: ["garfield-1d-boundary-path-rev-dune.json.bz2",],

        noise: "t600-corr-noise-spectra.json.bz2",
        
        chresp: null,
    },

}
