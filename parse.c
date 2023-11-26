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

Obj *new_lvar(char *name)
{
    Obj *var = calloc(1, sizeof(Obj));
    var->name = name;
    var->next = locals;
    locals = var;
    return var;
}

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

/*
    stmt = "return" expr ";"
                    | "{" compound-stmt
                    | expr-stmt
    compound-stmt = stmt* "}"
    expr-stmt = expr ";"
    expr = assign
    assign = equality ("=" assign)?
    equality = relational ("==" relational | "!=" relational)*
    relational = add ("<" add | "<=" add | ">" add | ">=" add)*
    add = mul ("+" mul | "-" mul)*
    mul = unary ("*" unary | "/" unary)*
    unary = ("+" | "-")? primary
    primary = num | ident | "(" expr ")"
*/

// stmt = "return" expr ";" | expr-stmt
Node *stmt(Token **rest, Token *tok)
{
    if (equal(tok, "return"))
    {
        Node *node = new_unary(ND_RETURN, expr(&tok, tok->next));
        *rest = skip(tok, ";");
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

// compound-stmt = stmt* "}"
Node *compound_stmt(Token **rest, Token *tok)
{
    Node head = {};
    Node *cur = &head;

    while (!equal(tok, "}"))
    {
        cur = cur->next = stmt(&tok, tok);
    }

    Node *node = new_node(ND_BLOCK);
    node->body = head.next;

    *rest = tok->next;
    return node;
}

// expr-stmt = expr ";"
Node *expr_stmt(Token **rest, Token *tok)
{
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
            node = new_binary(ND_ADD, node, mul(&tok, tok->next));
            continue;
        }
        if (equal(tok, "-"))
        {
            node = new_binary(ND_SUB, node, mul(&tok, tok->next));
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

// unary = ("+" | "-")? unary
Node *unary(Token **rest, Token *tok)
{
    if (equal(tok, "+"))
    {
        return primary(rest, tok->next);
    }
    if (equal(tok, "-"))
    {
        return new_binary(ND_SUB, new_num(0), unary(rest, tok->next));
    }
    return primary(rest, tok);
}

// primary = num | "(" expr ")"
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
        Obj *var = find_var(tok);
        if (!var)
        {
            var = new_lvar(strndup(tok->loc, tok->len));
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

Function *parse(Token *tok)
{
    tok = skip(tok, "{");

    Node head = {};
    Node *cur = &head;

    Function *prog = calloc(1, sizeof(Function));
    prog->body = compound_stmt(&tok, tok);
    prog->locals = locals;
    return prog;
    ;
}