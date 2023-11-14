#!/bin/bash

cd $(dirname $0)
if ! grep -q "\-\-output\-junit \$1.xml" ../../externals/threadx/scripts/cmake_bootstrap.sh; then
  sed -i 's/ctest $parallel --timeout 1000 -O $1.txt/& --output-junit $1.xml/g' ../../externals/threadx/scripts/cmake_bootstrap.sh
fi
[ -f .run.sh ] || ln -sf ../../externals/threadx/scripts/cmake_bootstrap.sh .run.sh
./.run.sh $*