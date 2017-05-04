#!/bin/bash

# This defines some Bash functions which help simply development of
# WCT on the assumption that externals are installed in a Spack view
# (or equivalent) and that WCT itself is installed in some other
# directory (although the two may be degenerate)


# This file can be sourced with paths to externals and WCT
# installation prefix:
#
# source sourceme-external-prefix.sh /path/to/externals /path/to/prefix
#
# or define WCT_EXTERNALS and WCT_PREFIX to each path, respectively.

export WCT_EXTERNALS=${WCT_EXTERNALS:-$1}
export WCT_PREFIX=${WCT_PREFIX:-$2}

addpath ()
{
    local pathvar=${2:-PATH}
    local pathval="${!pathvar}"
    pathval=${pathval//":$1"/}
    pathval=${pathval//"$1:"/}
    export $pathvar="$1:$pathval"
}

wct-configure () {
    
    addpath "$WCT_EXTERNALS/lib/pkgconfig" PKG_CONFIG_PATH
    addpath "$WCT_EXTERNALS/share/pkgconfig" PKG_CONFIG_PATH

    local mydir=$(dirname $(readlink -f $BASH_SOURCE))
    cd $mydir

    ./wcb configure \
	  --prefix=$WCT_PREFIX \
	  --boost-includes=$WCT_EXTERNALS/include \
	  --boost-libs=$WCT_EXTERNALS/lib \
	  --boost-mt \
	  --with-eigen=$WCT_EXTERNALS \
	  --with-jsoncpp=$WCT_EXTERNALS \
	  --with-tbb=$WCT_EXTERNALS \
	  --with-root=$WCT_EXTERNALS \
	  --with-fftw=$WCT_EXTERNALS \
	  --with-jsonnet=$WCT_EXTERNALS \
	  "$@"
}

wct-test () {
    local name=$1 ; shift
    
    local mydir=$(dirname $(readlink -f $BASH_SOURCE))
    local old_ld_library_path="$LD_LIBRARY_PATH"
    addpath $WCT_EXTERNALS/lib LD_LIBRARY_PATH
    for maybe in $mydir/build/* ;
    do
	maybe="$(readlink -f $maybe)"
	if [ -d "$maybe" ] ; then
	    addpath $maybe LD_LIBRARY_PATH
	fi
    done
    $mydir/build/*/test_$name $@
    LD_LIBRARY_PATH=$old_ld_library_path
}

wct-run () {
    local mydir=$(dirname $(readlink -f $BASH_SOURCE))
    local old_ld_library_path="$LD_LIBRARY_PATH"
    local old_path="$PATH"
    addpath $WCT_EXTERNALS/lib LD_LIBRARY_PATH
    addpath $WCT_EXTERNALS/bin PATH
    addpath $WCT_PREFIX/lib LD_LIBRARY_PATH
    addpath $WCT_PREFIX/bin PATH

    # configuration and field/wire data files
    addpath $WCT_PREFIX/share/wirecell/data   WIRECELL_PATH
    addpath $WCT_PREFIX/share/wirecell/config WIRECELL_PATH
    addpath $WCT_PREFIX/data                  WIRECELL_PATH

    for maybe in $mydir/build/* ;
    do
	maybe="$(readlink -f $maybe)"
	if [ -d "$maybe" ] ; then
	    addpath $maybe LD_LIBRARY_PATH
	fi
    done
    $@
    LD_LIBRARY_PATH=$old_ld_library_path
    PATH=$old_path
}
