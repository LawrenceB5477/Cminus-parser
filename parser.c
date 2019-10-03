#include "parser.h"

extern FILE* inputFile; 

TOKEN_STRUCT *currentToken; 
TOKEN_STRUCT *testToken; 

void match(TOKEN token, char *lex) {
    if (currentToken->token == token) {
        if (token == SYMBOL || token == KEYWORD) {
            if (strcmp(currentToken->lexeme, lex) != 0) {
                printf("REJECT\n");
                exit(1); 
            }
        } 
        
        free(currentToken); 
        currentToken = nextToken(); 
    } else {
        printf("REJECT\n");
        exit(1); 
    }
}

void parse(char *fileName) {
    inputFile = fopen(fileName, "r"); 
  
    if (!inputFile) {
        fprintf(stderr, "Error opening file. Aborting...\n"); 
        exit(1); 
    }

    currentToken = nextToken();
    program(); 
    fclose(inputFile);
}

void debug(const char *string) {
    if (DEBUG) {
        printf("%s\n", string); 
    }
}

int compareToken(TOKEN token, const char *string) {
    if (token != currentToken->token) {
        return 0; 
    }

    if (string != NULL && strcmp(string, currentToken->lexeme) != 0) {
        return 0; 
    }

    return 1; 
}

//Parsing functions 
void program(void) {
    debug("program"); 

    declaration_list(); 
}
void declaration_list(void) {
    debug("declaration-list");
    declaration();
    declaration_list_prime(); 
}

void declaration_list_prime(void) {
    debug("declaration-list`"); 

    if (compareToken(KEYWORD, "int") || compareToken(KEYWORD, "void")) {
        declaration(); 
        declaration_list_prime(); 
    }
}

void declaration(void) {
    debug("declaration");

    type_specifier(); 
    match(ID, NULL);
    declaration_prime(); 
}

void declaration_prime(void) {
    debug("declaration-prime"); 

    if (compareToken(SYMBOL, "(")) {
        match(SYMBOL, "(");
        params(); 
        match(SYMBOL, ")");
        compound_stmt(); 
    } else {
        var_declaration_prime(); 
    }
}

void var_declaration(void) {
    debug("var-declaration"); 

    type_specifier(); 
    match(ID, NULL); 
    var_declaration_prime(); 
} 

void var_declaration_prime(void) {
    debug("var-declaration-prime");

     if (compareToken(SYMBOL, ";")) {
        match(SYMBOL, ";"); 
    } else {
        match(SYMBOL, "[");
        match(NUM, NULL); 
        match(SYMBOL, "]");
        match(SYMBOL, ";"); 
    }
} 

void type_specifier(void) {
    debug("type-specifier"); 

    if (compareToken(KEYWORD, "int")) {
        match(KEYWORD, "int"); 
    } else {
        match(KEYWORD, "void"); 
    }
} 

void params(void) {
    debug("params"); 

    if (compareToken(KEYWORD, "int")) {
        match(KEYWORD, "int"); 
        match(ID, NULL); 
        param_prime(); 
        param_list_prime(); 
    } else {
        match(KEYWORD, "void");
        params_prime(); 
    }
}

void params_prime(void) {
    debug("params-prime"); 

    if (compareToken(ID, NULL)) {
        match(ID, NULL); 
        param_prime(); 
        param_list_prime(); 
    }
}

void param_list_prime(void) {
    debug("param-list-prime"); 

    if (compareToken(SYMBOL, ",")) {
        match(SYMBOL, ",");
        type_specifier(); 
        match(ID, NULL); 
        param_prime(); 
        param_list_prime(); 
    }
} 

void param_prime(void) {
    debug("param-prime"); 
    if (compareToken(SYMBOL, "[")) {
        match(SYMBOL, "[");
        match(SYMBOL, "]"); 
    }
}

void compound_stmt(void) {
    debug("compound-stmt");

    match(SYMBOL, "{");
    local_declarations(); 
    statement_list(); 
    match(SYMBOL, "}");
}

void local_declarations(void) {
    debug("local-declarations"); 

    if (compareToken(KEYWORD, "void") || compareToken(KEYWORD, "int")) {
        var_declaration(); 
        local_declarations(); 
    }
}

void statement_list(void) {
    debug("statement-list");

    //expression - ID or ( or NUM or { or if or while or return or ;
    if (compareToken(ID, NULL) || compareToken(NUM, NULL) || compareToken(SYMBOL, "(") || compareToken(SYMBOL, "{")
    || compareToken(KEYWORD, "if") || compareToken(KEYWORD, "while") || compareToken(KEYWORD, "return") 
    || compareToken(SYMBOL, ";"))
        {
            statement(); 
            statement_list(); 
        }
}

void statement(void) {
    // ( ID NUM use this for expression
    //expression statemnet is ID ( NUM ;
    debug("statement"); 

    if (compareToken(SYMBOL, "(") || compareToken(ID, NULL) || compareToken(NUM, NULL) || compareToken(SYMBOL, ";")) {
        expression_stmt(); 
    } else if (compareToken(SYMBOL, "{")) {
        compound_stmt(); 
    } else if (compareToken(KEYWORD, "if")) {
        selection_stmt(); 
    } else if (compareToken(KEYWORD, "while")) {
        iteration_stmt(); 
    } else {
        return_stmt(); 
    }
}

void expression_stmt(void) {
    debug("expression-stmt");

    if (compareToken(SYMBOL, ";")) {
        match(SYMBOL, ";"); 
    } else {
        expression(); 
        match(SYMBOL, ";"); 
    }
}

