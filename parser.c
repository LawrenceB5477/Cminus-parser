#include "parser.h"

extern FILE* inputFile; 

unsigned hash_value(char *id) {
    unsigned hash = 0; 
    while (*id) {
        hash = (hash << 4) + *id; 
        id++; 
    }
    return hash % 211; 
}

void free_symbol(SYMBOL_ENTRY *sym) {
    free(sym->id); 
    if (sym->arguments != NULL) {
        SYMBOL_ENTRY *arg = sym->arguments; 
        while (sym->arguments != NULL) {
            arg = sym->arguments->arguments; 
            free(sym->arguments); 
            sym->arguments = arg; 
        }
    }
    if (sym->sibling != NULL) {
        free_symbol(sym->sibling); 
    }
    free(sym); 
}

SYMBOL_ENTRY *create_symbol(char *id, SYMBOL_TYPE symType, TYPE type, SYMBOL_ENTRY *args) {
    SYMBOL_ENTRY *sym = malloc(sizeof(SYMBOL_ENTRY)); 
    sym->id = id; 
    sym->symType = symType; 
    sym->type = type; 
    sym->arguments = args; 
    sym->sibling = NULL; 
    return sym; 
}

SYMBOL_ENTRY *copy_symbol(SYMBOL_ENTRY *entry) {
    SYMBOL_ENTRY *sym = malloc(sizeof(SYMBOL_ENTRY)); 
    char *idcopy = malloc(sizeof(char) * (strlen(entry->id) + 1)); 
    strcpy(idcopy, entry->id); 
    return create_symbol(idcopy, entry->symType, entry->type, NULL); 
}

SYMBOL_TABLE *create_symbol_table(SYMBOL_ENTRY *context) {
    SYMBOL_TABLE *table = malloc(sizeof(SYMBOL_TABLE)); 
    table->next = NULL; 
    table->array = malloc(sizeof(SYMBOL_ENTRY) * 211);
    table->context = context; 
    for (int i = 0; i < 211; i++) {
        table->array[i] = NULL; 
    }
    return table; 
}

SYMBOL_TABLE* push_symbol_table(SYMBOL_TABLE *table, SYMBOL_ENTRY *context) {
    SYMBOL_TABLE *next = create_symbol_table(context);  
    next->next = table;
    return next; 
}

void free_symbol_table(SYMBOL_TABLE *table) {
    for (int i = 0; i < 211; i++) {
        if (table->array[i] != NULL) {
            free_symbol(table->array[i]); 
        }
    }
    free(table->array);
    if (table->context != NULL) {
        free_symbol(table->context); 
    } 
    free(table); 
}

SYMBOL_TABLE* pop_symbol_table(SYMBOL_TABLE *table) {
    if  (table->next == NULL) {
        fprintf(stderr, "Symbol table error, critcal failure\n"); 
        exit(1); 
    }

    SYMBOL_TABLE *temp = table->next; 
    free_symbol_table(table); 
    return temp;
}


void free_symbol_table_chain(SYMBOL_TABLE *head) {
    SYMBOL_TABLE *temp = head; 
    while (head != NULL) {
        temp = head->next; 
        free_symbol_table(head); 
        head = temp; 
    }
}

void insert_sym_table(SYMBOL_TABLE *table, SYMBOL_ENTRY *sym) {
    unsigned index = hash_value(sym->id); 
    if (table->array[index] == NULL) {
        table->array[index] = sym; 
    } else {
        sym->sibling = table->array[index];
        table->array[index] = sym; 
    }

}

SYMBOL_ENTRY *lookup_symbol(SYMBOL_TABLE *table, char *id) {
    SYMBOL_TABLE *walk = table; 
    SYMBOL_ENTRY *res = NULL; 

    while (walk != NULL) {
        unsigned index = hash_value(id); 

        if (walk->array[index] !=  NULL) {
            res = walk->array[index]; 
        
            while (res != NULL && strcmp(res->id, id) != 0) {
                res = res->sibling; 
            }
            
            if (res != NULL && strcmp(res->id, id) == 0) {
                return res; 
            } else {
                res = NULL; 
            }
        }

        walk = walk->next; 
    }

    return res; 
}

SYMBOL_ENTRY *lookup_symbol_local(SYMBOL_TABLE *table, char *id) {
    SYMBOL_ENTRY *res = NULL; 

    unsigned index = hash_value(id); 

    if (table->array[index] !=  NULL) {
        res = table->array[index]; 
        while (res != NULL && strcmp(res->id, id) != 0) {
            res = res->sibling; 
        }
        
        if (res != NULL && strcmp(res->id, id) == 0) {
            return res; 
        } else {
            res = NULL; 
        }
    }    

    return res; 
}
    

