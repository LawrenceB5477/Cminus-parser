#ifndef PARSER_HEADER 
#define PARSER_HEADER
#include "lexer.h"
#define DEBUG 0

typedef enum {VAR, FUNC, ARRAY} SYMBOL_TYPE; 

typedef enum {INT, VOID, ERRORTYPE, INTARRAY} TYPE; 

typedef struct SYMBOL_STRUCT_ENTRY {
    char *id; 
    SYMBOL_TYPE symType; 
    TYPE type;
    struct SYMBOL_STRUCT_ENTRY *arguments; 
    struct SYMBOL_STRUCT_ENTRY *sibling; 
} SYMBOL_ENTRY; 

typedef struct SYMBOL_TABLE_STRUCT {
    SYMBOL_ENTRY **array;
    SYMBOL_ENTRY *context;  
    struct SYMBOL_TABLE_STRUCT *next; 
} SYMBOL_TABLE; 

void parse(); 
char* match(TOKEN token, char *lex);

//Parsing functions 
void program(void); 
void declaration_list(void);
void declaration_list_prime(void);  
void declaration(void);
void declaration_prime(SYMBOL_ENTRY *); 
void var_declaration(void); 
/* */ void var_declaration_prime(SYMBOL_ENTRY *); 
/**/ TYPE type_specifier(void); 
void params(SYMBOL_ENTRY *entry);
void params_prime(void); 
void param_list_prime(SYMBOL_ENTRY *arg); 
void param_prime(SYMBOL_ENTRY *param); 
void compound_stmt(SYMBOL_ENTRY *entry);
void local_declarations(void);
void statement_list(void); 
void statement(void); 
void expression_stmt(void); 
void selection_stmt(void);
void selection_stmt_prime(int );
void iteration_stmt(void);
void return_stmt(void);
void return_stmt_prime(void);
TYPE expression(void); 
TYPE expression_prime(SYMBOL_ENTRY *entry);
TYPE expression_prime_prime(SYMBOL_ENTRY *entry); 
void var_prime(void); 
void simple_expression(void); 
TYPE simple_expression_prime(TYPE type); 
void relop(void);
TYPE additive_expression(void); 
TYPE additive_expression_prime(TYPE type);
void addop(void);
TYPE term(void);
TYPE term_prime(TYPE type);
void mulop(void);
TYPE factor(void);
TYPE factor_prime(SYMBOL_ENTRY *entry);
bool args(SYMBOL_ENTRY *entry); 
bool arg_list(SYMBOL_ENTRY *entry); 
bool arg_list_prime(SYMBOL_ENTRY *entry); 

#endif
