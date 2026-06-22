// common.h
#ifndef COMMON_H
#define COMMON_H

// ==========================================
// ESTRUTURAS DO LEXER
// ==========================================
typedef enum {
    STATE_START,
    STATE_IN_NUMBER,
    STATE_IN_ID
} LexerState;

typedef enum {
  TOKEN_KEYWORD,
  TOKEN_LITERAL,
  TOKEN_SEPARATOR
} TokenType;

typedef enum {
	EXIT,
    PRINT,
} TypeKeyWord;	

typedef enum {
	INT,
    FLOAT,
    STRING,
} TypeLiteral;

typedef enum {
	SEMICOLON,
	OPEN_PAREN,
	CLOSE_PAREN,
    DOUBLE_QUOTES,
} TypeSeparator;

typedef enum {
    LIT_INT,
    LIT_FLOAT,
    LIT_STRING,
} LiteralTag;

typedef struct {
    TokenType type;
    union {
        TypeKeyWord keyword;     
        TypeSeparator separator; 
        
        // Estrutura unificada para os literais (guarda o tipo E o valor)
        struct {
            LiteralTag tag;       // LIT_INT ou LIT_STRING OU LIT_FLOAT
            int int_value;        // Preenchido se for LIT_INT
            float float_value;    // Preenchido se for LIT_FLOAT
            char *string_value;   // Preenchido se for LIT_STRING
        } literal;
    } value;
    int line; 
} Token;

typedef struct TokenNode {
    Token token;             // O token atual (com tipo e valor)
    struct TokenNode *next;  // Ponteiro para o próximo token da lista
} TokenNode;

// ==========================================
// ESTRUTURAS DA GRAMÁTICA
// ==========================================
typedef enum {
    SYMBOL_TERMINAL,
    SYMBOL_NON_TERMINAL
} SymbolType;

typedef struct {
    char name[50];       
    SymbolType type;     
    
    //Guarda a assinatura exata do Token correspondente
    struct {
        TokenType token_type;
        union {
            TypeKeyWord keyword;
            TypeSeparator separator;
            TypeLiteral literal; // adicionado para cobrir INT/STRING de forma limpa
        } value;
    } terminal_info;
} Symbol;

typedef struct {
    Symbol *lhs;         
    Symbol **rhs;        
    int rhs_count;       
} Production;

// Representa uma ocorrência de um símbolo: qual produção (p) e em qual índice (i)
typedef struct {
    void *production_ptr; // Ponteiro para a Production (void* para evitar dependência cíclica)
    int index;            // O 'i' (índice baseado em 0)
} Occurrence;

typedef struct {
    Symbol *start_symbol;
    Symbol *symbols[100];       
    int symbol_count;
    Production *productions[100]; 
    int production_count;
} Grammar;

#endif