void print_symbol_table(SYMBOL_TABLE *table) {
    printf("\n---SYMBOL TABLE DUMP--- \n");
    while (table != NULL) {
        printf("-----------------------\n\n"); 
        if (table->context != NULL) {
            printf("Context: %s ", table->context->id); 
            printf("Type: %d\n\n", table->context->type);
        }
        for (int i = 0; i < 211; i++) {
            if (table->array[i] != NULL) {
                SYMBOL_ENTRY *walk = table->array[i]; 
           
                while (walk != NULL) {
                    printf("%s - type: %d - symtype: %d", walk->id, walk->type, walk->symType); 
                    if (walk->symType == FUNC) {
                        printf(" ARGS: "); 
                        SYMBOL_ENTRY *arg = walk->arguments; 
                        while (arg != NULL) {

                            if (arg->id != NULL) {
                                printf(" -- ARG ID: %s ", arg->id); 
                            }
                            printf(" - type: %d - ", arg->type); 
                            printf("symtype: %d ", arg->symType); 
                            arg = arg->arguments; 
                        }
                    }
                    walk = walk->sibling; 
                    if (walk != NULL) {
                        printf(" ---> "); 
                    }
                }
                printf("\n"); 
            }
        }
        table = table->next; 
    }
    printf("\n------END TABLE DUMP------\n\n");
}

TOKEN_STRUCT *currentToken; 
TOKEN_STRUCT *testToken; 
SYMBOL_TABLE *table;  
SYMBOL_ENTRY *latestDeclaration; 
TYPE latestReturn; 
bool invalidReturn; 


