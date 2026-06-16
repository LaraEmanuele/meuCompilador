// lexer.h
#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include "common.h" // Precisa conhecer o TokenNode

// Avisa para os outros arquivos que a função lexer existe
TokenNode* lexer(FILE *file);
void free_token_list(TokenNode *head);
void print_token_list(TokenNode *head);

#endif