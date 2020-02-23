#!/usr/bin/env python


TOP = '.'
APPNAME = 'WireCell'

def options(opt):
    opt.load("wcb")

def configure(cfg):
    cfg.load("wcb")

    # boost 1.59 uses auto_ptr and GCC 5 deprecates it vociferously.
    cfg.env.CXXFLAGS += ['-Wno-deprecated-declarations']
    cfg.env.CXXFLAGS += ['-Wall', '-Wno-unused-local-typedefs', '-Wno-unused-function']
    # cfg.env.CXXFLAGS += ['-Wpedantic', '-Werror']


def build(bld):
    bld.load('wcb')
