# Aggregate all the waftools to make the main wscript a bit shorter.
# Note, this is specific to WC building

import generic
import os.path as osp
mydir = osp.dirname(__file__)

## These are packages descriptions which fit the generic functions.
package_descriptions = dict(
    ## These typically CAN be found by pkg-config
    ZLib    = dict(incs=['zlib.h'], libs=['z']),
    FFTW    = dict(incs=['fftw3.h'], libs=['fftw3f'], pcname='fftw3f'),
    JsonCpp = dict(incs=["json/json.h"], libs=['jsoncpp']),
    ## These can't always be found by pkg-config:
    Eigen   = dict(incs=["Eigen/Dense"]),
    ## These likely can NOT be found by pkg-config:
    Jsonnet = dict(incs=["libjsonnet++.h"], libs=['jsonnet++','jsonnet']),
    TBB     = dict(incs=["tbb/parallel_for.h"], libs=['tbb'], mandatory=False),

    # note, actually, pgk-config fails often.  best to always use
    # explicit --with-NAME.
)


def options(opt):

    # from here
    opt.load('smplpkgs',tooldir=mydir)
    opt.load('rootsys',tooldir=mydir)
    opt.load('cuda',tooldir=mydir)
    # opt.load('tbb',tooldir=mydir)

    for name in package_descriptions:
        generic._options(opt, name)

    opt.add_option('--build-debug', default='-O2 -ggdb3',
                   help="Build with debug symbols")

def configure(cfg):
    print ('Compile options: %s' % cfg.options.build_debug)

    cfg.load('smplpkgs')

    for name, args in package_descriptions.items():
        #print ("Configure: %s %s" % (name, args))
        generic._configure(cfg, name, **args)
        #print ("configured %s" % name)

    if cfg.options.with_cuda is False:
        print ("sans CUDA")
    else:
        cfg.load('cuda')

    if cfg.options.with_root is False:
        print ("sans ROOT")
    else:
        cfg.load('rootsys')
    # cfg.load('tbb')
    # boost is assumed built in to main waf/wcb program via
    # ./waf-light --tools=doxygen,boost,bjam

    
