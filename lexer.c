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
                printf("Tipo: KEYWORD        | Valor: ");
                if (current->token.value.keyword == EXIT)  printf("EXIT\n");
                if (current->token.value.keyword == PRINT) printf("PRINT\n");
                break;
                
            case TOKEN_LITERAL:
                // Agora olhamos a tag interna para saber como formatar o print
                switch (current->token.value.literal.tag) {
                    case LIT_INT:
                        printf("Tipo: LITERAL (INT)  | Valor: %d\n", current->token.value.literal.int_value);
                        break;
                    case LIT_FLOAT:
                        // %.2f limita o print a duas casas decimais no terminal
                        printf("Tipo: LITERAL (FLOAT)| Valor: %.2f\n", current->token.value.literal.float_value);
                        break;
                    case LIT_STRING:
                        printf("Tipo: LITERAL (STR)  | Valor: %s\n", current->token.value.literal.string_value);
                        break;
                }
                break;
                
            case TOKEN_SEPARATOR:
                printf("Tipo: SEPARATOR      | Valor: ");
                if (current->token.value.separator == SEMICOLON)     printf(";\n");
                if (current->token.value.separator == OPEN_PAREN)    printf("(\n");
                if (current->token.value.separator == CLOSE_PAREN)   printf(")\n");
                if (current->token.value.separator == DOUBLE_QUOTES) printf("\"\n");
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
        
        // Se for um literal do tipo string, limpa o buffer do texto
        if (current->token.type == TOKEN_LITERAL && current->token.value.literal.tag == LIT_STRING) {
            free(current->token.value.literal.string_value);
        }
        
        free(current);
        current = next_node;
    }
}

/***********Funções relacionadas análise Lexica***********/
static Token generate_number (char current, FILE *file, int current_line){
    Token token;
    token.type = TOKEN_LITERAL;
    token.line = current_line;

    int int_part = 0;

    while(isdigit(current) && current != EOF){
        int_part = (int_part * 10) + (current - '0');
        current = fgetc(file);
    }

    // 2. Verifica se é um FLOAT (achou um ponto '.')
    if (current == '.') {
        token.value.literal.tag = LIT_FLOAT;
        
        current = fgetc(file); // Pula o ponto '.'
        
        float frac_part = 0.0;
        float divisor = 10.0;
        
        // Lê a parte fracionária
        while (isdigit(current) && current != EOF) {
            frac_part += (current - '0') / divisor;
            divisor *= 10.0;
            current = fgetc(file);
        }
        
        token.value.literal.float_value = (float)int_part + frac_part;
    } else {
        // Se não achou ponto, é um INT normal
        token.value.literal.tag = LIT_INT;
        token.value.literal.int_value = int_part;
    }

    // Devolve o caractere que fez o loop parar de volta para o arquivo
    if (current != EOF) {
        ungetc(current, file);
    }

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
    //Se não for nenhum dos dois, mude o tipo do token 
    // para que o lexer saiba que não deve aceitá-lo como KEYWORD
    token->type = TOKEN_LITERAL;
  }

  free(keyword);
  return token;
}

static void generate_string(char current, FILE *file, int *current_line, TokenNode **token_list) {
    // 1. GERA E INSERE O TOKEN DA ASPA DE ABERTURA
    Token open_quote;
    open_quote.type = TOKEN_SEPARATOR; // Ou o TokenType que preferir para DOUBLE_QUOTES
    open_quote.value.separator = DOUBLE_QUOTES; // Assumindo que adicionou DOUBLE_QUOTES no enum
    open_quote.line = *current_line;
    append_token(token_list, open_quote);
    
    
    // 2. CAPTURA O TEXTO DA STRING
    // Aloca o buffer para o texto interno
    char *buffer = malloc(sizeof(char) * 256);
    int idx = 0;

    // O uso da linha como ponteiro (int *current_line)
    // Isso é necessário porque se houver um \n DENTRO da string, 
    // precisa atualizar a linha no lexer principal também.
    current = fgetc(file); // Pula a aspa de abertura que disparou o estado
    
    while (current != '"' && current != EOF) {
        if (current == '\n') {
            (*current_line)++;
        }
        
        buffer[idx++] = current;
        current = fgetc(file);
    }
    buffer[idx] = '\0'; // Finaliza a string para o C

    // 3. GERA E INSERE O TOKEN DO TEXTO LITERAL
    Token string_literal;
    string_literal.type = TOKEN_LITERAL;
    string_literal.value.literal.tag = LIT_STRING;
    string_literal.value.literal.string_value = buffer;
    string_literal.line = *current_line;
    append_token(token_list, string_literal);

    // 4. GERA E INSERE O TOKEN DA ASPA DE FECHAMENTO (se encontrou)
    if (current == '"') {
        Token close_quote;
        close_quote.type = TOKEN_SEPARATOR;
        close_quote.value.separator = DOUBLE_QUOTES;
        close_quote.line = *current_line;
        append_token(token_list, close_quote);
    } else {
        printf("ERRO LÉXICO: Linha %d - String não foi fechada (esperado '\"').\n", *current_line);
    }

    return;
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
            // O &current_line como endereço serve para que a função possa incrementá-la se necessário
            generate_string(current, file, &current_line, &token_list);
            
            // Avançamos para o próximo caractere pós-aspa e continuamos o laço
            current = fgetc(file);
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
        }else{
            printf("ERRO LÉXICO: Identificador desconhecido na linha %d\n", current_line);
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