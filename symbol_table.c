#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "symbol_table.h"
#include "common.h"

// Atualizado para receber o parâmetro is_constant
int adicionar_variavel(SymbolTable *tabela, const char *nome, bool is_constant) {
    // 1. Verifica se a variável já existe
    for (int i = 0; i < tabela->quantidade; i++) {
        if (strcmp(tabela->variaveis[i].name, nome) == 0) {
            // Erro Semântico: Redefinição de variável/constante no mesmo escopo
            printf("[ERRO SEMÂNTICO]: Variável ou Constante '%s' já foi declarada!\n", nome);
            return tabela->variaveis[i].offset;
        }
    }
    
    // Limite de segurança da tabela estática
    if (tabela->quantidade >= 100) {
        printf("[ERRO]: Tabela de símbolos cheia!\n");
        return -1;
    }

    int novo_offset = tabela->quantidade;
    
    // 2. Salva os dados do novo símbolo
    strcpy(tabela->variaveis[novo_offset].name, nome);
    tabela->variaveis[novo_offset].offset = novo_offset;
    tabela->variaveis[novo_offset].is_constant = is_constant; // <-- Guarda o tipo
    
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

// NOVA FUNÇÃO: Fundamental para a análise semântica no parser!
bool eh_constante(SymbolTable *tabela, const char *nome) {
    for (int i = 0; i < tabela->quantidade; i++) {
        if (strcmp(tabela->variaveis[i].name, nome) == 0) {
            return tabela->variaveis[i].is_constant;
        }
    }
    return false; // Se não achar, assume falso (o erro de não declarada será pego pelo buscar_offset)
}