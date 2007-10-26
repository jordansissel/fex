#!/bin/sh

. ./test.subr

TESTS=
if [ $# -gt 0 ]; then 
  TESTS="$@"
else
  TESTS="basic numrange multifields othersplit multisplit"
fi

set -- $TESTS
while [ $# -gt 0 ]; do
  try $1
  shift
done
