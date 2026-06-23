#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"
#include "grammar.h"
#include "lexer.h"
#include "symbol_table.h"
#include "parser.h"

// Protótipos das novas funções do motor de expressões
static void parse_E(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela);
static void parse_E_linha(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela);
static void parse_T(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela);
static void parse_T_linha(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela);
static void parse_F(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela);
static void parse_RelExpr(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela);
static void parse_RelExpr_linha(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela);
static void parse_symbol(Grammar *g, Symbol *s, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela);

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
// PARSER PREDITIVO RECURSIVO (ESTRUTURA PRINCIPAL)
// ============================================================
static void parse_symbol(Grammar *g, Symbol *s, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela) {
    if (s == NULL) return; 

    // 1. CASO O SÍMBOLO SEJA TERMINAL (MATCH)
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

    // 2. CASO O SÍMBOLO SEJA NÃO-TERMINAL DE CONTROLE (S)
    Token t = (*current_node != NULL) ? (*current_node)->token : (Token){ .type = TOKEN_SEPARATOR, .value.separator = SEMICOLON };
    
    if (strcmp(s->name, "S") == 0) {
        
        // Regra [1]: S -> print ( EXPRESSAO_OU_STRING ) ; S
        if (t.type == TOKEN_KEYWORD && t.value.keyword == PRINT) {
            Symbol t_print = { .name = "print", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_KEYWORD, {.keyword = PRINT} } };
            Symbol t_open  = { .name = "(",     .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = OPEN_PAREN} } };
            Symbol t_close = { .name = ")",     .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = CLOSE_PAREN} } };
            Symbol t_semi  = { .name = ";",     .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = SEMICOLON} } };

            parse_symbol(g, &t_print, current_node, arquivo_sam, tabela);
            parse_symbol(g, &t_open,  current_node, arquivo_sam, tabela);
            
            Token token_arg = (*current_node != NULL) ? (*current_node)->token : (Token){0};
            if (token_arg.type == TOKEN_SEPARATOR && token_arg.value.separator == DOUBLE_QUOTES) {
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
            } else {
                parse_RelExpr(g, current_node, arquivo_sam, tabela);
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
                fprintf(arquivo_sam, "PUSHIMM %d\n", node_val->token.value.literal.int_value);
                if (tabela->quantidade > 0) {
                    fprintf(arquivo_sam, "ADDSP -%d\n", tabela->quantidade);
                }
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

        // IMPLEMENTADO: Regra Nova para Constantes: S -> const int ID = RelExpr ; S
        else if (t.type == TOKEN_KEYWORD && t.value.keyword == CONST) {
            Symbol t_const = { .name = "const", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_KEYWORD, {.keyword = CONST} } };
            Symbol t_int = { .name = "int", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_KEYWORD, {.keyword = INT_KW} } };
            
            parse_symbol(g, &t_const, current_node, arquivo_sam, tabela);
            parse_symbol(g, &t_int, current_node, arquivo_sam, tabela);
            
            Token t_id_token = (*current_node)->token;
            char nome_const[32];
            strcpy(nome_const, t_id_token.value.identifier_name);
            
            // Registra como CONSTANTE legítima (true)
            adicionar_variavel(tabela, nome_const, true); 
            fprintf(arquivo_sam, "ADDSP 1\n");
            
            Symbol t_id = { .name = "ID", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_IDENTIFIER } };
            parse_symbol(g, &t_id, current_node, arquivo_sam, tabela);
            
            Symbol t_eq = { .name = "=", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = EQUAL} } };
            parse_symbol(g, &t_eq, current_node, arquivo_sam, tabela);
            
            parse_RelExpr(g, current_node, arquivo_sam, tabela);
            fprintf(arquivo_sam, "STOREOFF %d\n", tabela->quantidade - 1);
            
            Symbol t_semi = { .name = ";", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = SEMICOLON} } };
            parse_symbol(g, &t_semi, current_node, arquivo_sam, tabela);
            parse_symbol(g, s, current_node, arquivo_sam, tabela);
            return;
        }
        
        // Regra [3]: S -> int ID ; S  OU  int ID = RelExpr ; S
        else if (t.type == TOKEN_KEYWORD && t.value.keyword == INT_KW) {
            Symbol t_int = { .name = "int", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_KEYWORD, {.keyword = INT_KW} } };
            parse_symbol(g, &t_int, current_node, arquivo_sam, tabela);
            
            Token t_id_token = (*current_node)->token; 
            char nome_var[32];
            strcpy(nome_var, t_id_token.value.identifier_name);
            
            // Registra a variável comum (false)
            adicionar_variavel(tabela, nome_var, false); 
            fprintf(arquivo_sam, "ADDSP 1\n");
            
            Symbol t_id = { .name = "ID", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_IDENTIFIER } };
            parse_symbol(g, &t_id, current_node, arquivo_sam, tabela);
            
            Token proximo = (*current_node != NULL) ? (*current_node)->token : (Token){0};
            if (proximo.type == TOKEN_SEPARATOR && proximo.value.separator == EQUAL) {
                Symbol t_eq = { .name = "=", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = EQUAL} } };
                parse_symbol(g, &t_eq, current_node, arquivo_sam, tabela);
                
                parse_RelExpr(g, current_node, arquivo_sam, tabela);
                fprintf(arquivo_sam, "STOREOFF %d\n", tabela->quantidade - 1);
            }
            
            Symbol t_semi = { .name = ";", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = SEMICOLON} } };
            parse_symbol(g, &t_semi, current_node, arquivo_sam, tabela);
            parse_symbol(g, s, current_node, arquivo_sam, tabela);
            return;
        }
        
        // Regra [4]: S -> ID = RelExpr ; S  OU  S -> RelExpr ; S
        else if (t.type == TOKEN_IDENTIFIER) {
            char nome_var[32];
            strcpy(nome_var, t.value.identifier_name);
            
            TokenNode *proximo_no = (*current_node)->next;
            
            // Caso A: Atribuição Pura (ex: x = y == z;)
            if (proximo_no != NULL && proximo_no->token.type == TOKEN_SEPARATOR && proximo_no->token.value.separator == EQUAL) {
                
                // --- AJUSTE DE CONTROLE SEMÂNTICO DE CONSTANTE ---
                if (eh_constante(tabela, nome_var)) {
                    printf("[ERRO SEMANTICO]: Linha %d - Tentativa ilegal de reatribuir valor a constante '%s'!\n", t.line, nome_var);
                    exit(1);
                }

                int offset = buscar_offset(tabela, nome_var);
                if (offset == -1) {
                    printf("[ERRO SEMANTICO]: Linha %d - Variavel '%s' nao declarada!\n", t.line, nome_var);
                    exit(1);
                }

                Symbol t_id = { .name = "ID", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_IDENTIFIER } };
                Symbol t_eq = { .name = "=",  .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = EQUAL} } };

                parse_symbol(g, &t_id, current_node, arquivo_sam, tabela); 
                parse_symbol(g, &t_eq, current_node, arquivo_sam, tabela); 
                
                parse_RelExpr(g, current_node, arquivo_sam, tabela); 
                fprintf(arquivo_sam, "STOREOFF %d\n", offset);
            } 
            // Caso B: Expressão Relacional ou Matemática Pura Solta (ex: y == x;)
            else {
                parse_RelExpr(g, current_node, arquivo_sam, tabela);
                fprintf(arquivo_sam, "ADDSP -1"); 
            }
            
            Symbol t_semi = { .name = ";", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = SEMICOLON} } };
            parse_symbol(g, &t_semi, current_node, arquivo_sam, tabela);
            parse_symbol(g, s, current_node, arquivo_sam, tabela);
            return;
        }
        
        // Regra [5]: S -> epsilon
        else {
            return; 
        }
    }

    printf("[ERRO SINTATICO]: Linha %d - Token inesperado no contexto de '%s'.\n", t.line, s->name);
    exit(1);
}

