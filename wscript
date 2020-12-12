#!/usr/bin/env python


TOP = '.'
APPNAME = 'WireCell'

def options(opt):
    opt.load("wcb")

    # this used in cfg/wscript_build
    opt.add_option('--install-config', type=str, default="",
                   help="Install configuration files for given experiment")


def configure(cfg):
    cfg.load("wcb")

    # fixme: should go into wcb.py
    cfg.find_program("jsonnet", var='JSONNET')

    # boost 1.59 uses auto_ptr and GCC 5 deprecates it vociferously.
    cfg.env.CXXFLAGS += ['-Wno-deprecated-declarations']
    cfg.env.CXXFLAGS += ['-Wall', '-Wno-unused-local-typedefs', '-Wno-unused-function']
    # cfg.env.CXXFLAGS += ['-Wpedantic', '-Werror']

def build(bld):
    bld.load('wcb')
