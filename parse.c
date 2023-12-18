#include "ktcc.h"

Obj *locals;

// ローカル変数の管理用
Obj *find_var(Token *tok)
{
    for (Obj *var = locals; var; var = var->next)
    {
        if (strlen(var->name) == tok->len && !strncmp(tok->loc, var->name, tok->len))
        {
            return var;
        }
    }
    return NULL;
}

Node *new_node(NodeKind kind)
{
    Node *node = calloc(1, sizeof(Node));
    node->kind = kind;
    return node;
}

Node *new_binary(NodeKind kind, Node *lhs, Node *rhs)
{
    Node *node = new_node(kind);
    node->lhs = lhs;
    node->rhs = rhs;
    return node;
}

Node *new_unary(NodeKind kind, Node *expr)
{
    Node *node = new_node(kind);
    node->lhs = expr;
    return node;
}

Node *new_num(int val)
{
    Node *node = new_node(ND_NUM);
    node->val = val;
    return node;
}

Node *new_var(Obj *var)
{
    Node *node = new_node(ND_VAR);
    node->var = var;
    return node;
}

Obj *new_lvar(char *name, Type *ty)
{
    Obj *var = calloc(1, sizeof(Obj));
    var->name = name;
    var->ty = ty;
    var->next = locals;
    locals = var;
    return var;
}

Type *declarator(Token **rest, Token *tok, Type *ty);
Node *declaration(Token **rest, Token *tok);
Node *compound_stmt(Token **rest, Token *tok);
Node *expr(Token **rest, Token *tok);
Node *expr_stmt(Token **rest, Token *tok);
Node *assign(Token **rest, Token *tok);
Node *equality(Token **rest, Token *tok);
Node *relational(Token **rest, Token *tok);
Node *add(Token **rest, Token *tok);
Node *mul(Token **rest, Token *tok);
Node *unary(Token **rest, Token *tok);
Node *primary(Token **rest, Token *tok);

Node *new_add(Node *lhs, Node *rhs, Token *tok);
Node *new_sub(Node *lhs, Node *rhs, Token *tok);

char *get_ident(Token *tok)
{
    if (tok->kind != TK_IDENT)
    {
        error_tok(tok, "expected an identifier");
    }
    return strndup(tok->loc, tok->len);
}

// declspec = "int"
Type *declspec(Token **rest, Token *tok)
{
    *rest = skip(tok, "int");
    return ty_int;
}

// type-suffix = ("(" func-params? ")")?
// func-params = param ("," param)*
// param       = declspec declarator
Type *type_suffix(Token **rest, Token *tok, Type *ty)
{
    if (equal(tok, "("))
    {
        tok = tok->next;

        Type head = {};
        Type *cur = &head;

        while (!equal(tok, ")"))
        {
            if (cur != &head)
            {
                tok = skip(tok, ",");
            }
            Type *basety = declspec(&tok, tok);
            Type *ty = declarator(&tok, tok, basety);
            cur = cur->next = copy_type(ty);
        }

        ty = func_type(ty);
        ty->params = head.next;
        *rest = tok->next;
        return ty;
    }

    *rest = tok;
    return ty;
}

// declarator = "*"* ident type-suffix
Type *declarator(Token **rest, Token *tok, Type *ty)
{
    while (consume(&tok, tok, "*"))
    {
        // memo: 複数回のderefに対応するために、tyを更新している
        ty = pointer_to(ty);
    }

    if (tok->kind != TK_IDENT)
    {
        error_tok(tok, "expected a variable name");
    }

    ty = type_suffix(rest, tok->next, ty);
    ty->name = tok;
    return ty;
}

// declaration = declspec (declarator ("=" expr)? ("," declarator ("=" expr)?)*)? ";"
Node *declaration(Token **rest, Token *tok)
{
    Type *basety = declspec(&tok, tok);

    Node head = {};
    Node *cur = &head;

    int i = 0;
    while (!equal(tok, ";"))
    {
        if (i++)
        {
            tok = skip(tok, ",");
        }

        Type *ty = declarator(&tok, tok, basety);
        Obj *var = new_lvar(get_ident(ty->name), ty);

        if (!equal(tok, "="))
        {
            continue;
        }

        Node *lhs = new_var(var);
        Node *rhs = assign(&tok, tok->next);
        Node *node = new_binary(ND_ASSIGN, lhs, rhs);
        cur = cur->next = new_unary(ND_EXPR_STMT, node);
    }

    Node *node = new_node(ND_BLOCK); // TODO: なぜND_BLOCKを使っているのか？
    node->body = head.next;
    *rest = tok->next;
    return node;
}

