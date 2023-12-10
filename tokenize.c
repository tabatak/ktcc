#include "ktcc.h"

static char *current_input;

// エラーを報告するための関数
// printfと同じ引数を取る
void error(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

// エラー箇所を報告する
void verror_at(char *loc, char *fmt, va_list ap)
{
    int pos = loc - current_input;
    fprintf(stderr, "%s\n", current_input);
    fprintf(stderr, "%*s", pos, ""); // pos個の空白を出力
    fprintf(stderr, "^ ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void error_at(char *loc, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    verror_at(loc, fmt, ap);
}

void error_tok(Token *tok, char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    verror_at(tok->loc, fmt, ap);
}

// Compares the value of a token with a given string.
bool equal(Token *tok, char *op)
{
    return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

// Consumes the current token if it matches the given string.
Token *skip(Token *tok, char *op)
{
    if (!equal(tok, op))
    {
        error_tok(tok, "'%s'ではありません", op);
    }
    return tok->next;
}

bool consume(Token **rest, Token *tok, char *str)
{
    if (equal(tok, str))
    {
        *rest = tok->next;
        return true;
    }

    *rest = tok;
    return false;
}

// Creates a new token.
Token *new_token(TokenKind kind, char *start, char *end)
{
    Token *tok = calloc(1, sizeof(Token));
    tok->kind = kind;
    tok->loc = start;
    tok->len = end - start;
    return tok;
}

// Checks if a string starts with another string.
bool startswith(char *p, char *q)
{
    return memcmp(p, q, strlen(q)) == 0;
}

// identifierの先頭文字として使えるかを判定する
bool is_ident1(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_';
}

// identifierの先頭文字以外の文字として使えるかを判定する
bool is_ident2(char c)
{
    return is_ident1(c) || ('0' <= c && c <= '9');
}

// 識別子に使用できる文字かを判定する
bool is_alnum(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_' || ('0' <= c && c <= '9');
}

// キーワードに使用できる文字列かを判定する
bool is_keyword(Token *tok)
{
    static char *kw[] = {"return", "if", "else", "for", "while", "int"};
    for (int i = 0; i < sizeof(kw) / sizeof(*kw); i++)
    {
        if (equal(tok, kw[i]))
        {
            return true;
        }
    }
    return false;
}

int read_punct(char *p)
{
    if (startswith(p, "==") || startswith(p, "!=") || startswith(p, "<=") || startswith(p, ">="))
    {
        return 2;
    }
    return ispunct(*p) ? 1 : 0;
}

void convert_keywords(Token *tok)
{
    for (Token *t = tok; t->kind != TK_EOF; t = t->next)
    {
        if (is_keyword(t))
        {
            t->kind = TK_KEYWORD;
        }
    }
}

// 入力文字列pをトークナイズしてそれを返す
Token *tokenize(char *p)
{
    current_input = p;

    Token head = {};
    Token *cur = &head;

    while (*p)
    {
        // 空白文字をスキップ
        if (isspace(*p))
        {
            p++;
            continue;
        }

        // 数字
        if (isdigit(*p))
        {
            cur = cur->next = new_token(TK_NUM, p, p);
            char *q = p;
            cur->val = strtol(p, &p, 10);
            cur->len = p - q;
            continue;
        }

        // 識別子 or キーワード
        if (is_ident1(*p))
        {
            char *q = p++;
            while (is_ident2(*p))
            {
                p++;
            }
            cur = cur->next = new_token(TK_IDENT, q, p);
            continue;
        }

        // Punctuators
        int punct_len = read_punct(p);
        if (punct_len)
        {
            cur = cur->next = new_token(TK_RESERVED, p, p + punct_len);
            p += punct_len;
            continue;
        }

        error_at(p, "トークナイズできません");
    }

    cur = cur->next = new_token(TK_EOF, p, p);
    convert_keywords(head.next);
    return head.next;
}
