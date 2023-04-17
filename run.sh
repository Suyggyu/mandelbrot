#!/bin/sh

bin="mandelbrot"
ldlibs="-lm -lncurses -pthread"
opts="-DNTHREADS=6"
cc="gcc"

[ -z "$1" ] && opts="$opts -g"
[ "$1" = "release" ] && opts="$opts -O2"
[ "$1" = "benchmark" ] && opts="$opts -DBENCHMARK -O2"
[ "$1" = "static" ] && opts="$opts -static -O2"

set -x

$cc $opts -o $bin src/*.c $ldlibs
compile_status=$?

set +x

if [ "$1" = "release" ] || [ "$1" = "static" ]
then
	strip ./$bin
fi

[ $compile_status -eq 0 ] && time ./$bin
