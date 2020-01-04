#!/bin/bash 

set -x
test -d ./config || mkdir ./config
autoreconf --force --install -I config -I m4
set +x