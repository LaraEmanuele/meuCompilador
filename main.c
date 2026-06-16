#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common.h"
#include "grammar.h"
#include "lexer.h"

int main() {
    printf("==================================================\n");
    printf("     INICIALIZANDO VALIDAÇÃO: GRAMMAR + LEXER     \n");
    printf("==================================================\n\n");

    // ============================================================
    // 1. CONFIGURAÇÃO DA GRAMÁTICA (RESTRITIVA)
    // ============================================================
    Grammar *g = grammar("S");
    Symbol *nt_s = g->start_symbol;

    // Cadastro dos Símbolos Terminais
    Symbol *t_exit  = add_terminal(g, "exit");
    Symbol *t_print = add_terminal(g, "print");
    Symbol *t_open  = add_terminal(g, "(");
    Symbol *t_num   = add_terminal(g, "NUMERO");
    Symbol *t_texto = add_terminal(g, "TEXTO");
    Symbol *t_close = add_terminal(g, ")");
    Symbol *t_semi  = add_terminal(g, ";");

    // Definição das Regras de Produção
    // Regra 1: S -> print ( TEXTO ) ; S
    Symbol *rhs_print[] = { t_print, t_open, t_texto, t_close, t_semi, nt_s };
    add_production(g, nt_s, rhs_print, 6);

    // Regra 2: S -> exit ( NUMERO ) ;
    Symbol *rhs_exit[] = { t_exit, t_open, t_num, t_close, t_semi };
    add_production(g, nt_s, rhs_exit, 5);
    
    // Regra 3: S -> ε (Epsilon / Vazio)
    add_production(g, nt_s, NULL, 0);

    // ============================================================
    // 2. VALIDAÇÃO MATEMÁTICA EXIGIDA PELO PROFESSOR
    // ============================================================
    // Chama a função que adicionaste no grammar.c para provar que é LL(1)
    if (!verificar_ll1(g)) {
        printf("Validacao abortada: A gramatica possui conflitos LL(1)!\n");
        return 1;
    }

    // ============================================================
    // 3. ANÁLISE LÉXICA (EXECUÇÃO DO TEU LEXER REAL)
    // ============================================================
    FILE *file = fopen("./test.unn", "r");
    if (file == NULL) {
        printf("Erro: Nao foi possivel abrir o arquivo 'test.unn'\n");
        return 1;
    }

    printf("A ler o ficheiro 'test.unn' e a gerar tokens...\n");
    // O Lexer varre o ficheiro e devolve a lista encadeada de tokens
    TokenNode *token_list = lexer(file);
    fclose(file);

    // Imprime o relatório visual dos tokens gerados na consola
    print_token_list(token_list);

    // ============================================================
    // 4. LIMPEZA DE MEMÓRIA
    // ============================================================
    free_token_list(token_list);
    
    printf(">> Execucao de testes terminada com sucesso.\n");
    return 0;
}