char* match(TOKEN token, char *lex) {
    //printf("Matching: %s\n", lex); 
    if (currentToken->token == token) {
        if (token == SYMBOL || token == KEYWORD) {
            if (strcmp(currentToken->lexeme, lex) != 0) {
                printf("REJECT\n");
                exit(1); 
            }
        } 
        
        char *str = currentToken->lexeme; 
        free(currentToken); 
        currentToken = nextToken(); 
        return str; 
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

    table = create_symbol_table(NULL); 

    currentToken = nextToken();
    program();

    printf("final declaration: %s \n", latestDeclaration->id); 

    if (latestDeclaration != NULL && strcmp(latestDeclaration->id, "main") != 0 ||
     latestDeclaration->type != VOID || latestDeclaration->arguments->type != VOID) {
        printf("REJECT\n");
        exit(1); 
    }

    if (currentToken->token != ENDFILE) {
        printf("REJECT\n"); 
        exit(1); 
    }
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

//This is where we add ids to the symbol table 
void declaration(void) {
    debug("declaration");

    TYPE type = type_specifier(); 
    char *id = match(ID, NULL);

    //If we already have this identifier 
    if (lookup_symbol_local(table, id) != NULL) {
        printf("REJECT\n");
        print_symbol_table(table); 
        exit(1); 
    } 

    SYMBOL_ENTRY *entry = create_symbol(id, 0, type, NULL); 
    declaration_prime(entry); 

    //Void variables, not possible 
    if (type == VOID && entry->symType != FUNC) {
        printf("Cannot have void variables!\n"); 
        printf("REJET\n"); 
        exit(1); 
    }
    insert_sym_table(table, entry); 

    if (entry->symType == FUNC) {
        if (invalidReturn) {
            printf("Invalid return yo\n"); 
            printf("REJECT\n");
            exit(1); 
        }
        if (entry->type != VOID && entry->type != latestReturn) {
            printf("Invalid return yooo\n"); 
            printf("REJECT\n");
            exit(1); 
        }

        latestReturn = ERROR; 
    }

    latestDeclaration = entry;


}

void declaration_prime(SYMBOL_ENTRY *entry) {
    debug("declaration-prime"); 

    if (compareToken(SYMBOL, "(")) {
        match(SYMBOL, "(");
        params(entry); 
        match(SYMBOL, ")");
        entry->symType = FUNC; 
        compound_stmt(entry);            
    } else {
        var_declaration_prime(entry); 
    }
}

//TODO come back to this
void var_declaration(void) {
    debug("var-declaration"); 

    TYPE type = type_specifier(); 
    char *id = match(ID, NULL); 

    if (type == VOID || lookup_symbol_local(table, id) != NULL) {
        printf("POSSIBLE CONFLICT\n");
        printf("REJECT\n");
        exit(1); 
    }

    SYMBOL_ENTRY *entry = create_symbol(id, VAR, type, NULL); 
    var_declaration_prime(entry); 
    insert_sym_table(table, entry); 
} 

void var_declaration_prime(SYMBOL_ENTRY *entry) {
    debug("var-declaration-prime");

     if (compareToken(SYMBOL, ";")) {
        match(SYMBOL, ";"); 
        entry->symType = VAR; 
    } else {
        match(SYMBOL, "[");
        match(NUM, NULL); 
        match(SYMBOL, "]");
        match(SYMBOL, ";"); 
        entry->symType = ARRAY; 
    }
} 

TYPE type_specifier(void) {
    debug("type-specifier"); 

    if (compareToken(KEYWORD, "int")) {
        match(KEYWORD, "int"); 
        return INT; 
    } else {
        match(KEYWORD, "void");
        return VOID;  
    }
} 

void params(SYMBOL_ENTRY *entry) {
    debug("params"); 

    if (compareToken(KEYWORD, "int")) {
        match(KEYWORD, "int"); 
        char *id = match(ID, NULL); 
        SYMBOL_ENTRY *arg = create_symbol(id, VAR, INT, NULL); 
        entry->arguments = arg; 
        param_prime(arg); 
        param_list_prime(arg); 
    } else {
        match(KEYWORD, "void");
        SYMBOL_ENTRY *arg = create_symbol(NULL, VAR, VOID, NULL); 
        entry->arguments = arg;
        params_prime(); 
    }
}

//We should never have any params after void! 
void params_prime(void) {
    debug("params-prime"); 

    if (compareToken(ID, NULL)) {
        printf("REJECT\n"); 
        exit(1); 

        match(ID, NULL); 
        param_prime(NULL); 
        param_list_prime(NULL); 
    }
}

void param_list_prime(SYMBOL_ENTRY *arg) {
    debug("param-list-prime"); 

    if (compareToken(SYMBOL, ",")) {
        match(SYMBOL, ",");
        TYPE type = type_specifier(); 
        char *id = match(ID, NULL);
        //Cannot have null params! 
        if (type == VOID) {
            printf("VOID PARAM! NOT ALLOWED!\n"); 
            printf("REJECT\n");
            exit(1); 
        } 
        SYMBOL_ENTRY *nextArg = create_symbol(id, VAR, type, NULL); 
        arg->arguments = nextArg; 
        param_prime(nextArg); 
        param_list_prime(nextArg); 
    }
} 

void param_prime(SYMBOL_ENTRY *param) {
    debug("param-prime"); 
    if (compareToken(SYMBOL, "[")) {
        param->symType = ARRAY; 
        match(SYMBOL, "[");
        match(SYMBOL, "]"); 
    }
}

// ------------------- HOLD THE LINE ----------------

//Creates a new symbol table and pops it when it's done with it!
void compound_stmt(SYMBOL_ENTRY *entry) {
    debug("compound-stmt");

    match(SYMBOL, "{");
  
    table = push_symbol_table(table, copy_symbol(entry));


    SYMBOL_ENTRY *walk = entry->arguments; 
    while (walk != NULL && walk->type != VOID) {
        insert_sym_table(table, copy_symbol(walk)); 
        walk = walk->arguments; 
    } 

    print_symbol_table(table); 

    local_declarations(); 

    statement_list(); 

    print_symbol_table(table); 
    table = pop_symbol_table(table); 
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
        //DONE
        expression_stmt(); 
    } else if (compareToken(SYMBOL, "{")) {
        //DONE ? 
        compound_stmt(table->context); 
    } else if (compareToken(KEYWORD, "if")) {
        selection_stmt(); 
    } else if (compareToken(KEYWORD, "while")) {
        iteration_stmt(); 
    } else {
        return_stmt(); 
    }
}

//IGNORE RETURN VALUE OF EXPRESSION STMT
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
    TYPE type = expression(); 
    if (type != INT) {
        printf("ERROR! expression must be int\n");
        printf("REJECT\n");
        exit(1); 
    }
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
    TYPE type = expression(); 
    if (type != INT) {
        printf("FAIL ITERATION\n");
        printf("REJET\n");
        exit(1); 
    }
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
        if (table->context->type != VOID) {
            printf("REJECT\n"); 
            exit(1); 
        }
        latestReturn = VOID; 
        match(SYMBOL, ";");
    } else {
        TYPE type = expression(); 
        if (type == table->context->type) {
            latestReturn = type; 
        } else {
            invalidReturn = true; 
        }
        match(SYMBOL, ";"); 
    }
}