void selection_stmt(void) {
    debug("selection-stmt"); 

    match(KEYWORD, "if");
    match(SYMBOL, "(");
    expression(); 
    match(SYMBOL, ")");
    statement(); 
    selection_stmt_prime(); 
}

void selection_stmt_prime(void) {
    debug("selection-stmt`");
    
    if (compareToken(KEYWORD, "else")) {
        match(KEYWORD, "else");
        statement(); 
    }
}

void iteration_stmt(void) {
    debug("iteration-stmt"); 

    match(KEYWORD, "while"); 
    match(SYMBOL, "(");
    expression(); 
    match(SYMBOL, ")");
    statement(); 
}

void return_stmt(void) {
    debug("return-stmt"); 

    match(KEYWORD, "return"); 
    return_stmt_prime(); 
}

void return_stmt_prime(void) {
    debug("return-stmt`");

    if (compareToken(SYMBOL, ";")) {
        match(SYMBOL, ";");
    } else {
        expression(); 
        match(SYMBOL, ";"); 
    }
}

void expression(void) {
    debug("expression"); 

        // ( ID NUM use this for expression
    if (compareToken(NUM, NULL)) {
        match(NUM, NULL); 
        term_prime(); 
        additive_expression_prime(); 
        simple_expression_prime(); 
    } else if (compareToken(ID, NULL)) {
        match(ID, NULL); 
        expression_prime(); 
    } else {
        match(SYMBOL, "(");
        expression();
        match(SYMBOL, ")");
        term_prime(); 
        additive_expression_prime();
        simple_expression_prime(); 
    }
}

void expression_prime(void) {
    debug("expression`");

    if (compareToken(SYMBOL, "=")) {
        match(SYMBOL, "=");
        expression();
    } else if (compareToken(SYMBOL, "[")) {
        match(SYMBOL, "[");
        expression(); 
        match(SYMBOL, "]");
        expression_prime_prime(); 
    } else if (compareToken(SYMBOL, "(")) {
        match(SYMBOL, "(");
        args();
        match(SYMBOL, ")");
        term_prime();
        additive_expression_prime(); 
        simple_expression_prime(); 
    } else {
        term_prime();
        additive_expression_prime();
        simple_expression_prime();
    }
}

void expression_prime_prime(void) {
    debug("expression``");

    if (compareToken(SYMBOL, "=")) {
        match(SYMBOL, "=");
        expression();
    } else {
        term_prime(); 
        additive_expression_prime();
        simple_expression_prime(); 
    }
}

void simple_expression_prime(void) {
    debug("simple-expression`");

    if (compareToken(SYMBOL, "<=") || compareToken(SYMBOL, "<") || compareToken(SYMBOL, ">") || compareToken(SYMBOL, ">=")
    || compareToken(SYMBOL, "==") || compareToken(SYMBOL, "!=")) {
        relop(); 
        additive_expression();
    }
}

void relop(void) {
    debug("relop");

    if (compareToken(SYMBOL, "<=")) {
        match(SYMBOL, "<=");
    } else if (compareToken(SYMBOL, "<")) {
        match(SYMBOL, "<");
    } else if (compareToken(SYMBOL, ">")) {
        match(SYMBOL, ">"); 
    } else if (compareToken(SYMBOL, ">=")) {
        match(SYMBOL, ">=");
    } else if (compareToken(SYMBOL, "==")) {
        match(SYMBOL, "=="); 
    } else {
        match(SYMBOL, "!=");
    }
}

void additive_expression(void) {
    debug("additive-expression"); 

    term();
    additive_expression_prime(); 
}

void additive_expression_prime(void) {
    debug("additive-expression`");

    if (compareToken(SYMBOL, "+") || compareToken(SYMBOL, "-")) {
        addop();
        term(); 
        additive_expression_prime(); 
    }
}

void addop(void) {
    debug("addop");

    if (compareToken(SYMBOL, "+")) {
        match(SYMBOL, "+");
    } else {
        match(SYMBOL, "-"); 
    }
}

void term(void) {
    debug("term");

    factor();
    term_prime();
}

void term_prime(void) {
    debug("term`");

    if (compareToken(SYMBOL, "*") || compareToken(SYMBOL, "/")) {
        mulop();
        factor();
        term_prime();
    }
}

void mulop(void) {
    debug("mulop");

    if (compareToken(SYMBOL, "*")) {
        match(SYMBOL, "*");
    } else {
        match(SYMBOL, "/");
    }
}

void factor(void) {
    debug("factor");

    if (compareToken(NUM, NULL)) {
        match(NUM, NULL); 
    } else if (compareToken(ID, NULL)) {
        match(ID, NULL); 
        factor_prime(); 
    } else {
        match(SYMBOL, "(");
        expression(); 
        match(SYMBOL, ")");
    }
}
void factor_prime(void) {
    debug("factor`");

    if (compareToken(SYMBOL, "[")) {
        match(SYMBOL, "[");
        expression(); 
        match(SYMBOL, "]");
    } else if (compareToken(SYMBOL, "(")) {
        match(SYMBOL, "(");
        args(); 
        match(SYMBOL, ")");
    }
}

void args(void) {
    debug("args");

    if (compareToken(ID, NULL) || compareToken(NUM, NULL) || compareToken(SYMBOL, "(")) {
        arg_list(); 
    }
} 
void arg_list(void) {
    debug("args-list");

    expression(); 
    arg_list_prime(); 
}

void arg_list_prime(void) { 
    debug("arg-list`");

    if (compareToken(SYMBOL, ",")) {
        match(SYMBOL, ","); 
        expression(); 
        arg_list_prime(); 
    }
}