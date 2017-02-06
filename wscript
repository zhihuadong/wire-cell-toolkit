#!/usr/bin/env python

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

