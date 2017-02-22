#!/usr/bin/env python

import os.path as osp
from waflib.Utils import to_list


TOP = '.'
APPNAME = 'WireCell'

def options(opt):
    opt.load('doxygen')                   # from waf
    opt.load('boost')

    opt.load("wcb",tooldir="waftools")
    opt.add_option('--doxygen-tarball', default=None,
                   help="Build Doxygen documentation to a tarball")
    opt.add_option('--doxygen-install-path', default="",
                   help="Build Doxygen documentation to a tarball")


def configure(cfg):
    cfg.load('doxygen')
    cfg.load('boost')

    cfg.load("wcb", tooldir="waftools")


    # application specific requirements 

    cfg.check_boost(lib='system filesystem graph thread program_options iostreams regex')

    cfg.check_cxx(header_name="boost/pipeline.hpp", use='BOOST',
                  define_name='BOOST_PIPELINE', mandatory=False)
    cfg.check(header_name="dlfcn.h", uselib_store='DYNAMO',
              lib=['dl'], mandatory=True)


    cfg.check(features='cxx cxxprogram', lib=['pthread'], uselib_store='PTHREAD')

    # boost 1.59 uses auto_ptr and GCC 5 deprecates it vociferously.
    cfg.env.CXXFLAGS += ['-Wno-deprecated-declarations']

    cfg.env.CXXFLAGS += to_list(cfg.options.build_debug)
    cfg.env.CXXFLAGS += ['-DEIGEN_FFTW_DEFAULT=1']

    known_sm = 'util iface gen alg sst bio rootvis apps sigproc dfp tbb'.split()
    known_sm.sort()
    existing_sm = [sm for sm in known_sm if osp.isdir(sm)]

    if 'BOOST_PIPELINE=1' not in cfg.env.DEFINES and 'dfp' in existing_sm:
        existing_sm.remove('dfp')

    if 'HAVE_TBB_TBB_H=1' not in cfg.env.DEFINES and 'tbb' in existing_sm:
        existing_sm.remove('tbb')

    cfg.env.SUBDIRS = existing_sm
    print 'Configured for: %s' % (', '.join(existing_sm), )


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

