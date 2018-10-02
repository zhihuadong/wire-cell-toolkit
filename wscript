#!/usr/bin/env python

import os.path as osp
from waflib.Utils import to_list


TOP = '.'
APPNAME = 'WireCell'

def options(opt):
    opt.load('boost')
    opt.load("wcb",tooldir="waftools")

def configure(cfg):

    cfg.load('boost')
    cfg.load("wcb", tooldir="waftools")

    # application specific requirements 

    cfg.check_boost(lib='system filesystem graph thread program_options iostreams regex')

    #cfg.check_cxx(header_name="boost/pipeline.hpp", use='BOOST',
    #              define_name='BOOST_PIPELINE', mandatory=False)
    cfg.check(header_name="dlfcn.h", uselib_store='DYNAMO',
              lib=['dl'], mandatory=True)

    cfg.check(features='cxx cxxprogram', lib=['pthread'], uselib_store='PTHREAD')

    # boost 1.59 uses auto_ptr and GCC 5 deprecates it vociferously.
    cfg.env.CXXFLAGS += ['-Wno-deprecated-declarations']

    cfg.env.CXXFLAGS += to_list(cfg.options.build_debug)
    cfg.env.CXXFLAGS += ['-DEIGEN_FFTW_DEFAULT=1']

    cfg.env.CXXFLAGS += ['-Wall', '-Wno-unused-local-typedefs', '-Wno-unused-function']
    cfg.env.CXXFLAGS += ['-Wpedantic', '-Werror']

    # fixme: needed by cnpy in WireCellUtil.  should make this an explicit dependency
    cfg.env.LIB += ['z']

    known_sm = 'util iface gen pgraph sst sio rootvis apps sigproc dfp tbb ress cfg'.split()
    known_sm.sort()
    existing_sm = [sm for sm in known_sm if osp.isdir(sm)]

    if 'BOOST_PIPELINE=1' not in cfg.env.DEFINES and 'dfp' in existing_sm:
        existing_sm.remove('dfp')

    if 'HAVE_TBB' not in cfg.env and 'tbb' in existing_sm:
        existing_sm.remove('tbb')

    cfg.env.SUBDIRS = existing_sm
    print 'Configured for: %s' % (', '.join(existing_sm), )

def build(bld):
    bld.load('smplpkgs')

    subdirs = bld.env.SUBDIRS
    print 'Building: %s' % (', '.join(subdirs), )

    bld.recurse(subdirs)

