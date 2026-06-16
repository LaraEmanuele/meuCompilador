// lexer.c
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h" // Inclui o seu próprio cabeçalho

/***********Funções relacionadas a lista de tokens***********/
// Função que insere um token no fim da lista encadeada
static void append_token(TokenNode **head, Token token) {
    TokenNode *new_node = malloc(sizeof(TokenNode));
    new_node->token = token;
    new_node->next = NULL;

    // Se a lista estiver vazia, o novo nó se torna o primeiro (head)
    if (*head == NULL) {
        *head = new_node;
        return;
    }

    // Caso contrário, caminha até o último nó e pendura o novo lá
    TokenNode *current = *head;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = new_node;
}

// SUA FUNÇÃO DE TESTE: Mostra a lista inteira no terminal. Usada para testar o lexer
void print_token_list(TokenNode *head) {
    TokenNode *current = head;
    printf("\n============= LISTA DE TOKENS GERADA =============\n");
    
    int index = 0;
    while (current != NULL) {
        printf("[%d] ", index++);
        
        switch (current->token.type) {
            case TOKEN_KEYWORD:
                if (current->token.value.keyword == EXIT)  printf("Tipo: KEYWORD        | Valor: EXIT\n");
                if (current->token.value.keyword == PRINT) printf("Tipo: KEYWORD        | Valor: PRINT\n");
                break;
                
            case TOKEN_LITERAL:
                if (current->token.value.literal_value == STRING) {
                    printf("Tipo: LITERAL (STR)  | Valor: TEXTO\n");
                } else {
                    printf("Tipo: LITERAL (INT)  | Valor: %d\n", current->token.value.literal_value);
                }
                break;
                
            case TOKEN_SEPARATOR:
                printf("Tipo: SEPARATOR      | Valor: ");
                if (current->token.value.separator == SEMICOLON)  printf(";\n");
                if (current->token.value.separator == OPEN_PAREN) printf("(\n");
                if (current->token.value.separator == CLOSE_PAREN) printf(")\n");
                break;
        }
        current = current->next; // Avança para o próximo nó
    }
    printf("==================================================\n\n");
}

// Função para limpar a memória no final do programa
void free_token_list(TokenNode *head) {
    TokenNode *current = head;
    while (current != NULL) {
        TokenNode *next_node = current->next;
        free(current);
        current = next_node;
    }
}


/***********Funções relacionadas análise Lexica***********/
static Token generate_number (char current, FILE *file, int current_line){
  Token token;
  token.type = TOKEN_LITERAL;
  token.line = current_line;

  int value = 0;

  while(isdigit(current) && current != EOF){
    value = (value * 10) + (current - '0');
    current = fgetc(file);
  }

  if (current != EOF) {
    ungetc(current, file);
  }

  token.value.literal_value = value;

  return token;
}

static Token* generate_keyword (char current, FILE *file, int current_line){
  Token *token = malloc(sizeof(Token));
  token->type = TOKEN_KEYWORD;
  token->line = current_line;

  char *keyword = malloc(sizeof(char) * 256);
  int keyword_index = 0;

  while(isalpha(current) && current != EOF){
    keyword[keyword_index] = current;
    keyword_index++;
    current = fgetc(file);
  }

  keyword[keyword_index] = '\0';

  if (current != EOF) {
    ungetc(current, file);
  }

  if (strcmp(keyword, "exit") == 0){
    token->value.keyword = EXIT;
  }else if (strcmp(keyword, "print") == 0) {
    token->value.keyword = PRINT;
  } else {
  }

  free(keyword);
  return token;
}

TokenNode* lexer (FILE *file){
  char current = fgetc(file);
  LexerState state = STATE_START; // Todo arquivo começa no estado START
  TokenNode *token_list = NULL; //Começa com uma lista vazia para armazenar os tokens

  int current_line = 1; // Linha do arquivo

  while (current!=EOF){
    switch (state){
      case STATE_START:
        if (current == '\n') {
            current_line++;
            current = fgetc(file);
        } else if (current == ' ' || current == '\r' || current == '\t'){
          current = fgetc(file);
        }else if (current == ';') {
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.value.separator = SEMICOLON;
            token.line = current_line;
            append_token(&token_list, token); //Salva o token na lista

            current = fgetc(file); // Avança
        } else if (current == '(') {
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.value.separator = OPEN_PAREN;
            token.line = current_line;
            append_token(&token_list, token); //Salva o token na lista

            current = fgetc(file); // Avança
        } else if (current == ')') {
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.value.separator = CLOSE_PAREN;
            token.line = current_line;
            append_token(&token_list, token); //Salva o token na lista

            current = fgetc(file); // Avança
        }else if (current == '"') {
            Token token;
            token.type = TOKEN_LITERAL; // Classificado como Literal na sua estrutura
            token.line = current_line;
            token.value.literal_value = STRING; // Indica que o valor interno representa uma String

            // Consome os caracteres de dentro das aspas (vamos pular o texto interno no teste simples)
            current = fgetc(file);
            while (current != '"' && current != EOF) {
                if (current == '\n') current_line++;
                current = fgetc(file);
            }
            
            append_token(&token_list, token);
            current = fgetc(file); // Avança para depois das aspas de fechamento
        } else if (isdigit(current)) {
            // Não processa o número aqui! Só muda o estado
            state = STATE_IN_NUMBER;
        } else if (isalpha(current)) {
            // Não processa a palavra aqui! Só muda o estado
            state = STATE_IN_ID;
        } else {
            printf("ERRO LEXICO: Caractere invalido '%c'\n", current);
            current = fgetc(file);
        }
        break;

      case STATE_IN_NUMBER: {
          Token token = generate_number(current, file, current_line);
          token.line = current_line;
          append_token(&token_list, token); //Salva o token na lista
          
          // O ungetc dentro de generate_number já organizou o arquivo.
          // Lemos o próximo caractere e voltamos para o START.
          current = fgetc(file); 
          state = STATE_START; 
          break;
      }

    case STATE_IN_ID: {
        Token *test_keyword = generate_keyword(current, file, current_line);
        if (test_keyword->value.keyword == EXIT || test_keyword->value.keyword == PRINT) {
            append_token(&token_list, *test_keyword); 
        }
        free(test_keyword);
        
        current = fgetc(file);
        state = STATE_START; 
        break;
    }

    }
  }
  return token_list;
}
