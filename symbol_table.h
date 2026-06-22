#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <string.h>
#include <stdio.h>
#include "common.h"

int adicionar_variavel(SymbolTable *tabela, const char *nome);
int buscar_offset(SymbolTable *tabela, const char *nome);

#endif