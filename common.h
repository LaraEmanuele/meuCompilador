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
    STRING,
} TypeLiteral;

typedef enum {
	SEMICOLON,
	OPEN_PAREN,
	CLOSE_PAREN,
} TypeSeparator;

typedef struct {
  TokenType type;
  /*O union irá armazenar uma das informações definidas em seu interior */
  union {
    TypeKeyWord keyword;     // Preenchido se for TOKEN_KEYWORD
    int literal_value;       // Preenchido se for TOKEN_LITERAL (seu 'int value')
    TypeSeparator separator; // Preenchido se for TOKEN_SEPARATOR
  } value;
  int line; // Preenchido com a linha do arquivo tex em que o token foi obtido
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