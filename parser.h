#ifndef PARSER_HEADER 
#define PARSER_HEADER
#include "lexer.h"
#define DEBUG 0

void parse(); 
void match(TOKEN token, char *lex);

//Parsing functions 
void program(void); 
void declaration_list(void);
void declaration_list_prime(void);  
void declaration(void);
void declaration_prime(void); 
void var_declaration(void); 
void var_declaration_prime(void); 
void type_specifier(void); 
void params(void);
void params_prime(void); 
void param_list_prime(void); 
void param_prime(void); 
void compound_stmt(void);
void local_declarations(void);
void statement_list(void); 
void statement(void); 
void expression_stmt(void); 
void selection_stmt(void);
void selection_stmt_prime(void);
void iteration_stmt(void);
void return_stmt(void);
void return_stmt_prime(void);
void expression(void); 
void expression_prime(void);
void expression_prime_prime(); 
void var_prime(void); 
void simple_expression(void); 
void simple_expression_prime(void); 
void relop(void);
void additive_expression(void); 
void additive_expression_prime(void);
void addop(void);
void term(void);
void term_prime(void);
void mulop(void);
void factor(void);
void factor_prime(void);
void args(void); 
void arg_list(void); 
void arg_list_prime(void); 

#endif