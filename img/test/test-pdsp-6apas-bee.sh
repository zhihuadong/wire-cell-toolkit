#!/bin/bash

# Leave this script in place.
# 
# Run PDSP sig+noise sim + sigproc + imaging using one or more input
# depo files in Bee JSON format.
#
# Example JSON files provided:
# 
# $ unzip sio/test/test_beedeposource.zip
# $ ls -R data/
#
# Then run this script like:
# 
# $ ./img/test/test-pdsp-6apas-bee.sh data/?/?-truthDepo.json
#
# or for just one "event":
# 
# $ ./img/test/test-pdsp-6apas-bee.sh data/0/0-truthDepo.json

cfgfile="${BASH_SOURCE/.sh/.jsonnet}"
if [ ! -f "$cfgfile" ] ; then
    echo "expect to find $cfgfile"
    exit 1
fi

if [ -z "$1" ] ; then
    echo "Need at least one input depo file"
    exit 1
fi
depofiles=$(for n in $@; do echo -n '"'$n'"',; done)

set -x
wire-cell -L debug -l stdout -C "depofiles=["$depofiles"]" -c $cfgfile