TYPE expression(void) {
    debug("expression"); 

        // ( ID NUM use this for expression
    if (compareToken(NUM, NULL)) {
        match(NUM, NULL); 
        TYPE t1 = term_prime(INT); 
        printf("t1: %d\n", t1); 
        TYPE t2 = additive_expression_prime(t1); 
        printf("t2: %d\n", t2); 
        TYPE t3 = simple_expression_prime(t2);

        if (t3 == ERRORTYPE) {
            printf("in this one?\n");
            printf("Expression error\n");
            printf("REJECT\n"); 
            exit(1); 
        }

        return t3;  

    } else if (compareToken(ID, NULL)) {
        char *id = match(ID, NULL); 
        printf("%s\n", id); 
        SYMBOL_ENTRY *entry = lookup_symbol(table, id); 

        if (entry == NULL) {
            printf("Expression error - id not found\n");
            printf("REJECT\n"); 
            exit(1); 
        }
        printf("here- %s\n", entry->id); 
        TYPE resType = expression_prime(entry);

        if (resType == ERRORTYPE) {
            printf("Expression error\n");
            printf("REJECT\n"); 
            exit(1); 
        }

        return resType; 

    } else {
        match(SYMBOL, "(");
        TYPE t1 = expression();
        match(SYMBOL, ")");
        TYPE t2 = term_prime(t1); 
        TYPE t3 = additive_expression_prime(t2);
        TYPE t4 = simple_expression_prime(t3);

        if (t4 == ERRORTYPE) {
            printf("Expression error\n");
            printf("REJECT\n"); 
            exit(1); 
        }
        return t4;  
    }
}

//USED FOR identifiers!
TYPE expression_prime(SYMBOL_ENTRY *entry) {
    debug("expression`");

    if (compareToken(SYMBOL, "=")) {
        if (entry->symType != VAR) {
            return ERRORTYPE; 
        }
        match(SYMBOL, "=");
        return expression();

    } else if (compareToken(SYMBOL, "[")) {
        match(SYMBOL, "[");
        TYPE indexType = expression(); 
        match(SYMBOL, "]");
        if (entry->symType != ARRAY || indexType != INT) {
            return ERRORTYPE; 
        }
        return expression_prime_prime(entry); 

    } else if (compareToken(SYMBOL, "(")) {
        if (entry->symType != FUNC) {
            return ERRORTYPE; 
        }
    
        match(SYMBOL, "(");
        bool argsMatched = args(entry);
        printf("argsmatched: %d\n", argsMatched);
        match(SYMBOL, ")");

        if (!argsMatched) {
            return ERRORTYPE; 
        }

        TYPE t1 = term_prime(entry->type);
        TYPE t2 = additive_expression_prime(t1); 
        TYPE t3 = simple_expression_prime(t2);
        return t3; 

    } else {
        TYPE t1 = term_prime(entry->type);
        TYPE t2 = additive_expression_prime(t1);
        printf("t2  %d\n", t2);
        TYPE t3 = simple_expression_prime(t2);
        printf("t3 %d\n", t3);
        return t3; 
    }
}

//Also used for identifiers 
TYPE expression_prime_prime(SYMBOL_ENTRY *entry) {
    debug("expression``");

    if (compareToken(SYMBOL, "=")) {
        match(SYMBOL, "=");
        TYPE type = expression();
        if (type != entry->type) {
            return ERRORTYPE; 
        } else {
            return type; 
        }
    } else {
        TYPE t1 = term_prime(entry->type); 
        TYPE t2 = additive_expression_prime(t1);
        return simple_expression_prime(t2); 
    }
}

