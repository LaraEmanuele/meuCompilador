#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

// Função principal que recebe a lista de tokens do Lexer e faz a análise sintática
void program_parser(TokenNode *tokens_do_lexer, FILE *output_file);

#endif