// stmt = "return" expr ";" | expr-stmt
Node *stmt(Token **rest, Token *tok)
{
    if (equal(tok, "return"))
    {
        Node *node = new_unary(ND_RETURN, expr(&tok, tok->next));
        *rest = skip(tok, ";");
        return node;
    }

    if (equal(tok, "if"))
    {
        Node *node = new_node(ND_IF);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(&tok, tok);
        if (equal(tok, "else"))
        {
            node->els = stmt(&tok, tok->next);
        }
        *rest = tok;
        return node;
    }

    if (equal(tok, "for"))
    {
        // for (init; cond; inc) body

        Node *node = new_node(ND_FOR);
        tok = skip(tok->next, "(");

        node->init = expr_stmt(&tok, tok);

        if (!equal(tok, ";"))
        {
            node->cond = expr(&tok, tok);
        }
        tok = skip(tok, ";");

        if (!equal(tok, ")"))
        {
            node->inc = expr(&tok, tok);
        }
        tok = skip(tok, ")");

        node->then = stmt(rest, tok);
        return node;
    }

    if (equal(tok, "while"))
    {
        Node *node = new_node(ND_FOR);
        tok = skip(tok->next, "(");
        node->cond = expr(&tok, tok);
        tok = skip(tok, ")");
        node->then = stmt(rest, tok);
        return node;
    }

    if (equal(tok, "{"))
    {
        Node *node = compound_stmt(&tok, tok->next);
        *rest = tok;
        return node;
    }

    return expr_stmt(rest, tok);
}

// compound-stmt = (declaration | stmt)* "}"
Node *compound_stmt(Token **rest, Token *tok)
{
    Node head = {};
    Node *cur = &head;

    while (!equal(tok, "}"))
    {
        if (equal(tok, "int"))
        {
            cur = cur->next = declaration(&tok, tok);
        }
        else
        {
            cur = cur->next = stmt(&tok, tok);
        }
        add_type(cur);
    }

    Node *node = new_node(ND_BLOCK);
    node->body = head.next;

    *rest = tok->next;
    return node;
}

// expr-stmt = expr? ";"
Node *expr_stmt(Token **rest, Token *tok)
{
    if (equal(tok, ";"))
    {
        *rest = tok->next;
        return new_node(ND_BLOCK);
    }

    Node *node = new_unary(ND_EXPR_STMT, expr(&tok, tok));
    *rest = skip(tok, ";");
    return node;
}

// expr = assign
Node *expr(Token **rest, Token *tok)
{
    return assign(rest, tok);
}

// assign = equality ("=" assign)?
Node *assign(Token **rest, Token *tok)
{
    Node *node = equality(&tok, tok);
    if (equal(tok, "="))
    {
        node = new_binary(ND_ASSIGN, node, assign(&tok, tok->next));
    }
    *rest = tok;
    return node;
}

// equality = relational ("==" relational | "!=" relational)*
Node *equality(Token **rest, Token *tok)
{
    Node *node = relational(&tok, tok);

    for (;;)
    {
        if (equal(tok, "=="))
        {
            node = new_binary(ND_EQ, node, relational(&tok, tok->next));
            continue;
        }

        if (equal(tok, "!="))
        {
            node = new_binary(ND_NE, node, relational(&tok, tok->next));
            continue;
        }

        *rest = tok;
        return node;
    }
}

// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
Node *relational(Token **rest, Token *tok)
{
    Node *node = add(&tok, tok);

    for (;;)
    {
        if (equal(tok, "<"))
        {
            node = new_binary(ND_LT, node, add(&tok, tok->next));
            continue;
        }
        if (equal(tok, "<="))
        {
            node = new_binary(ND_LE, node, add(&tok, tok->next));
            continue;
        }
        if (equal(tok, ">"))
        {
            node = new_binary(ND_LT, add(&tok, tok->next), node);
            continue;
        }
        if (equal(tok, ">="))
        {
            node = new_binary(ND_LE, add(&tok, tok->next), node);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// add = mul ("+" mul | "-" mul)*
Node *add(Token **rest, Token *tok)
{
    Node *node = mul(&tok, tok);

    for (;;)
    {
        if (equal(tok, "+"))
        {
            node = new_add(node, mul(&tok, tok->next), tok);
            continue;
        }
        if (equal(tok, "-"))
        {
            node = new_sub(node, mul(&tok, tok->next), tok);
            continue;
        }

        *rest = tok;
        return node;
    }
}

// mul = unary ("*" unary | "/" unary)*
Node *mul(Token **rest, Token *tok)
{
    Node *node = unary(&tok, tok);

    for (;;)
    {
        if (equal(tok, "*"))
        {
            node = new_binary(ND_MUL, node, unary(&tok, tok->next));
            continue;
        }
        if (equal(tok, "/"))
        {
            node = new_binary(ND_DIV, node, unary(&tok, tok->next));
            continue;
        }

        *rest = tok;
        return node;
    }
}

// unary = ("+" | "-" | "*" | "&") unary
//          | primary
Node *unary(Token **rest, Token *tok)
{
    if (equal(tok, "+"))
    {
        return unary(rest, tok->next);
    }
    if (equal(tok, "-"))
    {
        return new_unary(ND_NEG, unary(rest, tok->next));
    }
    if (equal(tok, "&"))
    {
        return new_unary(ND_ADDR, unary(rest, tok->next));
    }
    if (equal(tok, "*"))
    {
        return new_unary(ND_DEREF, unary(rest, tok->next));
    }

    return primary(rest, tok);
}

// funccall = ident "(" (assign ("," assign)*)? ")"
Node *funccall(Token **rest, Token *tok)
{
    Token *start = tok;
    tok = tok->next->next;

    Node head = {};
    Node *cur = &head;

    while (!equal(tok, ")"))
    {
        if (cur != &head)
        {
            tok = skip(tok, ",");
        }
        cur = cur->next = assign(&tok, tok);
    }

    *rest = skip(tok, ")");

    Node *node = new_node(ND_FUNCCALL);
    node->funcname = strndup(start->loc, start->len);
    node->args = head.next;
    return node;
}

// primary = "(" expr ")" | funccall | num
Node *primary(Token **rest, Token *tok)
{
    if (equal(tok, "("))
    {
        Node *node = expr(&tok, tok->next);
        *rest = skip(tok, ")");
        return node;
    }

    if (tok->kind == TK_IDENT)
    {
        // 関数呼び出し
        if (equal(tok->next, "("))
        {
            return funccall(rest, tok);
        }

        // 変数
        Obj *var = find_var(tok);
        if (!var)
        {
            error_tok(tok, "undefined variable");
        }
        Node *node = new_var(var);
        *rest = tok->next;
        return node;
    }

    if (tok->kind == TK_NUM)
    {
        Node *node = new_num(tok->val);
        *rest = tok->next;
        return node;
    }

    error_tok(tok, "expected an expression");
}

Node *new_add(Node *lhs, Node *rhs, Token *tok)
{
    add_type(lhs);
    add_type(rhs);

    // 数値 + 数値 の場合は、数値同士の足し算として扱う
    if (is_integer(lhs->ty) && is_integer(rhs->ty))
    {
        return new_binary(ND_ADD, lhs, rhs);
    }

    // 両方がポインタの場合はエラー
    if (lhs->ty->base && rhs->ty->base)
    {
        error_tok(tok, "invalid operands");
    }

    // 数値 + ポインタの場合は、ポインタが左オペランドになるように入れ替える
    // 計算結果はポインタ型になる
    if (!lhs->ty->base && rhs->ty->base)
    {
        Node *tmp = lhs;
        lhs = rhs;
        rhs = tmp;
    }

    // ポインタ + 数値の場合は、右オペランドをポインタのサイズ分*数値にする
    // MEMO: 実行時まで値がわからないので、コンパイル時には計算できない？？
    rhs = new_binary(ND_MUL, rhs, new_num(8));
    return new_binary(ND_ADD, lhs, rhs);
}

Node *new_sub(Node *lhs, Node *rhs, Token *tok)
{
    add_type(lhs);
    add_type(rhs);

    // 数値 - 数値 の場合は、数値同士の引き算として扱う
    if (is_integer(lhs->ty) && is_integer(rhs->ty))
    {
        return new_binary(ND_SUB, lhs, rhs);
    }

    if (lhs->ty->base && is_integer(rhs->ty))
    {
        rhs = new_binary(ND_MUL, rhs, new_num(8));
        add_type(rhs);
        Node *node = new_binary(ND_SUB, lhs, rhs);
        node->ty = lhs->ty;
        return node;
    }

    // ポインタ - ポインタ の場合は、間にある要素の数を計算する
    // 計算結果は数値型になる
    if (lhs->ty->base && rhs->ty->base)
    {
        Node *node = new_binary(ND_SUB, lhs, rhs);
        node->ty = ty_int;
        return new_binary(ND_DIV, node, new_num(8));
    }

    error_tok(tok, "invalid operands");
}

void create_param_lvars(Type *param)
{
    if (param)
    {
        create_param_lvars(param->next);
        new_lvar(get_ident(param->name), param);
    }
}

Function *function(Token **rest, Token *tok)
{
    Type *ty = declspec(&tok, tok);
    ty = declarator(&tok, tok, ty);

    // ローカル変数のリストを初期化
    locals = NULL;

    // functionを作成
    Function *fn = calloc(1, sizeof(Function));
    fn->name = get_ident(ty->name);
    // 引数を処理
    create_param_lvars(ty->params);
    fn->params = locals;

    // ブロックの中を読む
    tok = skip(tok, "{");
    fn->body = compound_stmt(rest, tok);
    fn->locals = locals;
    return fn;
}

// program = function*
Function *parse(Token *tok)
{
    Function head = {};
    Function *cur = &head;

    while (tok->kind != TK_EOF)
    {
        cur = cur->next = function(&tok, tok);
    }

    return head.next;
}