TYPE simple_expression_prime(TYPE type) {
    debug("simple-expression`");

    if (compareToken(SYMBOL, "<=") || compareToken(SYMBOL, "<") || compareToken(SYMBOL, ">") || compareToken(SYMBOL, ">=")
    || compareToken(SYMBOL, "==") || compareToken(SYMBOL, "!=")) {
        if (type == VOID || type == ERRORTYPE) {
            return ERRORTYPE; 
        }
        printf("Relative\n");
        relop(); 

        TYPE typeRes = additive_expression();

        printf("the type %d\n", typeRes);
        if (typeRes == ERRORTYPE || typeRes == VOID) {
            printf("How???\n"); 
            return ERRORTYPE; 
        } else {
            return typeRes; 
        }
    }

    return type; 
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

TYPE additive_expression(void) {
    debug("additive-expression"); 

    TYPE type1 = term();
    TYPE type2 = additive_expression_prime(type1); 
    return type2; 
}

TYPE additive_expression_prime(TYPE type) {
    debug("additive-expression`");
    printf("Here again with type: %d\n", type); 
    if (compareToken(SYMBOL, "+") || compareToken(SYMBOL, "-")) {
        if (type == VOID || type == ERRORTYPE) {
            return ERRORTYPE; 
        }

        addop();

        TYPE termType = term(); 

        printf("%d\n", termType); 

        if (termType == VOID || termType == ERRORTYPE) {
            printf("This is bad\n"); 
            return ERRORTYPE; 
        }

        TYPE type2 = additive_expression_prime(termType); 
        return type2; 
    }

    return type; 

}

void addop(void) {
    debug("addop");

    if (compareToken(SYMBOL, "+")) {
        match(SYMBOL, "+");
    } else {
        match(SYMBOL, "-"); 
    }
}

TYPE term(void) {
    debug("term");

    TYPE type1 = factor();
    TYPE type2 = term_prime(type1);

    return type2; 
}

TYPE term_prime(TYPE type) {
    debug("term`");

    if (compareToken(SYMBOL, "*") || compareToken(SYMBOL, "/")) {
        if (type == VOID || type == ERRORTYPE) {
            return ERRORTYPE; 
        }
        mulop();
        //result of this must be an int! 
        TYPE factorType = factor();
        if (factorType == VOID || factorType == ERRORTYPE) {
            return ERRORTYPE; 
        }

        return term_prime(factorType);
    }

    return type; 
}

void mulop(void) {
    debug("mulop");

    if (compareToken(SYMBOL, "*")) {
        match(SYMBOL, "*");
    } else {
        match(SYMBOL, "/");
    }
}


//Actual values! 
TYPE factor(void) {
    debug("factor");

    if (compareToken(NUM, NULL)) {
        match(NUM, NULL); 
        return INT; 
    } else if (compareToken(ID, NULL)) {
        char *id = match(ID, NULL); 
        SYMBOL_ENTRY *entry = lookup_symbol(table, id); 
        if (entry == NULL) {
            return ERRORTYPE; 
        }
        TYPE type = factor_prime(entry); 
        return type; 
    } else {
        match(SYMBOL, "(");
        TYPE type = expression(); 
        match(SYMBOL, ")");
        return type; 
    }
}

TYPE factor_prime(SYMBOL_ENTRY *entry) {
    debug("factor`");

    if (compareToken(SYMBOL, "[")) {
        if (entry->symType != ARRAY) {
            return ERRORTYPE; 
        }

        match(SYMBOL, "[");
        TYPE type = expression(); 
        if (type != INT) {
            return ERRORTYPE; 
        }
        match(SYMBOL, "]");
        return entry->type; 
    } else if (compareToken(SYMBOL, "(")) {
        if (entry->symType != FUNC) {
            return ERRORTYPE; 
        }
        printf("here: %d\n", entry->id);
        match(SYMBOL, "(");
        bool paramsMatch = args(entry); 
        match(SYMBOL, ")");
        if (paramsMatch) {
            return entry->type; 
        } else {
            return ERRORTYPE; 
        }
    }
}

bool args(SYMBOL_ENTRY *entry) {
    debug("args");

    if (compareToken(ID, NULL) || compareToken(NUM, NULL) || compareToken(SYMBOL, "(")) {
        return arg_list(entry); 
    }

    if (entry->arguments->type == VOID) {
        return true; 
    } else {
        return false; 
    }
} 

bool arg_list(SYMBOL_ENTRY *entry) {
    debug("args-list");

    TYPE type = expression(); 
    if (type == entry->arguments->type) {
        printf("%s\n", entry->arguments->arguments->id ); 
        return arg_list_prime(entry->arguments->arguments); 
    } else {
        return false; 
    }
}

bool arg_list_prime(SYMBOL_ENTRY *entry) { 
    debug("arg-list`");

    if (compareToken(SYMBOL, ",")) {
        match(SYMBOL, ","); 
        TYPE type = expression(); 
        if (entry != NULL && type == entry->type) {
            return arg_list_prime(entry->arguments); 
        } else {
            return false; 
        }
    }

    if (entry == NULL) {
        printf("hereeee\n"); 
        return true; 
    } else {
        return false; 
    }
}