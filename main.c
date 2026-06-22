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
    
    Symbol *t_open  = add_terminal_separator(g, "OPEN_PAREN", OPEN_PAREN);
    Symbol *t_close = add_terminal_separator(g, "CLOSE_PAREN", CLOSE_PAREN);
    Symbol *t_semi  = add_terminal_separator(g, "SEMICOLON", SEMICOLON);
    Symbol *t_quote = add_terminal_separator(g, "DOUBLE_QUOTES", DOUBLE_QUOTES);

    Symbol *t_int   = add_terminal_literal(g, "LIT_INT", INT);
    Symbol *t_float = add_terminal_literal(g, "LIT_FLOAT", FLOAT);
    Symbol *t_str   = add_terminal_literal(g, "LIT_STRING", STRING);

    // Cadastro do Novo Símbolo Não-Terminal auxiliar para os argumentos
    Symbol *nt_arg  = add_nonterminal(g, "ARG"); 

    // ------ DEFINIÇÃO DAS REGRAS DE PRODUÇÃO ------

    // Regra 1: S -> print ( ARG ) ; S
    Symbol *rhs_print[] = { t_print, t_open, nt_arg, t_close, t_semi, nt_s };
    add_production(g, nt_s, rhs_print, 6);

    // Regra 2: S -> exit ( LIT_INT ) ;
    Symbol *rhs_exit[] = { t_exit, t_open, t_int, t_close, t_semi };
    add_production(g, nt_s, rhs_exit, 5);
    
    // Regra 3: S -> ε (Epsilon / Vazio)
    add_production(g, nt_s, NULL, 0);

    // Regras de Derivação do Argumento (ARG) para suportar múltiplos tipos:
    // ARG -> LIT_INT
    Symbol *rhs_arg_int[] = { t_int };
    add_production(g, nt_arg, rhs_arg_int, 1);

    // ARG -> LIT_FLOAT
    Symbol *rhs_arg_float[] = { t_float };
    add_production(g, nt_arg, rhs_arg_float, 1);

    // ARG -> DOUBLE_QUOTES LIT_STRING DOUBLE_QUOTES
    Symbol *rhs_arg_str[] = { t_quote, t_str, t_quote };
    add_production(g, nt_arg, rhs_arg_str, 3);


    // ============================================================
    // 2. VALIDAÇÃO MATEMÁTICA LL(1) ATUALIZADA
    // ============================================================
    // Agora varre S e ARG gerando o relatório completo para o professor
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
    TokenNode *token_list = lexer(file);
    fclose(file);

    // Imprime o relatório visual dos tokens gerados na consola
    print_token_list(token_list);

    // ============================================================
    // 4. ANÁLISE SINTÁTICA E GERAÇÃO DO CÓDIGO ASSEMBLY
    // ============================================================
    FILE *arquivo_sam = fopen("./resultado.sam", "w");
    if (arquivo_sam == NULL) {
        printf("Erro ao criar o ficheiro resultado.sam\n");
        free_token_list(token_list);
        return 1;
    }

    printf("============= INICIANDO ANALISE + GERACAO DE CODIGO =============\n");
    // Executa o Parser Preditivo unificado com a sua nova gramática
    program_parser(token_list, arquivo_sam, g);
    printf("=================================================================\n\n");

    // ============================================================
    // 5. LIMPEZA DE MEMÓRIA
    // ============================================================
    free_token_list(token_list);
    // Se tiver uma função free_grammar(g), lembre-se de chamá-la aqui.
    
    return 0;
}