// ============================================================
// MOTOR DE EXPRESSÕES ARITMÉTICAS (LL(1) PRECEDENTE)
// ============================================================

// E -> T E'
static void parse_E(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela) {
    parse_T(g, current_node, arquivo_sam, tabela);
    parse_E_linha(g, current_node, arquivo_sam, tabela);
}

// E' -> + T E' | - T E' | epsilon
static void parse_E_linha(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela) {
    if (*current_node != NULL && (*current_node)->token.type == TOKEN_SEPARATOR) {
        int op = (*current_node)->token.value.separator;
        
        if (op == SUM) {
            Symbol t_sum = { .name = "+", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = SUM} } };
            parse_symbol(g, &t_sum, current_node, arquivo_sam, tabela);
            parse_T(g, current_node, arquivo_sam, tabela);
            fprintf(arquivo_sam, "ADD\n");
            parse_E_linha(g, current_node, arquivo_sam, tabela);
        } 
        else if (op == SUB) {
            Symbol t_sub = { .name = "-", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = SUB} } };
            parse_symbol(g, &t_sub, current_node, arquivo_sam, tabela);
            parse_T(g, current_node, arquivo_sam, tabela);
            fprintf(arquivo_sam, "SUB\n");
            parse_E_linha(g, current_node, arquivo_sam, tabela);
        }
    }
}

