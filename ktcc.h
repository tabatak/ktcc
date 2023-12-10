#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Type Type;

//
// tokenize.c
//

// トークンの種類
typedef enum
{
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数トークン
    TK_KEYWORD,  // keyword
    TK_EOF,      // 入力の終わりを表すトークン
} TokenKind;

typedef struct Token Token;

// トークン型
struct Token
{
    TokenKind kind; // トークンの型
    Token *next;    // 次の入力トークン
    int val;        // kindがTK_NUMの場合、その数値
    char *loc;      // トークン位置
    int len;        // トークンの長さ
};

void error(char *fmt, ...);
void verror_at(char *loc, char *fmt, va_list ap);
void error_tok(Token *tok, char *fmt, ...);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *op);
Token *tokenize(char *user_input);
bool consume(Token **rest, Token *tok, char *str);

//
// parse.c
//

typedef struct Node Node;

// Local variable
typedef struct Obj Obj;
struct Obj
{
    Obj *next;
    char *name;
    Type *ty;
    int offset;
};

//
// type.c
//

typedef enum
{
    TY_INT,
    TY_PTR,
} TypeKind;

struct Type
{
    TypeKind kind;

    // Pointer
    Type *base;

    // Declaration
    Token *name;
};

extern Type *ty_int;

bool is_integer(Type *ty);
void add_type(Node *node);
Type *pointer_to(Type *base);

// Function
typedef struct Function Function;
struct Function
{
    Node *body;
    Obj *locals;
    int stack_size;
};

// 抽象構文木のノードの種類
typedef enum
{
    ND_ADD,       // +
    ND_SUB,       // -
    ND_MUL,       // *
    ND_DIV,       // /
    ND_EQ,        // ==
    ND_NE,        // !=
    ND_LT,        // <
    ND_LE,        // <=
    ND_NUM,       // 整数
    ND_EXPR_STMT, // expression statement
    ND_ASSIGN,    // = 代入
    ND_NEG,       // unary -
    ND_ADDR,      // unary &
    ND_DEREF,     // unary *
    ND_VAR,       // variable
    ND_IF,        // if文
    ND_FOR,       // for文 or while文
    ND_BLOCK,     // { ... }
    ND_FUNCCALL,  // 関数呼び出し
    ND_RETURN,    // return
} NodeKind;

// 抽象構文木のノードの型
struct Node
{
    NodeKind kind; // ノードの型
    Node *next;    // 次のノード
    Type *ty;      // 型
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kindがND_NUMの場合のみ値が設定される
    Obj *var;      // kindがND_VARの場合のみ値が設定される

    // if文 or for文
    Node *cond;
    Node *then;
    Node *els;
    Node *init;
    Node *inc;

    // ブロック
    Node *body;

    // 関数呼び出し
    char *funcname;
    Node *args;
};

Function *parse(Token *tok);

//
// codegen.c
//
// コード生成
void codegen(Function *prog);
