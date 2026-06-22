#ifndef GRAMMAR_H
#define GRAMMAR_H

#include "common.h"

// Protótipos das funções da gramática

// Funções de Criação
Grammar* grammar(const char *start_symbol_name);
Symbol* add_terminal_separator(Grammar *g, const char *name, TypeSeparator separator_enum);
Symbol* add_terminal_keyword(Grammar *g, const char *name, TypeKeyWord keyword_enum);
Symbol* add_terminal_literal(Grammar *g, const char *name, TypeLiteral literal_enum);
Symbol* add_terminal_identifier(Grammar *g, const char *name);
Symbol* add_nonterminal(Grammar *g, const char *name);
Symbol* add_terminal(Grammar *g, const char *name);
Production* add_production(Grammar *g, Symbol *A, Symbol **rhs_sequence, int rhs_count);

// Funções de Verificação Simples
Symbol* find_symbol(Grammar *g, const char *name);
bool is_terminal(Grammar *g, Symbol *X);
Symbol* lhs(Production *p);

// Iteradores (Retornam arrays terminados em NULL)
Symbol** terminals(Grammar *g);
Symbol** nonterminals(Grammar *g);
Production** productions(Grammar *g);
Symbol** rhs(Production *p);
Production** productions_for(Grammar *g, Symbol *A);

// Funções Avançadas de Ocorrência
Occurrence** occurrences(Grammar *g, Symbol *X);
Production* production(Occurrence *O);
Symbol** tail(Production *p, int i);

//Validação da gramática LL(1)
bool verificar_ll1(Grammar *g);

#endif