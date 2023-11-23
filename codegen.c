#include "ktcc.h"

int depth;

void push(void)
{
    printf("  push rax\n");
    depth++;
}

void pop(char *arg)
{
    printf("  pop %s\n", arg);
    depth--;
}

/**
 * Generates code for the given node.
 *
 * @param node The node to generate code for.
 */
void gen_expr(Node *node)
{
    if (node->kind == ND_NUM)
    {
        printf("  mov rax, %d\n", node->val);
        return;
    }

    gen_expr(node->lhs);
    push();
    gen_expr(node->rhs);
    push();

    pop("rdi");
    pop("rax");

    switch (node->kind)
    {
    case ND_ADD:
        printf("  add rax, rdi\n");
        break;
    case ND_SUB:
        printf("  sub rax, rdi\n");
        break;
    case ND_MUL:
        printf("  imul rax, rdi\n");
        break;
    case ND_DIV:
        printf("  cqo\n");
        printf("  idiv rdi\n");
        break;
    case ND_EQ:
        printf("  cmp rax, rdi\n");
        printf("  sete al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_NE:
        printf("  cmp rax, rdi\n");
        printf("  setne al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LT:
        printf("  cmp rax, rdi\n");
        printf("  setl al\n");
        printf("  movzb rax, al\n");
        break;
    case ND_LE:
        printf("  cmp rax, rdi\n");
        printf("  setle al\n");
        printf("  movzb rax, al\n");
        break;
    }
}

void gen_stmt(Node *node)
{
    if (node->kind == ND_EXPR_STMT)
    {
        gen_expr(node->lhs);
        assert(depth == 0);
        return;
    }
    error("invalid statement");
}

void codegen(Node *node)
{
    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    for (Node *n = node; n; n = n->next)
    {
        gen_stmt(n);
    }

    // スタックトップに式全体の値が残っているはずなので
    // それをRAXにロードして関数からの返り値とする
    printf("  ret\n");
}
