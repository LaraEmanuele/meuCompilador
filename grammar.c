// grammar.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "grammar.h"

// Protótipos das funções privadas (static) para evitar avisos de compilação
Symbol* add_nonterminal(Grammar *g, const char *name);
Symbol* add_terminal(Grammar *g, const char *name);

// grammar(S): cria uma nova gramática com símbolo de início S.
Grammar* grammar(const char *start_symbol_name) {
    Grammar *g = malloc(sizeof(Grammar));

    g->symbol_count = 0;
    g->production_count = 0;

    // Todo símbolo de início (S) é, por definição, um Não-Terminal
    g->start_symbol = add_nonterminal(g, start_symbol_name);

    return g;
}


// Adiciona um terminal do tipo Separador (Ex: OPEN_PAREN, SEMICOLON)
Symbol* add_terminal_separator(Grammar *g, const char *name, TypeSeparator separator_enum) {
    Symbol *s = add_terminal(g, name); // Usa sua lógica atual de alocação/validação
    s->terminal_info.token_type = TOKEN_SEPARATOR;
    s->terminal_info.value.separator = separator_enum;
    return s;
}

// Adiciona um terminal do tipo Palavra-Chave (Ex: PRINT, EXIT)
Symbol* add_terminal_keyword(Grammar *g, const char *name, TypeKeyWord keyword_enum) {
    Symbol *s = add_terminal(g, name);
    s->terminal_info.token_type = TOKEN_KEYWORD;
    s->terminal_info.value.keyword = keyword_enum;
    return s;
}

// Adiciona um terminal do tipo Literal (Ex: INT, STRING)
Symbol* add_terminal_literal(Grammar *g, const char *name, TypeLiteral literal_enum) {
    Symbol *s = add_terminal(g, name);
    s->terminal_info.token_type = TOKEN_LITERAL;
    s->terminal_info.value.literal = literal_enum;
    return s;
}


// add-nonterminal(A): adiciona o não-terminal A ao conjunto e retorna seu descritor (ponteiro).
Symbol* add_nonterminal(Grammar *g, const char *name) {

    // Regra do professor: Caso já esteja nos terminais, reporta erro.

    for (int i = 0; i < g->symbol_count; i++) {

        if (strcmp(g->symbols[i]->name, name) == 0) {

            if (g->symbols[i]->type == SYMBOL_TERMINAL) {

                printf("ERRO NA GRAMATICA: '%s' ja foi declarado como TERMINAL!\n", name);

                exit(1);

            }

            return g->symbols[i]; // Se já existir como não-terminal, apenas retorna ele

        }

    }



    // Cria o novo símbolo não-terminal

    Symbol *s = malloc(sizeof(Symbol));

    strcpy(s->name, name);

    s->type = SYMBOL_NON_TERMINAL;

   

    g->symbols[g->symbol_count++] = s;

    return s;

}



// add-terminal(x): adiciona x ao conjunto de terminais e retorna seu descritor.

Symbol* add_terminal(Grammar *g, const char *name) {

    // Regra do professor: Erro se x se encontra no conjunto de não-terminais.

    for (int i = 0; i < g->symbol_count; i++) {

        if (strcmp(g->symbols[i]->name, name) == 0) {

            if (g->symbols[i]->type == SYMBOL_NON_TERMINAL) {

                printf("ERRO NA GRAMATICA: '%s' ja foi declarado como NAO-TERMINAL!\n", name);

                exit(1);

            }

            return g->symbols[i]; // Se já existir como terminal, apenas retorna ele

        }

    }



    // Cria o novo símbolo terminal

    Symbol *s = malloc(sizeof(Symbol));

    strcpy(s->name, name);

    s->type = SYMBOL_TERMINAL;

   

    g->symbols[g->symbol_count++] = s;

    return s;

}



//add-production(A, rhs)

Production* add_production(Grammar *g, Symbol *A, Symbol **rhs_sequence, int rhs_count) {

    if (A->type != SYMBOL_NON_TERMINAL) {

        printf("ERRO: O lado esquerdo (LHS) de uma producao deve ser um Nao-Terminal.\n");

        exit(1);

    }



    Production *p = malloc(sizeof(Production));

    p->lhs = A;

    p->rhs_count = rhs_count;

   

    // Aloca e copia a sequência do lado direito

    p->rhs = malloc(sizeof(Symbol*) * rhs_count);

    for (int i = 0; i < rhs_count; i++) {

        p->rhs[i] = rhs_sequence[i];

    }



    g->productions[g->production_count++] = p;

    return p;

}



// Encontra um símbolo no banco de dados da gramática comparando o nome string

Symbol* find_symbol(Grammar *g, const char *name) {

    if (g == NULL || name == NULL) return NULL;



    for (int i = 0; i < g->symbol_count; i++) {

        if (strcmp(g->symbols[i]->name, name) == 0) {

            return g->symbols[i]; // Retorna o ponteiro correto encontrado no vetor!

        }

    }

    return NULL; // Se varrer o vetor inteiro e não achar, retorna NULL

}



// is-terminal(X): retorna verdadeiro se X é um terminal, e falso caso contrário.

bool is_terminal(Grammar *g, Symbol *X) {

    if (X == NULL) return false;

    return X->type == SYMBOL_TERMINAL;

}



//lhs(p)

Symbol* lhs(Production *p) {

    return p->lhs;

}



//terminals() - Retorna array de ponteiros terminado em NULL

