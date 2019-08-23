#!/usr/bin/env python

import os.path as osp
from waflib.Utils import to_list


TOP = '.'
APPNAME = 'WireCell'

def find_submodules(ctx):
    sms = list()
    for wb in ctx.path.ant_glob("**/wscript_build"):
        sms.append(wb.parent.name)
    sms.sort()
    return sms

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

    submodules = find_submodules(cfg)

    # submodules = 'util iface gen sigproc img pgraph apps sio dfp tbb ress cfg root'.split()
    # submodules.sort()
    # submodules = [sm for sm in submodules if osp.isdir(sm)]


    if 'BOOST_PIPELINE=1' not in cfg.env.DEFINES and 'dfp' in submodules:
        print ('Removing submodule "dfp" due to lack of external')
        submodules.remove('dfp')

    # Remove WCT packages that happen to have same name as external name
    for pkg,ext in dict(root="ROOTSYS", tbb="TBB", cuda="CUDA").items():
        have='HAVE_'+ext
        if have in cfg.env:
            continue
        if pkg in submodules:
            print ('Removing package "%s" due to lack of external dependency'%pkg)
            submodules.remove(pkg)

    cfg.env.SUBDIRS = submodules
    print ('Configured for submodules: %s' % (', '.join(submodules), ))

def build(bld):
    bld.load('smplpkgs')

    subdirs = bld.env.SUBDIRS
    print ('Building: %s' % (', '.join(subdirs), ))

    bld.recurse(subdirs)

