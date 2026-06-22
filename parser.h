#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"


void parse_symbol(Grammar *g, Symbol *s, TokenNode **current_node, FILE *arquivo_sam);

void program_parser(TokenNode *tokens_do_lexer, FILE *arquivo_sam, Grammar *g);

#endif