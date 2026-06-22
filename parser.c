#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"
#include "grammar.h"
#include "lexer.h"
#include "symbol_table.h"
#include "parser.h"

// ============================================================
// AUXILIAR DE FILTRAGEM
// ============================================================
bool token_matches_symbol(Token t, Symbol *s) {
    if (s == NULL) return false;
    if (s->type != SYMBOL_TERMINAL) return false; 

    if ((int)t.type != (int)s->terminal_info.token_type) return false;

    switch (t.type) {
        case TOKEN_KEYWORD:
            return ((int)t.value.keyword == (int)s->terminal_info.value.keyword);
            
        case TOKEN_SEPARATOR:
            return ((int)t.value.separator == (int)s->terminal_info.value.separator);
            
        case TOKEN_LITERAL:
            if (s->terminal_info.value.literal == INT && t.value.literal.tag == LIT_INT) return true;
            if (s->terminal_info.value.literal == FLOAT && t.value.literal.tag == LIT_FLOAT) return true;
            if (s->terminal_info.value.literal == STRING && t.value.literal.tag == LIT_STRING) return true;
            return false;
            
        case TOKEN_IDENTIFIER:
            return true;

        default:
            return false;
    }
}

// ============================================================
// PARSER PREDITIVO RECURSIVO
// ============================================================
static void parse_symbol(Grammar *g, Symbol *s, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela) {
    if (s == NULL) return;

    // ============================================================
    // 1. CASO O SÍMBOLO SEJA TERMINAL (MATCH)
    // ============================================================
    if (s->type == SYMBOL_TERMINAL) {
        if (*current_node == NULL) {
            printf("[ERRO SINTATICO]: Linha Fim do Arquivo - Esperado terminal '%s', mas o arquivo acabou.\n", s->name);
            exit(1);
        }

        Token t = (*current_node)->token;
        if (token_matches_symbol(t, s)) {
            *current_node = (*current_node)->next;
            return;
        } else {
            printf("[ERRO SINTATICO]: Linha %d - Token inesperado. Esperava '%s', mas recebeu algo diferente.\n", t.line, s->name);
            exit(1);
        }
    }

    // ============================================================
    // 2. CASO O SÍMBOLO SEJA NÃO-TERMINAL (DECISÃO DIRETA LL(1))
    // ============================================================
    Token t = (*current_node != NULL) ? (*current_node)->token : (Token){ .type = TOKEN_SEPARATOR, .value.separator = SEMICOLON };

    // --- DECISÃO PARA O NÃO-TERMINAL "S" ---
    if (strcmp(s->name, "S") == 0) {
        
        // Regra [1]: S -> print ( ARG ) ; S
        if (t.type == TOKEN_KEYWORD && t.value.keyword == PRINT) {
            Symbol t_print = { .name = "print", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_KEYWORD, {.keyword = PRINT} } };
            Symbol t_open  = { .name = "(",     .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = OPEN_PAREN} } };
            Symbol t_close = { .name = ")",     .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = CLOSE_PAREN} } };
            Symbol t_semi  = { .name = ";",     .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = SEMICOLON} } };

            parse_symbol(g, &t_print, current_node, arquivo_sam, tabela);
            parse_symbol(g, &t_open,  current_node, arquivo_sam, tabela);
            
            // Espia se o argumento é String antes de processar
            Token token_arg = (*current_node != NULL) ? (*current_node)->token : (Token){0};
            bool eh_string = (token_arg.type == TOKEN_SEPARATOR && token_arg.value.separator == DOUBLE_QUOTES);
            
            // Executa o ARG (coloca o valor numérico na pilha ou chama o WRITESTR)
            parse_symbol(g, add_nonterminal(g, "ARG"), current_node, arquivo_sam, tabela);
            
            // Se NÃO for string, emite o WRITE para consumir o número/variável da pilha
            if (!eh_string) {
                fprintf(arquivo_sam, "WRITE\n");
            }
            
            parse_symbol(g, &t_close, current_node, arquivo_sam, tabela);
            parse_symbol(g, &t_semi,  current_node, arquivo_sam, tabela);
            
            parse_symbol(g, s, current_node, arquivo_sam, tabela);
            return;
        }
        
        // Regra [2]: S -> exit ( LIT_INT ) ;
        else if (t.type == TOKEN_KEYWORD && t.value.keyword == EXIT) {
            TokenNode *node_val = (*current_node)->next->next; 
            if (node_val != NULL && node_val->token.type == TOKEN_LITERAL) {
                // 1. Empilha o código de retorno do exit
                fprintf(arquivo_sam, "PUSHIMM %d\n", node_val->token.value.literal.int_value);
                
                // 2. Se houver variáveis alocadas na tabela, limpa a pilha mudando o SP de volta
                if (tabela->quantidade > 0) {
                    fprintf(arquivo_sam, "ADDSP -%d\n", tabela->quantidade);
                }
                
                // 3. Para a execução da máquina virtual
                fprintf(arquivo_sam, "STOP\n");
            }
            
            Symbol t_exit  = { .name = "exit", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_KEYWORD, {.keyword = EXIT} } };
            Symbol t_open  = { .name = "(",    .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = OPEN_PAREN} } };
            Symbol t_lit   = { .name = "LIT_INT", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_LITERAL, {.literal = INT} } };
            Symbol t_close = { .name = ")",    .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = CLOSE_PAREN} } };
            Symbol t_semi  = { .name = ";",    .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = SEMICOLON} } };

            parse_symbol(g, &t_exit,  current_node, arquivo_sam, tabela);
            parse_symbol(g, &t_open,  current_node, arquivo_sam, tabela);
            parse_symbol(g, &t_lit,   current_node, arquivo_sam, tabela);
            parse_symbol(g, &t_close, current_node, arquivo_sam, tabela);
            parse_symbol(g, &t_semi,  current_node, arquivo_sam, tabela);
            return;
        }
        
        // Regra [3]: S -> int ID ; S  OU  int ID = ARG ; S
        else if (t.type == TOKEN_KEYWORD && t.value.keyword == INT_KW) {
            TokenNode *node_id = (*current_node)->next;
            if (node_id != NULL && node_id->token.type == TOKEN_IDENTIFIER) {
                adicionar_variavel(tabela, node_id->token.value.identifier_name);
                fprintf(arquivo_sam, "ADDSP 1\n");
            }
            
            Symbol t_int = { .name = "int", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_KEYWORD, {.keyword = INT_KW} } };
            Symbol t_id  = { .name = "ID",  .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_IDENTIFIER } };
            
            parse_symbol(g, &t_int, current_node, arquivo_sam, tabela);
            parse_symbol(g, &t_id,  current_node, arquivo_sam, tabela);
            
            Token proximo = (*current_node != NULL) ? (*current_node)->token : (Token){0};
            if (proximo.type == TOKEN_SEPARATOR && proximo.value.separator == EQUAL) {
                Symbol t_eq = { .name = "=", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = EQUAL} } };
                parse_symbol(g, &t_eq, current_node, arquivo_sam, tabela);
                
                parse_symbol(g, add_nonterminal(g, "ARG"), current_node, arquivo_sam, tabela);
                fprintf(arquivo_sam, "STOREOFF %d\n", tabela->quantidade - 1);
            }
            
            Symbol t_semi = { .name = ";", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = SEMICOLON} } };
            parse_symbol(g, &t_semi, current_node, arquivo_sam, tabela);
            
            parse_symbol(g, s, current_node, arquivo_sam, tabela);
            return;
        }
        
        // Regra [4]: S -> ID = ARG ; S
        else if (t.type == TOKEN_IDENTIFIER) {
            char nome_var[32];
            strcpy(nome_var, t.value.identifier_name);
            
            int offset = buscar_offset(tabela, nome_var);
            if (offset == -1) {
                printf("[ERRO SEMANTICO]: Linha %d - Variavel '%s' nao declarada!\n", t.line, nome_var);
                exit(1);
            }
            
            Symbol t_id   = { .name = "ID", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_IDENTIFIER } };
            Symbol t_eq   = { .name = "=",  .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = EQUAL} } };
            Symbol t_semi = { .name = ";",  .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = SEMICOLON} } };

            parse_symbol(g, &t_id, current_node, arquivo_sam, tabela);
            parse_symbol(g, &t_eq, current_node, arquivo_sam, tabela);
            
            parse_symbol(g, add_nonterminal(g, "ARG"), current_node, arquivo_sam, tabela);
            
            fprintf(arquivo_sam, "STOREOFF %d\n", offset);
            
            parse_symbol(g, &t_semi, current_node, arquivo_sam, tabela);
            
            parse_symbol(g, s, current_node, arquivo_sam, tabela);
            return;
        }
        
        // Regra [5]: S -> epsilon
        else {
            return; 
        }
    }

    // --- DECISÃO PARA O NÃO-TERMINAL "ARG" ---
    if (strcmp(s->name, "ARG") == 0) {
        
        // Regra [1]: ARG -> LIT_INT
        if (t.type == TOKEN_LITERAL && t.value.literal.tag == LIT_INT) {
            fprintf(arquivo_sam, "PUSHIMM %d\n", t.value.literal.int_value);
            
            Symbol t_lit = { .name = "LIT_INT", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_LITERAL, {.literal = INT} } };
            parse_symbol(g, &t_lit, current_node, arquivo_sam, tabela);
            return;
        }
        
        // Regra [2]: ARG -> LIT_FLOAT
        else if (t.type == TOKEN_LITERAL && t.value.literal.tag == LIT_FLOAT) {
            fprintf(arquivo_sam, "PUSHIMMF %.2f\n", t.value.literal.float_value);
            
            Symbol t_lit = { .name = "LIT_FLOAT", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_LITERAL, {.literal = FLOAT} } };
            parse_symbol(g, &t_lit, current_node, arquivo_sam, tabela);
            return;
        }
        
        // Regra [3]: ARG -> DOUBLE_QUOTES LIT_STRING DOUBLE_QUOTES
        else if (t.type == TOKEN_SEPARATOR && t.value.separator == DOUBLE_QUOTES) {
            TokenNode *node_texto = (*current_node)->next;
            if (node_texto != NULL && node_texto->token.type == TOKEN_LITERAL && node_texto->token.value.literal.tag == LIT_STRING) {
                fprintf(arquivo_sam, "PUSHIMMSTR \"%s\"\n", node_texto->token.value.literal.string_value);
                fprintf(arquivo_sam, "WRITESTR\n");
            }
            
            Symbol t_quote = { .name = "\"", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = DOUBLE_QUOTES} } };
            Symbol t_str   = { .name = "LIT_STRING", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_LITERAL, {.literal = STRING} } };

            parse_symbol(g, &t_quote, current_node, arquivo_sam, tabela);
            parse_symbol(g, &t_str,   current_node, arquivo_sam, tabela);
            parse_symbol(g, &t_quote, current_node, arquivo_sam, tabela);
            return;
        }
        
        // Regra [4]: ARG -> ID
        else if (t.type == TOKEN_IDENTIFIER) {
            int offset = buscar_offset(tabela, t.value.identifier_name);
            if (offset == -1) {
                printf("[ERRO SEMANTICO]: Linha %d - Variavel '%s' nao declarada.\n", t.line, t.value.identifier_name);
                exit(1);
            }
            fprintf(arquivo_sam, "PUSHOFF %d\n", offset);
            
            Symbol t_id = { .name = "ID", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_IDENTIFIER } };
            parse_symbol(g, &t_id, current_node, arquivo_sam, tabela);
            return;
        }
    }

    printf("[ERRO SINTATICO]: Linha %d - Token inesperado no contexto de '%s'.\n", t.line, s->name);
    exit(1);
}

// ============================================================
// FUNÇÃO PÚBLICA DE ENTRADA
// ============================================================
void program_parser(TokenNode *tokens_do_lexer, FILE *arquivo_sam, Grammar *g) {
    TokenNode *current = tokens_do_lexer;
    
    SymbolTable tabela;
    tabela.quantidade = 0;
    
    parse_symbol(g, g->start_symbol, &current, arquivo_sam, &tabela);
    
    if (current != NULL) {
        printf("[ERRO SINTATICO]: O parser processou a gramatica, mas restam tokens no fim do arquivo.\n");
        exit(1);
    }
}