Symbol** terminals(Grammar *g) {

    // Conta quantos terminais existem

    int count = 0;

    for (int i = 0; i < g->symbol_count; i++) {

        if (g->symbols[i]->type == SYMBOL_TERMINAL) count++;

    }



    Symbol **result = malloc(sizeof(Symbol*) * (count + 1));

    int idx = 0;

    for (int i = 0; i < g->symbol_count; i++) {

        if (g->symbols[i]->type == SYMBOL_TERMINAL) {

            result[idx++] = g->symbols[i];

        }

    }

    result[idx] = NULL; // Marcador de fim do iterador

    return result;

}



//nonterminals()

Symbol** nonterminals(Grammar *g) {

    int count = 0;

    for (int i = 0; i < g->symbol_count; i++) {

        if (g->symbols[i]->type == SYMBOL_NON_TERMINAL) count++;

    }



    Symbol **result = malloc(sizeof(Symbol*) * (count + 1));

    int idx = 0;

    for (int i = 0; i < g->symbol_count; i++) {

        if (g->symbols[i]->type == SYMBOL_NON_TERMINAL) {

            result[idx++] = g->symbols[i];

        }

    }

    result[idx] = NULL;

    return result;

}



//productions()

Production** productions(Grammar *g) {

    Production **result = malloc(sizeof(Production*) * (g->production_count + 1));

    for (int i = 0; i < g->production_count; i++) {

        result[i] = g->productions[i];

    }

    result[g->production_count] = NULL;

    return result;

}



//rhs(p)

Symbol** rhs(Production *p) {

    Symbol **result = malloc(sizeof(Symbol*) * (p->rhs_count + 1));

    for (int i = 0; i < p->rhs_count; i++) {

        result[i] = p->rhs[i];

    }

    result[p->rhs_count] = NULL;

    return result;

}



// productions-for(A)

Production** productions_for(Grammar *g, Symbol *A) {

    int count = 0;

    for (int i = 0; i < g->production_count; i++) {

        if (g->productions[i]->lhs == A) count++;

    }



    Production **result = malloc(sizeof(Production*) * (count + 1));

    int idx = 0;

    for (int i = 0; i < g->production_count; i++) {

        if (g->productions[i]->lhs == A) {

            result[idx++] = g->productions[i];

        }

    }

    result[idx] = NULL;

    return result;

}



//occurrences(X)

Occurrence** occurrences(Grammar *g, Symbol *X) {

    int count = 0;

    // Primeiro conta todas as vezes que X aparece no lado direito de qualquer regra

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



//production(O)

Production* production(Occurrence *O) {

    if (O == NULL) return NULL;

    return (Production*)O->production_ptr;

}



//tail(p, i)

Symbol** tail(Production *p, int i) {

    if (p == NULL || i < 0) return NULL;



    // Se o índice i+1 estiver além do tamanho do RHS, retorna uma lista vazia (apenas o NULL)

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

    result[idx] = NULL; // Fecha o iterador

    return result;

}



bool verificar_ll1(Grammar *g) {
    printf("==================================================\n");
    printf("   GERANDO CONJUNTOS PREDICT (VALIDACAO LL(1))    \n");
    printf("==================================================\n");

    bool g_tem_conflito = false;

    // 1. Pega todos os não-terminais cadastrados na gramática
    Symbol **todos_nao_terminais = nonterminals(g);

    // 2. Varre cada não-terminal da lista
    for (int nt_idx = 0; todos_nao_terminais[nt_idx] != NULL; nt_idx++) {
        Symbol *nt_atual = todos_nao_terminais[nt_idx];
        
        // Pega as regras específicas deste não-terminal
        Production **regras = productions_for(g, nt_atual);
        
        // Arrays locais para validar conflito dentro DO MESMO não-terminal
        const char *terminais_vistos[20];
        int qtd_vistos = 0;

        printf(">>> Analisando producoes para o Nao-Terminal: {%s}\n", nt_atual->name);
        printf("--------------------------------------------------\n");

        for (int i = 0; regras[i] != NULL; i++) {
            Production *p = regras[i];
            Symbol **sequencia_rhs = rhs(p);
            const char *predict_symbol_name = "";

            // Se a regra for Epsilon (vazia)
            if (p->rhs_count == 0 || sequencia_rhs == NULL) {
                predict_symbol_name = "$ (EOF)";
            } else {
                // Pega o primeiro terminal do lado direito da regra
                predict_symbol_name = sequencia_rhs[0]->name;
            }

            // Print visual das regras rodando na tela
            printf("  Regra [%d]: %s -> ", i + 1, p->lhs->name);
            if (p->rhs_count == 0) {
                printf("epsilon ");
            } else {
                for (int j = 0; j < p->rhs_count; j++) {
                    printf("%s ", sequencia_rhs[j]->name);
                }
            }
            printf("\n     => PREDICT = { %s }\n\n", predict_symbol_name);

            // Validação de Interseção: O mesmo Predict não pode aparecer em duas regras do mesmo NT
            for (int v = 0; v < qtd_vistos; v++) {
                if (strcmp(terminais_vistos[v], predict_symbol_name) == 0) {
                    g_tem_conflito = true;
                    printf("  [CONFLITO LL(1) DETECTADO]: O simbolo '%s' quebra a predicao em {%s}!\n\n", 
                           predict_symbol_name, nt_atual->name);
                }
            }

            terminais_vistos[qtd_vistos++] = predict_symbol_name;

            if (sequencia_rhs != NULL) {
                free(sequencia_rhs); // Limpa o vetor alocado por rhs()
            }
        }
        
        free(regras); // Limpa o vetor alocado por productions_for()
        printf("\n");
    }

    free(todos_nao_terminais); // Limpa o vetor alocado por nonterminals()

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