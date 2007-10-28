#!/bin/sh

CC=${CC-gcc}

$CC -flags 2>&1 | grep 'Sun software' > /dev/null
if [ $? -eq 0 ]; then
  # Sun Forte compiler
  echo "Unsupported compiler"
  exit 1
else
  # Gnu?
  CFLAGS="-D_GNU_SOURCE -pipe -x c"
fi

tmp=`mktemp XXXXXXXX`
out=`mktemp XXXXXXXX`

echo "#include <stdio.h>" > $tmp
echo "int main() { char *foo; asprintf(&foo, \"testing\"); }" >> $tmp

${CC} $CFLAGS $tmp -o $out 2> /dev/null
if [ $? -ne 0 ] ; then
  cd snprintf_2.2/
  make CC=$CC CFLAGS="-DNEED_ASPRINTF"  >&2
  echo "snprintf_2.2/snprintf.o"
fi
 
rm -f $out $tmp
