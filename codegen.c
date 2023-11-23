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

int align_to(int n, int align)
{
    return (n + align - 1) & ~(align - 1);
}

void gen_addr(Node *node)
{
    if (node->kind == ND_VAR)
    {
        printf("  lea rax, [rbp-%d]\n", node->var->offset);
        return;
    }
}

/**
 * Generates code for the given node.
 *
 * @param node The node to generate code for.
 */
void gen_expr(Node *node)
{
    switch (node->kind)
    {
    case ND_NUM:
        printf("  mov rax, %d\n", node->val);
        return;
    case ND_VAR:
        gen_addr(node);
        printf("  mov rax, [rax]\n");
        return;
    case ND_ASSIGN:
        gen_addr(node->lhs);
        push();
        gen_expr(node->rhs);
        pop("rdi");
        printf("  mov [rdi], rax\n");
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

void assign_lvar_offsets(Function *prog)
{
    int offset = 0;
    for (Obj *var = prog->locals; var; var = var->next)
    {
        offset += 8;
        var->offset = offset;
    }
    prog->stack_size = offset; // 複数文字のスタックサイズはアラインしなくても大丈夫そう
}

void codegen(Function *prog)
{
    assign_lvar_offsets(prog);

    printf(".intel_syntax noprefix\n");
    printf(".globl main\n");
    printf("main:\n");

    // Prologue
    printf("  push rbp\n");
    printf("  mov rbp, rsp\n");
    printf("  sub rsp, %d\n", prog->stack_size);

    for (Node *n = prog->body; n; n = n->next)
    {
        gen_stmt(n);
    }

    // Epilogue
    printf("  mov rsp, rbp\n");
    printf("  pop rbp\n");
    printf("  ret\n");
}
