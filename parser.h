// parser.h
#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include "grammar.h"
#include "lexer.h"

// Assinatura que a Main chama (continua idêntica na assinatura externa)
void program_parser(TokenNode *tokens_do_lexer, FILE *arquivo_sam, Grammar *g);

#endif