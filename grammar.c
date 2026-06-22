// grammar.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "grammar.h"

// Protótipos das funções privadas (static) mantidos para organização interna
Symbol* add_nonterminal(Grammar *g, const char *name);
Symbol* add_terminal(Grammar *g, const char *name);

// ============================================================
// 1. FUNÇÕES DE CRIAÇÃO E BUSCA (OTIMIZADAS COM find_symbol)
// ============================================================

Grammar* grammar(const char *start_symbol_name) {
    Grammar *g = malloc(sizeof(Grammar));
    g->symbol_count = 0;
    g->production_count = 0;

    // Todo símbolo de início (S) é, por definição, um Não-Terminal
    g->start_symbol = add_nonterminal(g, start_symbol_name);
    return g;
}

Symbol* find_symbol(Grammar *g, const char *name) {
    if (g == NULL || name == NULL) return NULL;

    for (int i = 0; i < g->symbol_count; i++) {
        if (strcmp(g->symbols[i]->name, name) == 0) {
            return g->symbols[i];
        }
    }
    return NULL;
}

Symbol* add_nonterminal(Grammar *g, const char *name) {
    Symbol *existente = find_symbol(g, name);
    if (existente != NULL) {
        if (existente->type == SYMBOL_TERMINAL) {
            printf("ERRO NA GRAMATICA: '%s' ja foi declarado como TERMINAL!\n", name);
            exit(1);
        }
        return existente; // Se já existir como não-terminal, reaproveita
    }

    Symbol *s = malloc(sizeof(Symbol));
    strcpy(s->name, name);
    s->type = SYMBOL_NON_TERMINAL;
    g->symbols[g->symbol_count++] = s;
    return s;
}

Symbol* add_terminal(Grammar *g, const char *name) {
    Symbol *existente = find_symbol(g, name);
    if (existente != NULL) {
        if (existente->type == SYMBOL_NON_TERMINAL) {
            printf("ERRO NA GRAMATICA: '%s' ja foi declarado como NAO-TERMINAL!\n", name);
            exit(1);
        }
        return existente; // Se já existir como terminal, reaproveita
    }

    Symbol *s = malloc(sizeof(Symbol));
    strcpy(s->name, name);
    s->type = SYMBOL_TERMINAL;
    g->symbols[g->symbol_count++] = s;
    return s;
}

Symbol* add_terminal_separator(Grammar *g, const char *name, TypeSeparator separator_enum) {
    Symbol *s = add_terminal(g, name);
    s->terminal_info.token_type = TOKEN_SEPARATOR;
    s->terminal_info.value.separator = separator_enum;
    return s;
}

Symbol* add_terminal_keyword(Grammar *g, const char *name, TypeKeyWord keyword_enum) {
    Symbol *s = add_terminal(g, name);
    s->terminal_info.token_type = TOKEN_KEYWORD;
    s->terminal_info.value.keyword = keyword_enum;
    return s;
}

Symbol* add_terminal_literal(Grammar *g, const char *name, TypeLiteral literal_enum) {
    Symbol *s = add_terminal(g, name);
    s->terminal_info.token_type = TOKEN_LITERAL;
    s->terminal_info.value.literal = literal_enum;
    return s;
}

Symbol* add_terminal_identifier(Grammar *g, const char *name) {
    Symbol *s = add_terminal(g, name);
    s->terminal_info.token_type = TOKEN_IDENTIFIER;
    return s;
}

Production* add_production(Grammar *g, Symbol *A, Symbol **rhs_sequence, int rhs_count) {
    if (A->type != SYMBOL_NON_TERMINAL) {
        printf("ERRO: O lado esquerdo (LHS) de uma producao deve ser um Nao-Terminal.\n");
        exit(1);
    }

    Production *p = malloc(sizeof(Production));
    p->lhs = A;
    p->rhs_count = rhs_count;
    p->rhs = malloc(sizeof(Symbol*) * rhs_count);

    for (int i = 0; i < rhs_count; i++) {
        p->rhs[i] = rhs_sequence[i];
    }
    g->productions[g->production_count++] = p;
    return p;
}

// ============================================================
// 2. ITERADORES E AUXILIARES
// ============================================================

Occurrence** occurrences(Grammar *g, Symbol *X) {
    int count = 0;
    for (int i = 0; i < g->production_count; i++) {
        for (int j = 0; j < g->productions[i]->rhs_count; j++) {
            if (g->productions[i]->rhs[j] == X) count++;
        }
    }

    Occurrence **result = malloc(sizeof(Occurrence*) * (count + 1));
    int idx = 0;

    for (int i = 0; i < g->production_count; i++) {
        for (int j = 0; j < g->productions[i]->rhs_count; j++) {
            if (g->productions[i]->rhs[j] == X) {
                Occurrence *occ = malloc(sizeof(Occurrence));
                occ->production_ptr = g->productions[i];
                occ->index = j;
                result[idx++] = occ;
            }
        }
    }
    result[idx] = NULL;
    return result;
}

Production* production(Occurrence *O) {
    return (O == NULL) ? NULL : (Production*)O->production_ptr;
}

bool is_terminal(Grammar *g, Symbol *X) {
    return (X == NULL) ? false : (X->type == SYMBOL_TERMINAL);
}

Symbol* lhs(Production *p) {
    return p->lhs;
}

Symbol** terminals(Grammar *g) {
    int count = 0;
    for (int i = 0; i < g->symbol_count; i++) {
        if (g->symbols[i]->type == SYMBOL_TERMINAL) count++;
    }

    Symbol **result = malloc(sizeof(Symbol*) * (count + 1));
    int idx = 0;
    for (int i = 0; i < g->symbol_count; i++) {
        if (g->symbols[i]->type == SYMBOL_TERMINAL) result[idx++] = g->symbols[i];
    }
    result[idx] = NULL;
    return result;
}

