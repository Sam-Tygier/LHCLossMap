#!/bin/bash -e
MADX=/afs/cern.ch/user/m/mad/madx/releases/last-rel/madx-linux64-gnu

mkdir -p results

$MADX < job_sample_2018.madx
