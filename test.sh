#!/bin/bash
try() {
    expected="$1"
    input="$2"

    ./ktcc "$input" > tmp.s
    gcc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" != "$expected" ]; then
        echo "$input expected, but got $actual"
        exit 1
    fi
}


try 0 0
try 42 42

try 21 '5+20-4'
try 3 '20-20+3'

try 41 ' 12 + 34 - 5 '

echo OK
