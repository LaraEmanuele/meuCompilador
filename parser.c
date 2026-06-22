#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "grammar.h"
#include "lexer.h"
#include "parser.h"

// Função auxiliar para mapear se o Token casa com o nome do Símbolo Terminal
bool token_matches_symbol(Token t, Symbol *s) {
    if (s == NULL) return false;
    if (s->type != SYMBOL_TERMINAL) return false; 

    if (t.type != s->terminal_info.token_type) return false;

    switch (t.type) {
        case TOKEN_KEYWORD:
            return (t.value.keyword == s->terminal_info.value.keyword);
            
        case TOKEN_SEPARATOR:
            return (t.value.separator == s->terminal_info.value.separator);
            
        case TOKEN_LITERAL:
            if (s->terminal_info.value.literal == INT && t.value.literal.tag == LIT_INT) {
                return true;
            }
            if (s->terminal_info.value.literal == FLOAT && t.value.literal.tag == LIT_FLOAT) {
                return true;
            }
            if (s->terminal_info.value.literal == STRING && t.value.literal.tag == LIT_STRING) {
                return true;
            }
            return false;
            
        default:
            return false;
    }
}

// A FUNÇÃO MÁGICA: Executa a descida recursiva orientada por tabela e gera o SAMCODE
void parse_symbol(Grammar *g, Symbol *s, TokenNode **current_node, FILE *arquivo_sam) {
    if (s == NULL) return;

    // Caso a lista de tokens acabe mas o símbolo atual possa derivar em Epsilon, permitimos
    if (*current_node == NULL) {
        Production **regras = productions_for(g, s);
        bool aceita_epsilon = false;
        for (int i = 0; regras[i] != NULL; i++) {
            if (regras[i]->rhs_count == 0) aceita_epsilon = true;
        }
        free(regras);
        if (aceita_epsilon) return;

        printf("[ERRO SINTÁTICO]: O arquivo acabou antes do esperado. Faltou fechar comandos.\n");
        exit(1);
    }

    Token t = (*current_node)->token;

    // CASO 1: O símbolo que precisamos processar é um TERMINAL
    if (is_terminal(g, s)) {
        if (token_matches_symbol(t, s)) {
            *current_node = (*current_node)->next; // Avança o token globalmente
            return;
        } else {
            printf("[ERRO SINTÁTICO]: Linha %d - Esperado o terminal '%s'.\n", t.line, s->name);
            exit(1);
        }
    }

    // CASO 2: O símbolo é um NÃO-TERMINAL
    Production **regras = productions_for(g, s);
    Production *regra_escolhida = NULL;

    // Aplica o PREDICT dinamicamente para escolher qual regra abrir
    for (int i = 0; regras[i] != NULL; i++) {
        Symbol **sequencia_rhs = rhs(regras[i]);
        
        if (regras[i]->rhs_count == 0 || sequencia_rhs == NULL) {
            if (sequencia_rhs != NULL) free(sequencia_rhs);
            // LINHA CORRIGIDA:
            if (regra_escolhida == NULL) regra_escolhida = regras[i];// Epsilon fallback
            continue;
        }

        if (token_matches_symbol(t, sequencia_rhs[0])) {
            regra_escolhida = regras[i];
            free(sequencia_rhs);
            break;
        }
        free(sequencia_rhs);
    }

    free(regras);

    if (regra_escolhida == NULL) {
        printf("[ERRO SINTÁTICO]: Linha %d - Comando inesperado pela gramática.\n", t.line);
        exit(1);
    }

    // Se a regra escolhida for Epsilon (vazia), não faz nada e retorna
    if (regra_escolhida->rhs_count == 0) return;

    // ============================================================
    // GERAÇÃO DE CÓDIGO (INTERCEPTAÇÃO TRADUZIDA PARA O SAM REAL)
    // ============================================================
    
    // Se o não-terminal atual for o "ARG" que encapsula os literais
    if (strcmp(s->name, "ARG") == 0) {
        TokenNode *cursor_local = *current_node;
        
        // Se houver aspa de abertura, avançamos para pegar a string real de dentro
        if (t.type == TOKEN_SEPARATOR && t.value.separator == DOUBLE_QUOTES) {
            cursor_local = cursor_local->next;
        }

        Token token_valor = cursor_local->token;

        if (token_valor.type == TOKEN_LITERAL) {
            switch (token_valor.value.literal.tag) {
                case LIT_INT:
                    fprintf(arquivo_sam, "PUSHIMM %d\n", token_valor.value.literal.int_value);
                    fprintf(arquivo_sam, "WRITE\n");
                    break;
                case LIT_FLOAT:
                    fprintf(arquivo_sam, "PUSHIMMF %.2f\n", token_valor.value.literal.float_value);
                    fprintf(arquivo_sam, "WRITEF\n"); 
                    break;
                case LIT_STRING:
                    fprintf(arquivo_sam, "PUSHIMMSTR \"%s\"\n", token_valor.value.literal.string_value);
                    fprintf(arquivo_sam, "WRITESTR\n"); 
                    break;
            }
        }
    }
    
    // Se a regra escolhida capturar o encerramento 'exit(valor);'
    if (strcmp(s->name, "S") == 0 && t.type == TOKEN_KEYWORD && t.value.keyword == EXIT) {
        // Salta duas posições para capturar o inteiro: exit -> ( -> LIT_INT
        TokenNode *cursor_local = (*current_node)->next->next;
        if (cursor_local != NULL && cursor_local->token.type == TOKEN_LITERAL) {
            fprintf(arquivo_sam, "PUSHIMM %d\n", cursor_local->token.value.literal.int_value);
            fprintf(arquivo_sam, "STOP\n");
        }
    }

    // DESCIDA RECURSIVA AUTOMATIZADA:
    Symbol **corpo_regra = rhs(regra_escolhida);
    for (int j = 0; j < regra_escolhida->rhs_count; j++) {
        parse_symbol(g, corpo_regra[j], current_node, arquivo_sam); 
    }
    free(corpo_regra);
}

// Função principal disparada pela sua main.c
void program_parser(TokenNode *tokens_do_lexer, FILE *arquivo_sam, Grammar *g) {
    TokenNode *lexer_cursor = tokens_do_lexer;

    // Dispara a árvore preditiva a partir do Símbolo Inicial (g->start_symbol)
    parse_symbol(g, g->start_symbol, &lexer_cursor, arquivo_sam);

    printf("\n>> [Parser + CodeGen]: Sintaxe validada e 'resultado.sam' gravado com sucesso!\n");
}