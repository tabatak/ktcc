#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenize.c
//

// トークンの種類
typedef enum
{
    TK_RESERVED, // 記号
    TK_IDENT,    // 識別子
    TK_NUM,      // 整数トークン
    TK_RETURN,   // return
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
void error_at(char *loc, char *fmt, ...);
void error_tok(Token *tok, char *fmt, ...);
bool equal(Token *tok, char *op);
Token *skip(Token *tok, char *op);
Token *tokenize(char *user_input);

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
    int offset;
};

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
    ND_VAR,       // variable
    ND_BLOCK,     // { ... }
    ND_RETURN,    // return
} NodeKind;

// 抽象構文木のノードの型
struct Node
{
    NodeKind kind; // ノードの型
    Node *next;    // 次のノード
    Node *lhs;     // 左辺
    Node *rhs;     // 右辺
    int val;       // kindがND_NUMの場合のみ値が設定される
    Obj *var;      // kindがND_VARの場合のみ値が設定される

    Node *body; // { ... } 内の文を保持
};

Function *parse(Token *tok);

//
// codegen.c
//
// コード生成
void codegen(Function *prog);
