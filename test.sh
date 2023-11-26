#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./ktcc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 "{ return 0;}"
assert 42 "{ return 42;}"
assert 21 "{ return 5+20-4;}"
assert 41 "{ return 12 + 34 - 5;}"
assert 47 '{ return 5+6*7;}'
assert 15 '{ return 5 * (9 - 6);}'
assert 4 '{ return (3 + 5) / 2;}'
assert 5 '{ return -10 + 15;}'
assert 10 '{ return - -10;}'
assert 24 '{ return (-6) * (-4);}'
# ==, != のテスト
assert 1 '{ return 1 == 1;}'
assert 0 '{ return 1 == 2;}'
assert 1 '{ return 1 != 2;}'
assert 0 '{ return 1 != 1;}'
# <, <=, >, >= のテスト
assert 1 '{ return 1 < 2;}'
assert 0 '{ return 1 < 1;}'
assert 1 '{ return 1 <= 2;}'
assert 1 '{ return 1 <= 1;}'
assert 0 '{ return 2 <= 1;}'
assert 1 '{ return 2 > 1;}'
assert 0 '{ return 1 > 1;}'
assert 1 '{ return 2 >= 1;}'
assert 1 '{ return 1 >= 1;}'
assert 0 '{ return 1 >= 2;}'
# ; による複数の式のテスト
assert 7 '{ 1 + 2; return 3 + 4;}'
# 1文字変数のテスト
assert 3 '{ a = 3; return a;}'
assert 8 '{ a = 3; z = 5; return a + z;}'
assert 6 '{ a = z = 3; return a + z;}'
# 複数文字の変数のテスト
assert 3 '{ foo = 3; return foo;}'
assert 8 '{ foo123 = 3; bar = 5; return foo123 + bar;}'
# return文 のテスト
assert 1 '{ return 1; 2; 3;}'
assert 2 '{ 1; return 2; 3;}'
assert 3 '{ 1; 2; return 3;}'
# 空の文を受け付けるテスト
assert 5 '{ ;;; return 5; }'
# if文のテスト
assert 3 '{ if (0) return 2; return 3; }'
assert 3 '{ if (1-1) return 2; return 3; }'
assert 2 '{ if (1) return 2; return 3; }'
assert 2 '{ if (2-1) return 2; return 3; }'
assert 4 '{ if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 '{ if (1) { 1; 2; return 3; } else { return 4; } }'
assert 3 '{ if (0) { 1; } else { 0; } if ( 1 ) { return 3; } else { return 4; } }'
assert 9 '{ a = 10; if (1) { a = 9; } return a;}'
# for文のテスト
assert 1 '{ for ( i = 0; i < 2; i = i + 1) { 0; } return 1; }' 
assert 2 '{ for ( i = 0; i < 2; i = i + 1) { 0; } return i; }' 
assert 55 '{ i = 0; j = 0; for ( i = 0; i <= 10; i = i + 1) { j = i + j; } return j; }'
assert 3 '{ for (;;) { return 3; } return 5; }'
assert 15 '{ a = 0; for ( i = 0; i <= 10; i = i + 1 ) { if ( i == 2 ) { a = a + 10; } if ( i == 10 ) { a = a + 5; } } return a; }'

echo OK
