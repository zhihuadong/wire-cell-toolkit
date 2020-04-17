#!/bin/bash

# This script runs check_zioflow and "zio file-server".
# It is related to 
# https://github.com/WireCell/wire-cell-toolkit/issues/54

# It runs from 
tstdir=$(dirname $(realpath $BASH_SOURCE))
pkgdir=$(dirname $tstdir)
topdir=$(dirname $pkgdir)
blddir=$topdir/build

# take ZIO from environment
if [ ! -x "$(which zio)" ] ; then
    echo "This test needs ZIO Python CLI 'zio'"
    exit
fi

# take wire-cell from build
czf=$blddir/zio/check_zioflow
if [ ! -x $czf ] ; then
    echo "No check program found: $czf"
    exit -1
fi

tmpdir=$(mktemp -d /tmp/wct-issue54.XXXX)
echo $tmpdir

cfg=$tstdir/check_zioflow.jsonnet
if [ ! -f $cfg ] ; then
    echo "No cfg found: $cfg"
    exit -1
fi

cat <<EOF > $tmpdir/Procfile
czf: $czf
zio: zio flow-file-server -v debug -n zioflow -p flow $cfg
EOF

set -x
cd $tmpdir
$topdir/util/scripts/shoreman 


#rm -rf $tmpdir
echo "not removing $tmpdir"





