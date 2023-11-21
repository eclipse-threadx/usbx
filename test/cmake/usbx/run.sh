#!/bin/bash

cd $(dirname $0)

# Checkout externals
[ -d externals] || mkdir ../../externals
git clone https://github.com/azure-rtos/threadx.git ../../externals/threadx
git clone https://github.com/azure-rtos/netxduo.git ../../externals/netxduo
git clone https://github.com/azure-rtos/filex.git   ../../externals/filex

# Add junit output for ctest generation
if ! grep -q "\-\-output\-junit \$1.xml" ../../externals/threadx/scripts/cmake_bootstrap.sh; then
  sed -i 's/ctest $parallel --timeout 1000 -O $1.txt/& --output-junit $1.xml/g' ../../externals/threadx/scripts/cmake_bootstrap.sh
fi

[ -f .run.sh ] || ln -sf ../../externals/threadx/scripts/cmake_bootstrap.sh .run.sh
./.run.sh $*