// T -> F T'
static void parse_T(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela) {
    parse_F(g, current_node, arquivo_sam, tabela);
    parse_T_linha(g, current_node, arquivo_sam, tabela);
}

// T' -> * F T' | / F T' | % F T' | epsilon
static void parse_T_linha(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela) {
    if (*current_node != NULL && (*current_node)->token.type == TOKEN_SEPARATOR) {
        int op = (*current_node)->token.value.separator;
        
        if (op == MUL) {
            Symbol t_mul = { .name = "*", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = MUL} } };
            parse_symbol(g, &t_mul, current_node, arquivo_sam, tabela);
            parse_F(g, current_node, arquivo_sam, tabela);
            fprintf(arquivo_sam, "MUL\n");
            parse_T_linha(g, current_node, arquivo_sam, tabela);
        } 
        else if (op == DIV) {
            Symbol t_div = { .name = "/", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = DIV} } };
            parse_symbol(g, &t_div, current_node, arquivo_sam, tabela);
            parse_F(g, current_node, arquivo_sam, tabela);
            fprintf(arquivo_sam, "DIV\n");
            parse_T_linha(g, current_node, arquivo_sam, tabela);
        }
        else if (op == MOD) { 
            Symbol t_mod = { .name = "%", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = MOD} } };
            parse_symbol(g, &t_mod, current_node, arquivo_sam, tabela);
            parse_F(g, current_node, arquivo_sam, tabela);
            fprintf(arquivo_sam, "MOD\n");
            parse_T_linha(g, current_node, arquivo_sam, tabela);
        }
    }
}

// F -> ID | LIT_INT | LIT_FLOAT | ( E )
static void parse_F(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela) {
    if (*current_node == NULL) {
        printf("[ERRO SINTATICO]: Expressao incompleta no fim do arquivo.\n");
        exit(1);
    }

    Token t = (*current_node)->token;

    if (t.type == TOKEN_IDENTIFIER) {
        int offset = buscar_offset(tabela, t.value.identifier_name);
        if (offset == -1) {
            printf("[ERRO SEMANTICO]: Linha %d - Variavel '%s' nao declarada.\n", t.line, t.value.identifier_name);
            exit(1);
        }
        fprintf(arquivo_sam, "PUSHOFF %d\n", offset);
        Symbol t_id = { .name = "ID", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_IDENTIFIER } };
        parse_symbol(g, &t_id, current_node, arquivo_sam, tabela);
    } 
    else if (t.type == TOKEN_LITERAL && t.value.literal.tag == LIT_INT) {
        fprintf(arquivo_sam, "PUSHIMM %d\n", t.value.literal.int_value);
        Symbol t_lit = { .name = "LIT_INT", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_LITERAL, {.literal = INT} } };
        parse_symbol(g, &t_lit, current_node, arquivo_sam, tabela);
    } 
    else if (t.type == TOKEN_LITERAL && t.value.literal.tag == LIT_FLOAT) {
        fprintf(arquivo_sam, "PUSHIMMF %.2f\n", t.value.literal.float_value);
        Symbol t_lit = { .name = "LIT_FLOAT", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_LITERAL, {.literal = FLOAT} } };
        parse_symbol(g, &t_lit, current_node, arquivo_sam, tabela);
    }
    else if (t.type == TOKEN_SEPARATOR && t.value.separator == OPEN_PAREN) {
        Symbol t_open  = { .name = "(", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = OPEN_PAREN} } };
        Symbol t_close = { .name = ")", .type = SYMBOL_TERMINAL, .terminal_info = { TOKEN_SEPARATOR, {.separator = CLOSE_PAREN} } };
        
        parse_symbol(g, &t_open, current_node, arquivo_sam, tabela);
        parse_RelExpr(g, current_node, arquivo_sam, tabela); 
        parse_symbol(g, &t_close, current_node, arquivo_sam, tabela);
    } 
    else {
        printf("[ERRO SINTATICO]: Linha %d - Esperado identificador, numero ou '('.\n", t.line);
        exit(1);
    }
}

