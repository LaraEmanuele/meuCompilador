// lexer.c
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h" // Inclui o seu próprio cabeçalho

/***********Funções relacionadas a lista de tokens***********/
//Função que insere um token no fim da lista encadeada
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

//FUNÇÃO DE TESTE: Mostra a lista inteira no terminal. Usada para testar o lexer
void print_token_list(TokenNode *head) {
    TokenNode *current = head;
    printf("\n============= LISTA DE TOKENS GERADA =============\n");
    
    int index = 0;
    while (current != NULL) {
        printf("[%d] ", index++);
        
        switch (current->token.type) {
            case TOKEN_KEYWORD:
                printf("Tipo: KEYWORD        | Valor: ");
                if (current->token.value.keyword == EXIT)     printf("EXIT\n");
                else if (current->token.value.keyword == PRINT)    printf("PRINT\n");
                else if (current->token.value.keyword == INT_KW)   printf("INT\n");
                else if (current->token.value.keyword == CONST) printf("CONST\n"); // <-- NOVA PALAVRA-CHAVE
                break;
                
            case TOKEN_IDENTIFIER:
                printf("Tipo: IDENTIFIER     | Valor: %s\n", current->token.value.identifier_name);
                break;

            case TOKEN_LITERAL:
                // Agora olhamos a tag interna para saber como formatar o print
                switch (current->token.value.literal.tag) {
                    case LIT_INT:
                        printf("Tipo: LITERAL (INTEGER)  | Valor: %d\n", current->token.value.literal.int_value);
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
                if (current->token.value.separator == SEMICOLON)         printf(";\n");
                else if (current->token.value.separator == OPEN_PAREN)    printf("(\n");
                else if (current->token.value.separator == CLOSE_PAREN)   printf(")\n");
                else if (current->token.value.separator == DOUBLE_QUOTES) printf("\"\n");
                else if (current->token.value.separator == EQUAL)         printf("=\n"); // Atribuição comum
                else if (current->token.value.separator == EQ)            printf("==\n"); // Comparação de igualdade
                else if (current->token.value.separator == NE)            printf("!=\n"); // Diferença
                else if (current->token.value.separator == LT)            printf("<\n");
                else if (current->token.value.separator == LE)            printf("<=\n");
                else if (current->token.value.separator == GT)            printf(">\n");
                else if (current->token.value.separator == GE)            printf(">=\n");
                // --- OPERADORES ARITMÉTICOS ---
                else if (current->token.value.separator == SUM)           printf("+\n");
                else if (current->token.value.separator == SUB)           printf("-\n");
                else if (current->token.value.separator == MUL)           printf("*\n");
                else if (current->token.value.separator == DIV)           printf("/\n");
                else if (current->token.value.separator == MOD)           printf("%%\n"); // Corrigido para printar '%' corretamente em C
                else printf("DESCONHECIDO (%d)\n", current->token.value.separator);
                break;
        }
        current = current->next; // Avança para o próximo nó
    }
    printf("==================================================\n\n");
}

//Função para limpar a memória no final do programa
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

/***********Funções Separadoras de Tokens***********/
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

static Token* generate_identifier_or_keyword(char current, FILE *file, int current_line) {
    Token *t = (Token*) malloc(sizeof(Token));
    t->line = current_line;

    char buffer[32];
    int idx = 0;
    
    // O primeiro caractere já veio no parâmetro 'current'
    buffer[idx++] = current;

    // Continua lendo enquanto for caractere válido para nomes (letras, números ou _)
    char next = fgetc(file);
    while ((next >= 'a' && next <= 'z') || 
           (next >= 'A' && next <= 'Z') || 
           (next >= '0' && next <= '9') || 
           next == '_') {
        
        if (idx < 31) {
            buffer[idx++] = next;
        }
        next = fgetc(file);
    }
    
    // Devolve o caractere que não pertencia à palavra para o arquivo não perder posição
    if (next != EOF) {
        ungetc(next, file);
    }
    
    buffer[idx] = '\0'; // Finaliza a string em C

    // ==========================================
    // PASSO DE DECISÃO: É palavra-chave ou ID?
    // ==========================================
    if (strcmp(buffer, "const") == 0) {      
        t->type = TOKEN_KEYWORD;
        t->value.keyword = CONST;
    } else if (strcmp(buffer, "int") == 0) {
        t->type = TOKEN_KEYWORD;
        t->value.keyword = INT_KW; 
    } else if (strcmp(buffer, "print") == 0) {
        t->type = TOKEN_KEYWORD;
        t->value.keyword = PRINT;
    } else if (strcmp(buffer, "exit") == 0) {
        t->type = TOKEN_KEYWORD;
        t->value.keyword = EXIT;
    } else {
        // Se não for nenhuma palavra-chave mapeada, é um IDENTIFICADOR (nome de variável)
        t->type = TOKEN_IDENTIFIER;
        strcpy(t->value.identifier_name, buffer); 
    }

    return t;
}

static void generate_string(char current, FILE *file, int *current_line, TokenNode **token_list) {
    Token open_quote;
    open_quote.type = TOKEN_SEPARATOR; 
    open_quote.value.separator = DOUBLE_QUOTES; 
    open_quote.line = *current_line;
    append_token(token_list, open_quote);
    
    char *buffer = malloc(sizeof(char) * 256);
    int idx = 0;

    current = fgetc(file); 
    
    while (current != '"' && current != EOF) {
        if (current == '\n') {
            (*current_line)++;
        }
        
        buffer[idx++] = current;
        current = fgetc(file);
    }
    buffer[idx] = '\0'; 

    Token string_literal;
    string_literal.type = TOKEN_LITERAL;
    string_literal.value.literal.tag = LIT_STRING;
    string_literal.value.literal.string_value = buffer;
    string_literal.line = *current_line;
    append_token(token_list, string_literal);

    if (current == '"') {
        Token close_quote;
        close_quote.type = TOKEN_SEPARATOR;
        close_quote.value.separator = DOUBLE_QUOTES;
        close_quote.line = *current_line;
        append_token(token_list, close_quote);
    } else {
        printf("ERRO LEXICO: Linha %d - String não foi fechada (esperado '\"').\n", *current_line);
    }

    return;
}


/*********** Função da análise Lexica ***********/
TokenNode* lexer (FILE *file){
  char current = fgetc(file);
  LexerState state = STATE_START; 
  TokenNode *token_list = NULL; 

  int current_line = 1; 

  while (current != EOF){
    switch (state){
      case STATE_START:
        if (current == '\n') {
            current_line++;
            current = fgetc(file);
        } else if (current == ' ' || current == '\r' || current == '\t'){
          current = fgetc(file);
        } else if (current == ';') {
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.value.separator = SEMICOLON;
            token.line = current_line;
            append_token(&token_list, token); 
            current = fgetc(file); 
        } else if (current == '(') {
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.value.separator = OPEN_PAREN;
            token.line = current_line;
            append_token(&token_list, token); 
            current = fgetc(file); 
        } else if (current == ')') {
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.value.separator = CLOSE_PAREN;
            token.line = current_line;
            append_token(&token_list, token); 
            current = fgetc(file); 
        } else if (current == '"') {
            generate_string(current, file, &current_line, &token_list);
            current = fgetc(file);
        } else if (current == '='){
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.line = current_line;
            
            current = fgetc(file); // Espia o próximo caractere
            if (current == '=') {
                token.value.separator = EQ; // É '==' (Comparação)
                append_token(&token_list, token); 
                current = fgetc(file); // Consome o segundo '='
            } else {
                token.value.separator = EQUAL; // CORRIGIDO: Agora gera '=' (Atribuição)
                append_token(&token_list, token);
                // Mantém o current sem avançar, pois ele já contém o próximo caractere
            }
        } else if (current == '!') {
            current = fgetc(file);
            if (current == '=') {
                Token token = { .type = TOKEN_SEPARATOR, .value.separator = NE, .line = current_line };
                append_token(&token_list, token);
                current = fgetc(file);
            } else {
                printf("ERRO LEXICO: Caractere '!' isolado na linha %d (esperava '=' para fazer '!=')\n", current_line);
            }
        } else if (current == '+'){
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.value.separator = SUM;
            token.line = current_line;
            append_token(&token_list, token); 
            current = fgetc(file); 
        } else if (current == '-'){
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.value.separator = SUB;
            token.line = current_line;
            append_token(&token_list, token); 
            current = fgetc(file); 
        } else if (current == '*'){
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.value.separator = MUL;
            token.line = current_line;
            append_token(&token_list, token); 
            current = fgetc(file); 
        } else if (current == '/'){
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.value.separator = DIV;
            token.line = current_line;
            append_token(&token_list, token); 
            current = fgetc(file); 
        } else if (current == '%'){
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.value.separator = MOD;
            token.line = current_line;
            append_token(&token_list, token); 
            current = fgetc(file); 
        } else if (current == '<'){
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.line = current_line;
            
            current = fgetc(file);
            if (current == '='){
                token.value.separator = LE;
                append_token(&token_list, token); 
                current = fgetc(file);
            } else {
                token.value.separator = LT;
                append_token(&token_list, token); 
                // Mantém o current sem avançar pois já é o próximo caractere
            }
        } else if (current == '>'){
            Token token;
            token.type = TOKEN_SEPARATOR;
            token.line = current_line;
            
            current = fgetc(file);
            if (current == '='){
                token.value.separator = GE;
                append_token(&token_list, token); 
                current = fgetc(file);
            } else {
                token.value.separator = GT;
                append_token(&token_list, token); 
                // Mantém o current sem avançar
            }
        } else if (isdigit(current)) {
            state = STATE_IN_NUMBER;
        } else if (isalpha(current)) {
            state = STATE_IN_ID;
        } else {
            printf("ERRO LEXICO: Caractere invalido '%c' na linha %d\n", current, current_line);
            current = fgetc(file);
        }
        break;

      case STATE_IN_NUMBER: {
            // generate_number já faz o fgetc interno e devolve o excesso com ungetc
            Token token = generate_number(current, file, current_line);
            append_token(&token_list, token); 
            
            // CORREÇÃO: Não avançamos o fgetc aqui para ler o caractere devolvido no próximo loop
            current = fgetc(file); 
            state = STATE_START; 
            break;
        }

      case STATE_IN_ID: {
            // generate_identifier_or_keyword também faz ungetc interno
            Token *test_keyword = generate_identifier_or_keyword(current, file, current_line);
            
            if (test_keyword->type == TOKEN_KEYWORD || test_keyword->type == TOKEN_IDENTIFIER) {
                append_token(&token_list, *test_keyword); 
            } else {
                printf("ERRO LEXICO: Identificador desconhecido na linha %d\n", current_line);
            }
            free(test_keyword);
            
            // CORREÇÃO: Pegamos o caractere que foi devolvido pelo ungetc para iniciar a próxima validação
            current = fgetc(file);
            state = STATE_START; 
            break;
        }
    }
  }
  return token_list;
}