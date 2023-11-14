#!/bin/bash

cd $(dirname $0)
[ -f .run.sh ] || ln -sf ../../externals/threadx/scripts/cmake_bootstrap.sh .run.sh
if ! grep -q "\-\-output\-junit \$1.xml" .run.sh; then
  sed -i 's/ctest $parallel --timeout 1000 -O $1.txt/& --output-junit $1.xml/g' .run.sh
fi
./.run.sh $*