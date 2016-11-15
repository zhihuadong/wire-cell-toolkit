#!/usr/bin/env python

TOP = '.'
APPNAME = 'WireCell'

def options(opt):
    opt.load('doxygen')
    opt.load('smplpkgs')
    opt.load('rootsys')
    opt.load('eigen')
    opt.load('jsoncpp')
    opt.load('tbb')
    opt.load('boost')

    opt.add_option('--build-debug', default='-O2',
                   help="Build with debug symbols")
    opt.add_option('--doxygen-tarball', default=None,
                   help="Build Doxygen documentation to a tarball")
    opt.add_option('--doxygen-install-path', default="",
                   help="Build Doxygen documentation to a tarball")

def configure(cfg):
    cfg.load('doxygen')
    cfg.load('smplpkgs')
    cfg.load('rootsys')
    cfg.load('eigen')
    cfg.load('jsoncpp')
    cfg.load('tbb')
    cfg.load('boost')

    cfg.check_boost(lib='system filesystem graph thread program_options')

    cfg.check_cxx(header_name="boost/pipeline.hpp", use='BOOST',
                  define_name='BOOST_PIPELINE', mandatory=False)
    cfg.check(header_name="dlfcn.h", uselib_store='DYNAMO',
              lib=['dl'], mandatory=True)


    cfg.check(features='cxx cxxprogram', lib=['pthread'], uselib_store='PTHREAD')

    cfg.env.CXXFLAGS += [cfg.options.build_debug]
    cfg.env.SUBDIRS = 'util iface gen alg sst bio rootvis apps sigproc'.split()

    if 'BOOST_PIPELINE=1' in cfg.env.DEFINES:
        cfg.env.SUBDIRS += ['dfp'] # fixme: rename, make B.P specific

    if 'HAVE_TBB_TBB_H=1' in cfg.env.DEFINES:
        cfg.env.SUBDIRS += ['tbb'] 

    #print cfg.env



def build(bld):
    bld.load('smplpkgs')

    subdirs = bld.env.SUBDIRS
    print 'Building: %s' % (', '.join(subdirs), )

    bld.recurse(subdirs)
    if bld.env.DOXYGEN and bld.options.doxygen_tarball:
        bld(features="doxygen",
            doxyfile=bld.path.find_resource('Doxyfile'),
            install_path=bld.options.doxygen_install_path or bld.env.PREFIX + "/doc",
            doxy_tar = bld.options.doxygen_tarball)

