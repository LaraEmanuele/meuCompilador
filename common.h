// common.h
#ifndef COMMON_H
#define COMMON_H

// ==========================================
// 1. ENUMS BÁSICOS (DECLARADOS PRIMEIRO)
// ==========================================
typedef enum {
    SYMBOL_TERMINAL,
    SYMBOL_NON_TERMINAL
} SymbolType;

typedef enum {
    STATE_START,
    STATE_IN_NUMBER,
    STATE_IN_ID
} LexerState;

typedef enum {
    TOKEN_KEYWORD,
    TOKEN_LITERAL,
    TOKEN_SEPARATOR,
    TOKEN_IDENTIFIER, 
} TokenType;

typedef enum {
    EXIT,
    PRINT,
    INT_KW,
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
    EQUAL,
} TypeSeparator;

typedef enum {
    LIT_INT,
    LIT_FLOAT,
    LIT_STRING,
} LiteralTag;

// ==========================================
// 2. ESTRUTURAS DA GRAMÁTICA E DO LEXER
// ==========================================
typedef struct {
    char name[50];       
    SymbolType type;     
    
    // Guarda a assinatura exata do Token correspondente
    struct {
        TokenType token_type;
        union {
            TypeKeyWord keyword;
            TypeSeparator separator;
            TypeLiteral literal; 
        } value;
    } terminal_info;
} Symbol;

typedef struct {
    Symbol *lhs;         
    Symbol **rhs;        
    int rhs_count;       
} Production;

typedef struct {
    void *production_ptr; 
    int index;            
} Occurrence;

typedef struct {
    Symbol *start_symbol;
    Symbol *symbols[100];       
    int symbol_count;
    Production *productions[100]; 
    int production_count;
} Grammar;

typedef struct {
    TokenType type;
    union {
        TypeKeyWord keyword;     
        TypeSeparator separator; 
        char identifier_name[32]; 
        
        struct {
            LiteralTag tag;       
            int int_value;        
            float float_value;    
            char *string_value;   
        } literal;
    } value;
    int line; 
} Token;

typedef struct TokenNode {
    Token token;             
    struct TokenNode *next;  
} TokenNode;

// ==========================================
// 3. ESTRUTURAS DO PARSER
// ==========================================
typedef struct {
    char name[32];
    int offset;
} Variable;

typedef struct {
    Variable variaveis[100];
    int quantidade;
} SymbolTable;

#endif