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

    submodules = 'util iface gen sigproc img pgraph apps sio dfp tbb ress cfg root'.split()
    submodules.sort()
    submodules = [sm for sm in submodules if osp.isdir(sm)]


    if 'BOOST_PIPELINE=1' not in cfg.env.DEFINES and 'dfp' in submodules:
        submodules.remove('dfp')

    if 'HAVE_TBB' not in cfg.env and 'tbb' in submodules:
        submodules.remove('tbb')


    if 'HAVE_ROOTSYS' not in cfg.env:
        # eventually, make this list hold only "root"
        needsroot = 'root'.split()
        for sm in needsroot:
            if sm in submodules:
                submodules.remove(sm)
                print ("build is sans ROOT, removed module: %s" % sm)
        

    cfg.env.SUBDIRS = submodules
    print ('Configured for: %s' % (', '.join(submodules), ))

def build(bld):
    bld.load('smplpkgs')

    subdirs = bld.env.SUBDIRS
    print ('Building: %s' % (', '.join(subdirs), ))

    bld.recurse(subdirs)

