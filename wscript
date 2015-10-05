#!/usr/bin/env python

TOP = '.'
APPNAME = 'WireCell'

def options(opt):
    opt.load('doxygen')
    opt.load('smplpkgs')
    opt.add_option('--build-debug', default='-O2',
                   help="Build with debug symbols")
    opt.add_option('--doxygen-tarball', default=None,
                   help="Build Doxygen documentation to a tarball")
    opt.add_option('--doxygen-install-path', default="",
                   help="Build Doxygen documentation to a tarball")

def configure(cfg):
    cfg.load('doxygen')
    cfg.load('smplpkgs')

    cfg.check_cxx(header_name="boost/pipeline.hpp", use='BOOST',
                  define_name='BOOST_PIPELINE', mandatory=False)

    cfg.check_cxx(header_name="Rtypes.h", use='ROOTSYS',
                  mandatory=True)

    # JSONCPP
    # cfg.check_cfg(package='jsoncpp',  uselib_store='JSONCPP', args='--cflags --libs',
    #               mandatory=False)
    # cfg.check(header_name="json/json.h", use='JSONCPP', mandatory=False)

    cfg.env.CXXFLAGS += [cfg.options.build_debug]
    #cfg.env.SUBDIRS = 'util iface gen rio riodata rootdict'.split()
    cfg.env.SUBDIRS = 'util iface gen alg sst bio rootvis'.split()
    if 'BOOST_PIPELINE=1' in cfg.env.DEFINES:
        cfg.env.SUBDIRS += ['dfp']
    # if 'HAVE_JSON_JSON_H=1' in cfg.env.DEFINES:
    #     cfg.env.SUBDIRS += ['bio']    


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

