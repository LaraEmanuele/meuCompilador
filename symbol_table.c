#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "symbol_table.h"
#include "common.h"

int adicionar_variavel(SymbolTable *tabela, const char *nome) {
    // Se a variável já existe, apenas retorna o offset atual
    for (int i = 0; i < tabela->quantidade; i++) {
        if (strcmp(tabela->variaveis[i].name, nome) == 0) {
            return tabela->variaveis[i].offset;
        }
    }
    
    // Armazena o índice atual como offset antes de incrementar
    int novo_offset = tabela->quantidade;
    
    strcpy(tabela->variaveis[novo_offset].name, nome);
    tabela->variaveis[novo_offset].offset = novo_offset;
    
    tabela->quantidade++;
    return novo_offset;
}

int buscar_offset(SymbolTable *tabela, const char *nome) {
    for (int i = 0; i < tabela->quantidade; i++) {
        if (strcmp(tabela->variaveis[i].name, nome) == 0) {
            return tabela->variaveis[i].offset;
        }
    }
    return -1;
}