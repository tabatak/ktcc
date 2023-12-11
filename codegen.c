#include "ktcc.h"

static int depth;
static char *argregisters[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
static Function *current_func;

void gen_expr(Node *node);

static int count(void)
{
    static int i = 1;
    return i++;
}

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
    switch (node->kind)
    {
    case ND_VAR:
        printf("  lea rax, [rbp-%d]\n", node->var->offset);
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        return;
    }

    error("not an lvalue");
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
    case ND_NEG:
        gen_expr(node->lhs);
        printf("  neg rax\n");
        return;
    case ND_VAR:
        gen_addr(node);
        printf("  mov rax, [rax]\n");
        return;
    case ND_DEREF:
        gen_expr(node->lhs);
        printf("  mov rax, [rax]\n");
        return;
    case ND_ADDR:
        gen_addr(node->lhs);
        return;
    case ND_ASSIGN:
        gen_addr(node->lhs);
        push();
        gen_expr(node->rhs);
        pop("rdi");
        printf("  mov [rdi], rax\n");
        return;
    case ND_FUNCCALL:
    {
        int nargs = 0;
        for (Node *arg = node->args; arg; arg = arg->next)
        {
            gen_expr(arg);
            push();
            nargs++;
        }

        for (int i = nargs - 1; i >= 0; i--)
        {
            pop(argregisters[i]);
        }
        printf("  mov rax, 0\n");
        printf("  call %s\n", node->funcname);
        return;
    }
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
    switch (node->kind)
    {
    case ND_IF:
    {
        int c = count();
        gen_expr(node->cond);
        printf("  cmp rax, 0\n");
        printf("  je  .L.else.%d\n", c);
        gen_stmt(node->then);
        printf("  jmp .L.end.%d\n", c);
        printf(".L.else.%d:\n", c);
        if (node->els)
        {
            gen_stmt(node->els);
        }
        printf(".L.end.%d:\n", c);
        return;
    }
    case ND_FOR:
    {
        int c = count();
        if (node->init)
        {
            gen_stmt(node->init);
        }
        printf(".L.begin.%d:\n", c);
        if (node->cond)
        {
            gen_expr(node->cond);
            printf("  cmp rax, 0\n");
            printf("  je  .L.end.%d\n", c);
        }
        gen_stmt(node->then);
        if (node->inc)
        {
            gen_expr(node->inc);
        }
        printf("  jmp .L.begin.%d\n", c);
        printf(".L.end.%d:\n", c);
        return;
    }
    case ND_BLOCK:
        for (Node *n = node->body; n; n = n->next)
        {
            gen_stmt(n);
        }
        return;
    case ND_RETURN:
        gen_expr(node->lhs);
        printf("  jmp .L.return.%s\n", current_func->name);
        return;
    case ND_EXPR_STMT:
        gen_expr(node->lhs);
        return;
    }

    error("invalid statement");
}

void assign_lvar_offsets(Function *prog)
{
    for (Function *fn = prog; fn; fn = fn->next)
    {
        int offset = 0;
        for (Obj *var = fn->locals; var; var = var->next)
        {
            offset += 8;
            var->offset = offset;
        }
        fn->stack_size = align_to(offset, 16);
    }
}

void codegen(Function *prog)
{
    assign_lvar_offsets(prog);

    printf(".intel_syntax noprefix\n");

    for (Function *fn = prog; fn; fn = fn->next)
    {
        printf(".globl %s\n", fn->name);
        printf("%s:\n", fn->name);
        current_func = fn;

        // Prologue
        printf("  push rbp\n");
        printf("  mov rbp, rsp\n");
        printf("  sub rsp, %d\n", fn->stack_size);

        gen_stmt(fn->body);
        assert(depth == 0);

        // Epilogue
        printf(".L.return.%s:\n", fn->name);
        printf("  mov rsp, rbp\n");
        printf("  pop rbp\n");
        printf("  ret\n");
    }
}