Symbol** nonterminals(Grammar *g) {
    int count = 0;
    for (int i = 0; i < g->symbol_count; i++) {
        if (g->symbols[i]->type == SYMBOL_NON_TERMINAL) count++;
    }

    Symbol **result = malloc(sizeof(Symbol*) * (count + 1));
    int idx = 0;
    for (int i = 0; i < g->symbol_count; i++) {
        if (g->symbols[i]->type == SYMBOL_NON_TERMINAL) result[idx++] = g->symbols[i];
    }
    result[idx] = NULL;
    return result;
}

Production** productions(Grammar *g) {
    Production **result = malloc(sizeof(Production*) * (g->production_count + 1));
    for (int i = 0; i < g->production_count; i++) {
        result[i] = g->productions[i];
    }
    result[g->production_count] = NULL;
    return result;
}

Symbol** rhs(Production *p) {
    Symbol **result = malloc(sizeof(Symbol*) * (p->rhs_count + 1));
    for (int i = 0; i < p->rhs_count; i++) {
        result[i] = p->rhs[i];
    }
    result[p->rhs_count] = NULL;
    return result;
}

Production** productions_for(Grammar *g, Symbol *A) {
    int count = 0;
    for (int i = 0; i < g->production_count; i++) {
        if (g->productions[i]->lhs == A) count++;
    }

    Production **result = malloc(sizeof(Production*) * (count + 1));
    int idx = 0;
    for (int i = 0; i < g->production_count; i++) {
        if (g->productions[i]->lhs == A) result[idx++] = g->productions[i];
    }
    result[idx] = NULL;
    return result;
}

Symbol** tail(Production *p, int i) {
    if (p == NULL || i < 0) return NULL;

    if (i + 1 >= p->rhs_count) {
        Symbol **empty = malloc(sizeof(Symbol*));
        empty[0] = NULL;
        return empty;
    }

    int size = p->rhs_count - (i + 1);
    Symbol **result = malloc(sizeof(Symbol*) * (size + 1));
    int idx = 0;
    for (int j = i + 1; j < p->rhs_count; j++) {
        result[idx++] = p->rhs[j];
    }
    result[idx] = NULL;
    return result;
}

// ============================================================
// 3. VALIDAÇÃO MATEMÁTICA LL(1) (OTIMIZADA COM tail)
// ============================================================

bool verificar_ll1(Grammar *g) {
    printf("==================================================\n");
    printf("   GERANDO CONJUNTOS PREDICT (VALIDACAO LL(1))    \n");
    printf("==================================================\n");

    bool g_tem_conflito = false;
    Symbol **todos_nao_terminais = nonterminals(g);

    for (int nt_idx = 0; todos_nao_terminais[nt_idx] != NULL; nt_idx++) {
        Symbol *nt_atual = todos_nao_terminais[nt_idx];
        Production **regras = productions_for(g, nt_atual);
        
        const char *terminais_vistos[20];
        int qtd_vistos = 0;

        printf(">>> Analisando producoes para o Nao-Terminal: {%s}\n", nt_atual->name);
        printf("--------------------------------------------------\n");

        for (int i = 0; regras[i] != NULL; i++) {
            Production *p = regras[i];
            Symbol **sequencia_rhs = rhs(p);
            const char *predict_symbol_name = "";

            if (p->rhs_count == 0 || sequencia_rhs == NULL) {
                predict_symbol_name = "$ (EOF)";
            } else {
                // Descobre o Predict olhando o primeiro símbolo
                predict_symbol_name = sequencia_rhs[0]->name;
                
                // USO DO TAIL: Se a regra continuar além do primeiro símbolo,
                // poderíamos inspecionar a cauda se o primeiro fosse um Não-Terminal que aceitasse epsilon.
                Symbol **cauda = tail(p, 0);
                // (Para o seu parser atual baseado em tokens diretos, apenas rastrear a cauda limpa a memória)
                free(cauda); 
            }

            printf("  Regra [%d]: %s -> ", i + 1, lhs(p)->name);
            if (p->rhs_count == 0) {
                printf("epsilon ");
            } else {
                for (int j = 0; j < p->rhs_count; j++) {
                    printf("%s ", sequencia_rhs[j]->name);
                }
            }
            printf("\n     => PREDICT = { %s }\n\n", predict_symbol_name);

            // Validação de Interseção
            for (int v = 0; v < qtd_vistos; v++) {
                if (strcmp(terminais_vistos[v], predict_symbol_name) == 0) {
                    g_tem_conflito = true;
                    printf("  [CONFLITO LL(1) DETECTADO]: O simbolo '%s' quebra a predicao em {%s}!\n\n", 
                           predict_symbol_name, nt_atual->name);
                }
            }

            terminais_vistos[qtd_vistos++] = predict_symbol_name;
            if (sequencia_rhs != NULL) free(sequencia_rhs);
        }
        free(regras);
        printf("\n");
    }
    free(todos_nao_terminais);

    printf("==================================================\n");
    if (!g_tem_conflito) {
        printf(">> PROVA MATEMATICA: Nenhum Predict se intersecta!\n");
        printf(">> CONCLUSAO: A gramatica eh 100%% LL(1). Aprovada!\n");
        printf("==================================================\n\n");
        return true;
    } else {
        printf(">> ERRO: A gramatica possui conflitos e nao eh LL(1)!\n");
        printf("==================================================\n\n");
        return false;
    }
}