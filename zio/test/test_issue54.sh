#!/bin/bash

# This script runs the job satisfying:
# https://github.com/WireCell/wire-cell-toolkit/issues/54

# It runs from 
tstdir=$(dirname $(realpath $BASH_SOURCE))
pkgdir=$(dirname $tstdir)
topdir=$(dirname $pkgdir)
blddir=$topdir/build

if [ ! -x "$(which jsonnet)" ] ; then
    echo "This test needs jsonnet"
    exit
fi

# take ZIO from environment
if [ ! -x "$(which zio)" ] ; then
    echo "This test needs ZIO Python CLI 'zio'"
    exit
fi

# take wire-cell from build
wc=$blddir/apps/wire-cell
if [ ! -x $wc ] ; then
    echo "No wire-cell found: $wc"
    exit -1
fi

cfg=$tstdir/issue54.jsonnet
if [ ! -f $cfg ] ; then
    echo "No cfg found: $cfg"
    exit -1
fi
tmpdir=$(mktemp -d /tmp/wct-issue54.XXXX)
echo $tmpdir
jsonnet -J $topdir/cfg -m $tmpdir $cfg
if [ ! -f $tmpdir/wccfg.json ] ; then
    echo "Failed to generate wire-cell config"
    exit -1
fi
if [ ! -f $tmpdir/ziocfg.json ] ; then
    echo "Failed to generate zio config"
    exit -1
fi

cat <<EOF > $tmpdir/Procfile
wc: $wc -l stdout::debug -L debug -c $tmpdir/wccfg.json
zio: zio flow-file-server -v debug -n issue54server -p flow $tmpdir/ziocfg.json
EOF

set -x
cd $tmpdir
$topdir/util/scripts/shoreman 


#rm -rf $tmpdir
echo "not removing $tmpdir"





