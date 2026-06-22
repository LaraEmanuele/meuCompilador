// main.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "common.h"
#include "grammar.h"
#include "lexer.h"
#include "parser.h" 

int main() {
    printf("==================================================\n");
    printf("     INICIALIZANDO VALIDAÇÃO: GRAMMAR + LEXER     \n");
    printf("==================================================\n\n");

    // ============================================================
    // 1. CONFIGURAÇÃO DA GRAMÁTICA (UNIFICADA COM ENUMS)
    // ============================================================
    Grammar *g = grammar("S");
    Symbol *nt_s = g->start_symbol;

    // Cadastro dos Símbolos Terminais usando as funções especialistas
    Symbol *t_exit  = add_terminal_keyword(g, "exit", EXIT);
    Symbol *t_print = add_terminal_keyword(g, "print", PRINT);
    Symbol *t_int_keyword = add_terminal_keyword(g, "int", INT_KW);
    
    Symbol *t_open  = add_terminal_separator(g, "OPEN_PAREN", OPEN_PAREN);
    Symbol *t_close = add_terminal_separator(g, "CLOSE_PAREN", CLOSE_PAREN);
    Symbol *t_semi  = add_terminal_separator(g, "SEMICOLON", SEMICOLON);
    Symbol *t_quote = add_terminal_separator(g, "DOUBLE_QUOTES", DOUBLE_QUOTES);
    Symbol *t_assign = add_terminal_separator(g, "ASSIGN", EQUAL);

    Symbol *t_int   = add_terminal_literal(g, "LIT_INT", INT);
    Symbol *t_float = add_terminal_literal(g, "LIT_FLOAT", FLOAT);
    Symbol *t_str   = add_terminal_literal(g, "LIT_STRING", STRING);

    Symbol *t_id = add_terminal_identifier(g, "ID");

    // Cadastro do Novo Símbolo Não-Terminal auxiliar para os argumentos
    Symbol *nt_arg  = add_nonterminal(g, "ARG"); 

    // ------ DEFINIÇÃO DAS REGRAS DE PRODUÇÃO ------

    // --- REGRA 1: S -> print ( ARG ) ; S
    Symbol *rhs_print[] = { t_print, t_open, nt_arg, t_close, t_semi, nt_s };
    add_production(g, nt_s, rhs_print, 6);

    // --- REGRA 2: S -> exit ( LIT_INT ) ;
    Symbol *rhs_exit[] = { t_exit, t_open, t_int, t_close, t_semi };
    add_production(g, nt_s, rhs_exit, 5);

    // --- REGRA 3: S -> int ID ; S
    Symbol *rhs_decl[] = { t_int_keyword, t_id, t_semi, nt_s };
    add_production(g, nt_s, rhs_decl, 4);

    // --- REGRA 4: S -> ID = ARG ; S
    Symbol *rhs_assign[] = { t_id, t_assign, nt_arg, t_semi, nt_s };
    add_production(g, nt_s, rhs_assign, 5);

    // --- REGRA 5: S -> Epsilon
    add_production(g, nt_s, NULL, 0);

    // Regras de Derivação do Argumento (ARG)
    // ARG -> LIT_INT
    Symbol *rhs_arg_int[] = { t_int };
    add_production(g, nt_arg, rhs_arg_int, 1);

    // ARG -> LIT_FLOAT
    Symbol *rhs_arg_float[] = { t_float };
    add_production(g, nt_arg, rhs_arg_float, 1);

    // ARG -> DOUBLE_QUOTES LIT_STRING DOUBLE_QUOTES
    Symbol *rhs_arg_str[] = { t_quote, t_str, t_quote };
    add_production(g, nt_arg, rhs_arg_str, 3);

    // ARG -> ID
    Symbol *rhs_arg_id[] = { t_id };
    add_production(g, nt_arg, rhs_arg_id, 1);


    // ============================================================
    // 2. VALIDAÇÃO MATEMÁTICA LL(1) ATUALIZADA
    // ============================================================
    if (!verificar_ll1(g)) {
        printf("Validacao abortada: A gramatica possui conflitos LL(1)!\n");
        return 1;
    }

    // ============================================================
    // 3. ANÁLISE LÉXICA (EXECUÇÃO DO SEU LEXER REAL)
    // ============================================================
    FILE *file = fopen("./test.unn", "r");
    if (file == NULL) {
        printf("Erro: Nao foi possivel abrir o arquivo 'test.unn'\n");
        return 1;
    }

    printf("A ler o ficheiro 'test.unn' e a gerar tokens...\n");
    TokenNode *token_list = lexer(file); // CORREÇÃO: Declarado e executado antes do Parser!
    fclose(file);

    // Imprime o relatório visual dos tokens gerados na consola
    print_token_list(token_list);

    // ============================================================
    // 4. ANÁLISE SINTÁTICA E GERAÇÃO DO CÓDIGO ASSEMBLY
    // ============================================================
    FILE *arquivo_sam = fopen("./resultado.sam", "w");
    if (arquivo_sam == NULL) {
        printf("Erro ao criar o ficheiro resultado.sam\n");
        free_token_list(token_list);
        return 1;
    }

    printf("============= INICIANDO ANALISE + GERACAO DE CODIGO =============\n");
    // Executa o Parser Preditivo passando a lista populada e o arquivo válido
    program_parser(token_list, arquivo_sam, g);
    fclose(arquivo_sam); // Lembrar de fechar o arquivo gerado
    printf("=================================================================\n\n");

    // ============================================================
    // 5. LIMPEZA DE MEMÓRIA
    // ============================================================
    free_token_list(token_list);
    
    printf("Compilacao concluida com sucesso!\n");
    return 0;
}