#!/bin/bash
cat <<EOF | gcc -xc -c -o tmp2.o -
int ret3() { return 3; }
int ret5() { return 5; }
int add(int x, int y) { return x+y; }
int sub(int x, int y) { return x-y; }
int add6(int a, int b, int c, int d, int e, int f) {
  return a+b+c+d+e+f;
}
EOF

assert() {
    expected="$1"
    input="$2"

    ./ktcc "$input" > tmp.s || exit
    cc -static -o tmp tmp.s tmp2.o
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 "int main() { return 0;}"
assert 42 "int main() { return 42;}"
assert 21 "int main() { return 5+20-4;}"
assert 41 "int main() { return 12 + 34 - 5;}"
assert 47 'int main() { return 5+6*7;}'
assert 15 'int main() { return 5 * (9 - 6);}'
assert 4 'int main() { return (3 + 5) / 2;}'
assert 5 'int main() { return -10 + 15;}'
assert 10 'int main() { return - -10;}'
assert 24 'int main() { return (-6) * (-4);}'
# ==, != のテスト
assert 1 'int main() { return 1 == 1;}'
assert 0 'int main() { return 1 == 2;}'
assert 1 'int main() { return 1 != 2;}'
assert 0 'int main() { return 1 != 1;}'
# <, <=, >, >= のテスト
assert 1 'int main() { return 1 < 2;}'
assert 0 'int main() { return 1 < 1;}'
assert 1 'int main() { return 1 <= 2;}'
assert 1 'int main() { return 1 <= 1;}'
assert 0 'int main() { return 2 <= 1;}'
assert 1 'int main() { return 2 > 1;}'
assert 0 'int main() { return 1 > 1;}'
assert 1 'int main() { return 2 >= 1;}'
assert 1 'int main() { return 1 >= 1;}'
assert 0 'int main() { return 1 >= 2;}'
# ; による複数の式のテスト
assert 7 'int main() { 1 + 2; return 3 + 4;}'
# 1文字変数のテスト
assert 3 'int main() { int a = 3; return a;}'
assert 8 'int main() { int a = 3; int z = 5; return a + z;}'
# 複数文字の変数のテスト
assert 3 'int main() { int foo = 3; return foo;}'
assert 8 'int main() { int foo123 = 3; int bar = 5; return foo123 + bar;}'
# return文 のテスト
assert 1 'int main() { return 1; 2; 3;}'
assert 2 'int main() { 1; return 2; 3;}'
assert 3 'int main() { 1; 2; return 3;}'
# 空の文を受け付けるテスト
assert 5 'int main() { ;;; return 5; }'
# if文のテスト
assert 3 'int main() { if (0) return 2; return 3; }'
assert 3 'int main() { if (1-1) return 2; return 3; }'
assert 2 'int main() { if (1) return 2; return 3; }'
assert 2 'int main() { if (2-1) return 2; return 3; }'
assert 4 'int main() { if (0) { 1; 2; return 3; } else { return 4; } }'
assert 3 'int main() { if (1) { 1; 2; return 3; } else { return 4; } }'
assert 3 'int main() { if (0) { 1; } else { 0; } if ( 1 ) { return 3; } else { return 4; } }'
assert 9 'int main() { int a = 10; if (1) { a = 9; } return a;}'
# for文のテスト
assert 1 'int main() { int i = 0; for ( i = 0; i < 2; i = i + 1) { 0; } return 1; }' 
assert 2 'int main() { int i = 0; for ( i = 0; i < 2; i = i + 1) { 0; } return i; }' 
assert 55 'int main() { int i = 0; int j = 0; for ( i = 0; i <= 10; i = i + 1) { j = i + j; } return j; }'
assert 3 'int main() { for (;;) { return 3; } return 5; }'
assert 15 'int main() { int a = 0; int i = 0; for ( i = 0; i <= 10; i = i + 1 ) { if ( i == 2 ) { a = a + 10; } if ( i == 10 ) { a = a + 5; } } return a; }'
# while文のテスト
assert 10 'int main() { int i = 0; while ( i < 10 ) { i = i + 1; } return i; }'
# &, * のテスト
assert 3 'int main() { int x=3; return *&x; }'
assert 3 'int main() { int x=3; int y=&x; int z=&y; return **z; }'
assert 5 'int main() { int x=3; int y=5; return *(&x+1); }'
assert 3 'int main() { int x=3; int y=5; return *(&y-1); }'
assert 5 'int main() { int x=3; int y=5; return *(&x-(-1)); }'
assert 5 'int main() { int x=3; int y=&x; *y=5; return x; }'
assert 7 'int main() { int x=3; int y=5; *(&x+1)=7; return y; }'
assert 7 'int main() { int x=3; int y=5; *(&y-2+1)=7; return x; }'
assert 5 'int main() { int x=3; return (&x+2)-&x+3; }'

assert 8 'int main() { int x, y; x=3; y=5; return x+y; }'
# 関数呼び出し
assert 3 'int main() { return ret3(); }'
assert 5 'int main() { return ret5(); }'
assert 8 'int main() { return add(3, 5); }'
assert 2 'int main() { return sub(5, 3); }'
assert 21 'int main() { return add6(1,2,3,4,5,6); }'
assert 66 'int main() { return add6(1,2,add6(3,4,5,6,7,8),9,10,11); }'
assert 136 'int main() { return add6(1,2,add6(3,add6(4,5,6,7,8,9),10,11,12,13),14,15,16); }'
assert 8 'int main() { int a = 3; int b = 5; return add(a, b); }'
assert 32 'int main() { return ret32(); } int ret32() { return 32; }'
assert 44 'int main() { int a = ret32(); return a + 12; } int ret32() { return 32; }'

echo OK
