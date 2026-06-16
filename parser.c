#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "parser.h"
#include "lexer.h"

static TokenNode *current_node = NULL;
// Ponteiro para o ficheiro onde vamos gravar o código AssemblySam
static FILE *f_asm = NULL; 

static void match(TokenType expected_type, int expected_value, const char *error_msg) {
    if (current_node == NULL) { printf("[ERRO]: %s\n", error_msg); exit(1); }
    Token t = current_node->token;
    if (t.type != expected_type) { printf("[ERRO]: Linha %d - %s\n", t.line, error_msg); exit(1); }
    if (expected_type == TOKEN_SEPARATOR && t.value.separator != expected_value) { printf("[ERRO]: Linha %d - %s\n", t.line, error_msg); exit(1); }
    if (expected_type == TOKEN_KEYWORD && t.value.keyword != expected_value) { printf("[ERRO]: Linha %d - %s\n", t.line, error_msg); exit(1); }
    current_node = current_node->next;
}

void parse_S();

// Processa a regra do PRINT e gera o AssemblySam correspondente
static void parse_print_rule() {
    match(TOKEN_KEYWORD, PRINT, "Esperada 'print'.");
    match(TOKEN_SEPARATOR, OPEN_PAREN, "Esperado '('.");
    
    // Como o teu Lexer já validou o TOKEN_LITERAL, sabemos que é uma string
    match(TOKEN_LITERAL, STRING, "Esperado um TEXTO.");
    
    match(TOKEN_SEPARATOR, CLOSE_PAREN, "Esperado ')'.");
    match(TOKEN_SEPARATOR, SEMICOLON, "Esperado ';'.");

    // ============================================================
    //  MOMENTO ASSEMBLY SAM: Gerando o código do Print
    // ============================================================
    // (Ajusta estas instruções de acordo com as instruções reais do Sam)
    fprintf(f_asm, "    ; --- Comando Print ---\n");
    fprintf(f_asm, "    PUSH string_var_ficticia\n"); 
    fprintf(f_asm, "    CALL sys_print\n\n");

    parse_S();
}

// Processa a regra do EXIT e gera o AssemblySam correspondente
static void parse_exit_rule() {
    match(TOKEN_KEYWORD, EXIT, "Esperada 'exit'.");
    match(TOKEN_SEPARATOR, OPEN_PAREN, "Esperado '('.");
    
    // Captura o valor numérico que o utilizador digitou no exit(X) antes de avançar o token!
    int valor_retorno = current_node->token.value.literal_value;
    
    match(TOKEN_LITERAL, INT, "Esperado um NUMERO.");
    match(TOKEN_SEPARATOR, CLOSE_PAREN, "Esperado ')'.");
    match(TOKEN_SEPARATOR, SEMICOLON, "Esperado ';'.");

    // ============================================================
    //  MOMENTO ASSEMBLY SAM: Gerando o código do Exit
    // ============================================================
    fprintf(f_asm, "    ; --- Comando Exit ---\n");
    fprintf(f_asm, "    MOV R0, #%d\n", valor_retorno); // Move o número do exit para o registador
    fprintf(f_asm, "    CALL sys_exit\n\n");

    if (current_node != NULL) {
        printf("[ERRO SINTÁTICO]: Linha %d - Codigo morto apos o exit.\n", current_node->token.line);
        exit(1);
    }
}

void parse_S() {
    if (current_node == NULL) return;
    Token t = current_node->token;
    if (t.type == TOKEN_KEYWORD) {
        if (t.value.keyword == PRINT) { parse_print_rule(); return; }
        if (t.value.keyword == EXIT)  { parse_exit_rule(); return; }
    }
    printf("[ERRO SINTÁTICO]: Linha %d - Comando inesperado.\n", t.line);
    exit(1);
}

// Atualizada para receber o ficheiro de escrita do Assembly
void program_parser(TokenNode *tokens_do_lexer, FILE *output_file) {
    current_node = tokens_do_lexer;
    f_asm = output_file; // Define o arquivo global de escrita
    
    // Cabeçalho padrão do AssemblySam
    fprintf(f_asm, ".SECTION DATA\n");
    fprintf(f_asm, "    ; Aqui entrariam as strings capturadas\n");
    fprintf(f_asm, ".SECTION TEXT\n");
    fprintf(f_asm, ".GLOBAL _start\n");
    fprintf(f_asm, "_start:\n\n");

    parse_S(); // Dispara as regras
    
    printf("\n>> [Parser + Codegen]: Sintaxe validada e AssemblySam gerado!\n");
}