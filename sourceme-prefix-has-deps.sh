#!/bin/bash

# if the prefix for installing Wire Cell also holds all the deps, then
# this script simplifies configuration.


### latest ROOT no longer needs this workaround
# if [ "$(lsb_release -s -c)" = "xenial" ] ; then
#     echo "Setting compilers to GCC 4.9 on xenial"
#     export CC=/usr/bin/gcc-4.9
#     export CXX=/usr/bin/g++-4.9
# fi

wcb-configure () {
    local prefix="$1" ; shift
    if [ -z "$prefix" ] ; then
	echo usage: "wcb-configure /path/to/prefix [...]"
	return 1
    fi

    prefix=$(readlink -f $prefix)
    
    for sd in lib lib64 share
    do
	maybe="$prefix/$sd/pkgconfig"
	if [ -d $maybe ] ; then
	    PKG_CONFIG_PATH=$PKG_CONFIG_PATH${PKG_CONFIG_PATH:+:}$maybe
	fi
    done
    export PKG_CONFIG_PATH
    
    
    local mydir=$(dirname $(readlink -f $BASH_SOURCE))
    cd $mydir
    ./wcb configure \
	  --prefix=$prefix \
	  --with-eigen=$prefix \
	  --with-jsoncpp=$prefix \
	  --with-tbb=$prefix \
	  --boost-includes=$prefix/include \
	  --boost-libs=$prefix/lib \
	  --boost-mt \
	  --with-root=$prefix \
	  --with-fftw=$prefix \
	  "$@"
}

# setup run time environment
wcb-runtime-setup () {
    local prefix="$1" ; shift
    if [ -z "$prefix" ] ; then
	echo usage: "wcb-runtime-setup /path/to/prefix [...]"
	return 1
    fi
    prefix=$(readlink -f $prefix)
    PATH=$prefix/bin:$PATH
    # fixme, what about lib64?
    export LD_LIBRARY_PATH=$prefix/lib:${LD_LIBRARY_PATH:+:}$LD_LIBRARY_PATH
    #export PKG_CONFIG_PATH=$prefix/lib:${PKG_CONFIG_PATH:+:}$PKG_CONFIG_PATH
}

#wcb-run-test () {
#    for exe in $(find . -name $testprog