// RelExpr -> E RelExpr'
static void parse_RelExpr(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela) {
    parse_E(g, current_node, arquivo_sam, tabela);
    parse_RelExpr_linha(g, current_node, arquivo_sam, tabela);
}

// RelExpr' -> == E | != E | > E | < E | >= E | <= E | epsilon
static void parse_RelExpr_linha(Grammar *g, TokenNode **current_node, FILE *arquivo_sam, SymbolTable *tabela) {
    if (*current_node == NULL || (*current_node)->token.type != TOKEN_SEPARATOR) {
        return; 
    }

    int op = (*current_node)->token.value.separator;
    Symbol t_op;
    t_op.type = SYMBOL_TERMINAL;

    switch (op) {
        case EQ: 
            strcpy(t_op.name, "=="); 
            t_op.terminal_info.token_type = TOKEN_SEPARATOR;
            t_op.terminal_info.value.separator = EQ;
            
            parse_symbol(g, &t_op, current_node, arquivo_sam, tabela);
            parse_E(g, current_node, arquivo_sam, tabela);
            fprintf(arquivo_sam, "EQUAL\n");
            break;
            
        case NE: 
            strcpy(t_op.name, "!="); 
            t_op.terminal_info.token_type = TOKEN_SEPARATOR;
            t_op.terminal_info.value.separator = NE;
            
            parse_symbol(g, &t_op, current_node, arquivo_sam, tabela);
            parse_E(g, current_node, arquivo_sam, tabela);
            fprintf(arquivo_sam, "EQUAL\n");
            fprintf(arquivo_sam, "NOT\n"); 
            break;
            
        case GT: 
            strcpy(t_op.name, ">"); 
            t_op.terminal_info.token_type = TOKEN_SEPARATOR;
            t_op.terminal_info.value.separator = GT;
            
            parse_symbol(g, &t_op, current_node, arquivo_sam, tabela);
            parse_E(g, current_node, arquivo_sam, tabela);
            fprintf(arquivo_sam, "GREATER\n");
            break;
            
        case LT: 
            strcpy(t_op.name, "<"); 
            t_op.terminal_info.token_type = TOKEN_SEPARATOR;
            t_op.terminal_info.value.separator = LT;
            
            parse_symbol(g, &t_op, current_node, arquivo_sam, tabela);
            parse_E(g, current_node, arquivo_sam, tabela);
            fprintf(arquivo_sam, "LESS\n");
            break;
            
        case GE: 
            strcpy(t_op.name, ">="); 
            t_op.terminal_info.token_type = TOKEN_SEPARATOR;
            t_op.terminal_info.value.separator = GE;
            
            parse_symbol(g, &t_op, current_node, arquivo_sam, tabela);
            parse_E(g, current_node, arquivo_sam, tabela);
            fprintf(arquivo_sam, "LESS\n");
            fprintf(arquivo_sam, "NOT\n");
            break;
            
        case LE: 
            strcpy(t_op.name, "<="); 
            t_op.terminal_info.token_type = TOKEN_SEPARATOR;
            t_op.terminal_info.value.separator = LE;
            
            parse_symbol(g, &t_op, current_node, arquivo_sam, tabela);
            parse_E(g, current_node, arquivo_sam, tabela);
            fprintf(arquivo_sam, "GREATER\n");
            fprintf(arquivo_sam, "NOT\n");
            break;
            
        default:
            return; 
    }
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