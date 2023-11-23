#include "ktcc.h"

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "引数の個数が正しくありません\n");
        return 1;
    }
    fprintf(stderr, "input: %s\n", argv[1]);

    Token *tok = tokenize(argv[1]);
    Node *node = parse(tok);

    codegen(node);
    return 0;
}
