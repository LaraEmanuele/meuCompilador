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
    // 1. CONFIGURAÇÃO DA GRAMÁTICA (FATORADA E SEM CONFLITOS LL(1))
    // ============================================================
    Grammar *g = grammar("S");
    Symbol *nt_s = g->start_symbol;

    // Cadastro dos Símbolos Terminais usando as funções especialistas
    Symbol *t_exit  = add_terminal_keyword(g, "exit", EXIT);
    Symbol *t_print = add_terminal_keyword(g, "print", PRINT);
    Symbol *t_int_keyword = add_terminal_keyword(g, "int", INT_KW);
    Symbol *t_const_keyword = add_terminal_keyword(g, "const", CONST);
    
    Symbol *t_open  = add_terminal_separator(g, "OPEN_PAREN", OPEN_PAREN);
    Symbol *t_close = add_terminal_separator(g, "CLOSE_PAREN", CLOSE_PAREN);
    Symbol *t_semi  = add_terminal_separator(g, "SEMICOLON", SEMICOLON);
    Symbol *t_quote = add_terminal_separator(g, "DOUBLE_QUOTES", DOUBLE_QUOTES);
    Symbol *t_assign = add_terminal_separator(g, "ASSIGN", EQUAL);

    // Operadores relacionais e aritméticos
    Symbol *t_le    = add_terminal_separator(g, "<=", LE);
    Symbol *t_ge    = add_terminal_separator(g, ">=", GE);
    Symbol *t_lt    = add_terminal_separator(g, "<", LT);
    Symbol *t_gt    = add_terminal_separator(g, ">", GT);
    Symbol *t_eq    = add_terminal_separator(g, "==", EQ);
    Symbol *t_ne    = add_terminal_separator(g, "!=", NE);
    Symbol *t_sum   = add_terminal_separator(g, "+", SUM);
    Symbol *t_sub   = add_terminal_separator(g, "-", SUB);
    Symbol *t_mul   = add_terminal_separator(g, "*", MUL);
    Symbol *t_div   = add_terminal_separator(g, "/", DIV);
    Symbol *t_mod   = add_terminal_separator(g, "%", MOD);

    Symbol *t_int   = add_terminal_literal(g, "LIT_INT", INT);
    Symbol *t_float = add_terminal_literal(g, "LIT_FLOAT", FLOAT);
    Symbol *t_str   = add_terminal_literal(g, "LIT_STRING", STRING);
    Symbol *t_id    = add_terminal_identifier(g, "ID");

    // Cadastro dos Símbolos Não-Terminais
    Symbol *nt_relexpr     = add_nonterminal(g, "RelExpr");
    Symbol *nt_e           = add_nonterminal(g, "E");
    Symbol *nt_t           = add_nonterminal(g, "T");
    Symbol *nt_f           = add_nonterminal(g, "F");
    Symbol *nt_print_corpo = add_nonterminal(g, "PrintCorpo"); // Novo auxiliar para fatoração

    // ------ DEFINIÇÃO REGRAS DO NÃO-TERMINAL DE CONTROLE (S) ------

    // S -> print ( PrintCorpo
    Symbol *rhs_print_fatorado[] = { t_print, t_open, nt_print_corpo };
    add_production(g, nt_s, rhs_print_fatorado, 3);

    // S -> exit ( LIT_INT ) ;
    Symbol *rhs_exit[] = { t_exit, t_open, t_int, t_close, t_semi };
    add_production(g, nt_s, rhs_exit, 5);

    // S -> int ID = RelExpr ; S
    Symbol *rhs_decl_atrib[] = { t_int_keyword, t_id, t_assign, nt_relexpr, t_semi, nt_s };
    add_production(g, nt_s, rhs_decl_atrib, 6);

    // S -> const int ID = RelExpr ; S
    Symbol *rhs_const[] = { t_const_keyword, t_int_keyword, t_id, t_assign, nt_relexpr, t_semi, nt_s };
    add_production(g, nt_s, rhs_const, 7);

    // S -> ID = RelExpr ; S
    Symbol *rhs_assign[] = { t_id, t_assign, nt_relexpr, t_semi, nt_s };
    add_production(g, nt_s, rhs_assign, 5);

    // S -> Epsilon
    add_production(g, nt_s, NULL, 0);


    // ------ REGRAS DO CORPO DO PRINT (RESOLVE CONFLITO DO PRINT) ------
    
    // PrintCorpo -> DOUBLE_QUOTES LIT_STRING DOUBLE_QUOTES ) ; S
    Symbol *rhs_corpo_str[] = { t_quote, t_str, t_quote, t_close, t_semi, nt_s };
    add_production(g, nt_print_corpo, rhs_corpo_str, 6);

    // PrintCorpo -> RelExpr ) ; S
    Symbol *rhs_corpo_expr[] = { nt_relexpr, t_close, t_semi, nt_s };
    add_production(g, nt_print_corpo, rhs_corpo_expr, 4);


    // ------ REGRAS DE EXPRESSÃO LINEARES (EVITA CONFLITOS DE PREFIXO COMUM) ------
    
    // RelExpr -> E
    Symbol *rhs_rel_base[] = { nt_e };
    add_production(g, nt_relexpr, rhs_rel_base, 1);

    // E -> T
    Symbol *rhs_e_base[] = { nt_t };
    add_production(g, nt_e, rhs_e_base, 1);

    // T -> F
    Symbol *rhs_t_base[] = { nt_f };
    add_production(g, nt_t, rhs_t_base, 1);

    // F -> ID | LIT_INT | LIT_FLOAT | ( RelExpr )
    Symbol *rhs_f_id[] = { t_id };
    add_production(g, nt_f, rhs_f_id, 1);
    
    Symbol *rhs_f_int[] = { t_int };
    add_production(g, nt_f, rhs_f_int, 1);
    
    Symbol *rhs_f_float[] = { t_float };
    add_production(g, nt_f, rhs_f_float, 1);
    
    Symbol *rhs_f_paren[] = { t_open, nt_relexpr, t_close };
    add_production(g, nt_f, rhs_f_paren, 3);


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
    TokenNode *token_list = lexer(file);
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
    program_parser(token_list, arquivo_sam, g);
    fclose(arquivo_sam);
    printf("=================================================================\n\n");

    // ============================================================
    // 5. LIMPEZA DE MEMÓRIA
    // ============================================================
    free_token_list(token_list);
    
    printf("Compilacao concluida com sucesso!\n");
    return 0;
}