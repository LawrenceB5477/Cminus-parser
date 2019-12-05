#include "parser.h"

extern FILE* inputFile; 
int lineCount = 1; 
int tempNum = 0; 
char *lastOperand = NULL; 
FILE *codeFile = NULL; 
int bp = 0; 
int bpArray[200]; 
typedef enum {
    CMPL,
    CMPLE,
    CMPG,
    CMPGE,
    CMPE,
    CMPNE,
    CMPNONE
} CMP_RES;

CMP_RES lastCompare = CMPNONE; 

void set_operand(char *string) {
    if (lastOperand != NULL) {
        free(lastOperand); 
    }
    if (string != NULL) {
        lastOperand = malloc(strlen(string) + 1);
        strcpy(lastOperand, string);
    }
}


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
TYPE latestReturn = ERROR; 
bool invalidReturn; 


char* match(TOKEN token, char *lex) {
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

    //printf("final declaration: %s \n", latestDeclaration->id); 

    if (latestDeclaration != NULL && strcmp(latestDeclaration->id, "main") != 0 ||
     latestDeclaration->type != VOID || latestDeclaration->arguments->type != VOID || lookup_symbol(table, "main")->symType != FUNC) {
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
    codeFile = fopen("tempcodegen", "w"); 
    if (codeFile == NULL) {
        printf("REJECT\n");
        exit(1); 
    }
    declaration_list(); 
    fclose(codeFile); 
    FILE *tempInput =  fopen("tempcodegen", "r");  
    FILE *codeOut = fopen("codegen", "w"); 

    if (tempInput == NULL || codeOut == NULL) {
        printf("REJECT\n");
        exit(1); 
    }

    char buffer[100]; 
    while (fgets(buffer, 100, tempInput) != NULL) {
        char *bpPos = strstr(buffer, "bp"); 
        char number[20]; 
        

        if (strstr(buffer, "jmp") != NULL && bpPos != NULL) {
            char *walk = bpPos + 2; 
            int index = 0;
            while (*walk) {
                number[index] = *walk; 
                index++; 
                walk++; 
            }
            number[index] = '\0'; 
            int bpNumber = atoi(number); 
            int bufSize = bpPos - buffer; 
            char outBuffer[bufSize + 1]; 
            strncpy(outBuffer, buffer, bufSize);
            outBuffer[bufSize] = '\0'; 
            outBuffer[strlen(outBuffer) + 1] = '\0';
            fprintf(codeOut, "%s%d\n", outBuffer, bpArray[bpNumber]); 
        } else {
            fputs(buffer, codeOut); 
        }
    }
    fclose(tempInput); 
    fclose(codeOut); 
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
        exit(1); 
    } 

    SYMBOL_ENTRY *entry = create_symbol(id, 0, type, NULL); 
    declaration_prime(entry); 

    //Void variables, not possible 
    if (type == VOID && entry->symType != FUNC) {
        //printf("Cannot have void variables!\n"); 
        printf("REJECT\n"); 
        exit(1); 
    }

    if (entry->symType == FUNC) {
        if (invalidReturn) {
            printf("REJECT\n");
            exit(1); 
        }
        if (entry->type != VOID && entry->type != latestReturn) {
            printf("REJECT\n");
            exit(1); 
        }

        latestReturn = ERROR; 
    }

    // add declaration here 

    latestDeclaration = entry;
}

void declaration_prime(SYMBOL_ENTRY *entry) {
    debug("declaration-prime"); 

    if (compareToken(SYMBOL, "(")) {
        entry->symType = FUNC; 
        insert_sym_table(table, entry); 
        match(SYMBOL, "(");
        params(entry); 
        match(SYMBOL, ")");
       
        //print the function 
        char *functype = NULL; 
        if (entry->type == INT) {
            functype = "int"; 
        } else {
            functype = "void"; 
        }

        //get argument information 
        int argCount = 0; 
        if (entry->arguments->type != VOID) {
            SYMBOL_ENTRY *walk = entry->arguments; 
            while (walk != NULL) {
                argCount++; 
                walk = walk->arguments; 
            }
        }

        fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10d\n", lineCount++, "func", entry->id, functype, argCount); 
        
        if (entry->arguments->type != VOID) {
            SYMBOL_ENTRY *walk = entry->arguments; 
            while (walk != NULL) {
                fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "param", "", "", walk->id); 
                walk = walk->arguments; 
            }
            walk = entry->arguments; 
            while (walk != NULL) {
                fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "alloc", "4", "", walk->id); 
                walk = walk->arguments; 
            }
        }
        compound_stmt(entry);            
        fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "end", entry->id, "", ""); 
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
        printf("REJECT\n");
        exit(1); 
    }

    SYMBOL_ENTRY *entry = create_symbol(id, VAR, type, NULL); 
    var_declaration_prime(entry); 
} 

void var_declaration_prime(SYMBOL_ENTRY *entry) {
    debug("var-declaration-prime");

     if (compareToken(SYMBOL, ";")) {
        match(SYMBOL, ";"); 
        entry->symType = VAR; 
        insert_sym_table(table, entry); 
        
        fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "alloc", "4", "", entry->id); 

    } else {
        match(SYMBOL, "[");
        char *num = match(NUM, NULL); 
        match(SYMBOL, "]");
        match(SYMBOL, ";"); 
        entry->symType = ARRAY; 
            insert_sym_table(table, entry); 
        
        int numb = atoi(num); 
        numb *= 4; 

        fprintf(codeFile, "%-5d %-10s %-10d %-10s %-10s\n", lineCount++, "alloc", numb, "", entry->id); 
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

//DONE
void compound_stmt(SYMBOL_ENTRY *entry) {
    debug("compound-stmt");

    match(SYMBOL, "{");
    fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "block", "", "", ""); 

    table = push_symbol_table(table, copy_symbol(entry));

    SYMBOL_ENTRY *walk = entry->arguments; 
    while (walk != NULL && walk->type != VOID) {
        insert_sym_table(table, copy_symbol(walk)); 
        walk = walk->arguments; 
    } 

    local_declarations(); 

    statement_list(); 

    table = pop_symbol_table(table); 

    fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++,  "end block", "", "", ""); 
    match(SYMBOL, "}");
}

//DONE
void local_declarations(void) {
    debug("local-declarations"); 

    if (compareToken(KEYWORD, "void") || compareToken(KEYWORD, "int")) {
        var_declaration(); 
        local_declarations(); 
    }
}

//DONE
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

//DONE
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
        printf("REJECT\n");
        exit(1); 
    }
    match(SYMBOL, ")");

    int saveBp1 = bp; 
    if (lastCompare == CMPL) {
        fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmpl", lastOperand, "", bp++); 
    } else if (lastCompare == CMPLE) {
        fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmple", lastOperand, "", bp++); 
    } else if (lastCompare == CMPG) {
        fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmpg", lastOperand, "", bp++); 
    } else if (lastCompare == CMPGE) {
        fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmpge", lastOperand, "", bp++); 
    } else if (lastCompare == CMPE) {
        fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmpe", lastOperand, "", bp++); 
    } else if (lastCompare == CMPNE) {
        fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmpne", lastOperand, "", bp++); 
    }

    statement(); 
    int saveBp = bp; 
    fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmp", "", "", bp++); 
    
    int elseline = lineCount; 
    bpArray[saveBp1] = elseline;
    selection_stmt_prime(saveBp); 
}

void selection_stmt_prime(int bp) {
    debug("selection-stmt`");
    
    if (compareToken(KEYWORD, "else")) {
        match(KEYWORD, "else");
        statement(); 
        bpArray[bp] = lineCount;
    }
}

void iteration_stmt(void) {
    debug("iteration-stmt"); 

    match(KEYWORD, "while"); 
    match(SYMBOL, "(");
    int startWhile = lineCount; 

    TYPE type = expression(); 
    if (type != INT) {
        printf("REJECT\n");
        exit(1); 
    }
    match(SYMBOL, ")");
    if (lastCompare == CMPNONE) {
        printf("REJECT\n"); 
        exit(1); 
    }

    int saveBp = bp; 

    if (lastCompare == CMPL) {
        fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmpl", lastOperand, "", bp++); 
    } else if (lastCompare == CMPLE) {
        fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmple", lastOperand, "", bp++); 
    } else if (lastCompare == CMPG) {
        fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmpg", lastOperand, "", bp++); 
    } else if (lastCompare == CMPGE) {
        fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmpge", lastOperand, "", bp++); 
    } else if (lastCompare == CMPE) {
        fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmpe", lastOperand, "", bp++); 
    } else if (lastCompare == CMPNE) {
        fprintf(codeFile, "%-5d %-10s %-10s %-10s bp%-8d\n", lineCount++, "jmpne", lastOperand, "", bp++); 
    }

    statement(); 
    fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10d\n", lineCount++, "jmp", "", "", startWhile); 
    bpArray[saveBp] = lineCount;
}

//RETURN
void return_stmt(void) {
    debug("return-stmt"); 

    match(KEYWORD, "return"); 
    return_stmt_prime(); 
}

//DONE
void return_stmt_prime(void) {
    debug("return-stmt`");

    if (compareToken(SYMBOL, ";")) {
        if (table->context->type != VOID) {
            printf("REJECT\n"); 
            exit(1); 
        }
        latestReturn = VOID; 

        fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "return", "", "", ""); 
        match(SYMBOL, ";");
    } else {
        TYPE type = expression(); 
        if (table->context->type != VOID && type == table->context->type) {
            latestReturn = type; 
        } else {
            invalidReturn = true; 
        }

        fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "return", "", "", lastOperand); 
        match(SYMBOL, ";"); 
    }
}

//DONE
TYPE expression(void) {
    debug("expression"); 

        // ( ID NUM use this for expression
    if (compareToken(NUM, NULL)) {
        set_operand(match(NUM, NULL)); 
        TYPE t1 = term_prime(INT); 
        TYPE t2 = additive_expression_prime(t1); 
        TYPE t3 = simple_expression_prime(t2);

        if (t3 == ERRORTYPE) {
            printf("REJECT\n"); 
            exit(1); 
        }

        return t3;  

    } else if (compareToken(ID, NULL)) {
        char *id = match(ID, NULL); 
        SYMBOL_ENTRY *entry = lookup_symbol(table, id); 

        if (entry == NULL ) {
            printf("REJECT\n"); 
            exit(1); 
        }

        //TODO here 
        set_operand(id); 
        TYPE resType = expression_prime(entry);

        if (resType == ERRORTYPE) {
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
            printf("REJECT\n"); 
            exit(1); 
        }
        return t4;  
    }
}

//DONE
TYPE expression_prime(SYMBOL_ENTRY *entry) {
    debug("expression`");

    if (compareToken(SYMBOL, "=")) {
        if (entry->symType != VAR) {
            return ERRORTYPE; 
        }
        char op1[strlen(lastOperand) + 1];
        strcpy(op1, lastOperand); 

        match(SYMBOL, "=");
        TYPE res = expression();
        if (res == VOID || res == ERROR || res == INTARRAY) {
            printf("REJECT\n");
            exit(1); 
        }

        fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "assign", lastOperand, "", op1); 
        return res; 
    } else if (compareToken(SYMBOL, "[")) {
        char op1[strlen(lastOperand) + 1];
        strcpy(op1, lastOperand); 

        match(SYMBOL, "[");
        TYPE indexType = expression(); 
        match(SYMBOL, "]");
        
        
        //code gen 
        char tempName[7];
        strcpy(tempName, "_t"); 
        char num[5]; 
        snprintf(num, 7, "%d", tempNum++); 
        strcat(tempName, num);

        if (entry->symType != ARRAY || indexType != INT) {
            return ERRORTYPE; 
        }

        fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "mul", lastOperand, "4", tempName);
        set_operand(tempName); 


        char tempName2[7];
        strcpy(tempName2, "_t"); 
        char num2[5]; 
        snprintf(num2, 7, "%d", tempNum++); 
        strcat(tempName2, num2);

        fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "disp", op1, lastOperand, tempName2);

        set_operand(tempName2); 

        return expression_prime_prime(entry); 

    } else if (compareToken(SYMBOL, "(")) {
        if (entry->symType != FUNC) {
            return ERRORTYPE; 
        }
    
        int argCount = 0; 
        SYMBOL_ENTRY *walk = entry->arguments; 
        while (walk != NULL) {
            argCount++; 
            walk = walk->arguments; 
        }

        char op1[strlen(lastOperand) + 1];
        strcpy(op1, lastOperand);

        match(SYMBOL, "(");
        bool argsMatched = args(entry);
        match(SYMBOL, ")");

        if (!argsMatched) {
            return ERRORTYPE; 
        }

        char tempName[7];
        strcpy(tempName, "_t");
        char num[5];
        snprintf(num, 5, "%d", tempNum++);
        strcat(tempName, num);

        fprintf(codeFile, "%-5d %-10s %-10s %-10d %-10s\n", lineCount++, "call", op1, argCount, tempName);
        set_operand(tempName); 

        TYPE t1 = term_prime(entry->type);
        TYPE t2 = additive_expression_prime(t1); 
        TYPE t3 = simple_expression_prime(t2);
        return t3; 

    } else {
        if (entry->symType != VAR && entry->symType != ARRAY) {
            printf("REJECT\n");
            exit(1);
        }

        if (entry->symType == ARRAY) {
            TYPE t1 = term_prime(VOID);
            TYPE t2 = additive_expression_prime(t1);
            TYPE t3 = simple_expression_prime(t2);
            if (t3 != VOID) {
                printf("REJECT\n");
                exit(1); 
            } else {
                return INTARRAY; 
            }
        } else {
            TYPE t1 = term_prime(entry->type);
            TYPE t2 = additive_expression_prime(t1);
            TYPE t3 = simple_expression_prime(t2);
            return t3; 
        }
      
    }
}

//DONE 
TYPE expression_prime_prime(SYMBOL_ENTRY *entry) {
    debug("expression``");

    if (compareToken(SYMBOL, "=")) {
        match(SYMBOL, "=");

        char op1[strlen(lastOperand) + 1];
        strcpy(op1, lastOperand); 

        TYPE type = expression();

        if (type != entry->type) {
            return ERRORTYPE; 
        } else {
            fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "assign", lastOperand, "", op1); 
            return type; 
        }
    } else {
        TYPE t1 = term_prime(entry->type); 
        TYPE t2 = additive_expression_prime(t1);
        return simple_expression_prime(t2); 
    }
}

//DONE
TYPE simple_expression_prime(TYPE type) {
    debug("simple-expression`");

    if (compareToken(SYMBOL, "<=") || compareToken(SYMBOL, "<") || compareToken(SYMBOL, ">") || compareToken(SYMBOL, ">=")
    || compareToken(SYMBOL, "==") || compareToken(SYMBOL, "!=")) {
        if (type == VOID || type == ERRORTYPE) {
            return ERRORTYPE; 
        }
        char op1[strlen(lastOperand) + 1];
        strcpy(op1, lastOperand); 

        relop(); 

        TYPE typeRes = additive_expression();

        if (typeRes == ERRORTYPE || typeRes == VOID) {
            return ERRORTYPE; 
        } else {

            char tempName[7];
            strcpy(tempName, "_t"); 
            char num[5]; 
            snprintf(num, 7, "%d", tempNum++); 
            strcat(tempName, num);
        
            fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "comp", op1, lastOperand, tempName); 
            set_operand(tempName); 
            return typeRes; 
        }
    }

    return type; 
}

//DONE
void relop(void) {
    debug("relop");

    if (compareToken(SYMBOL, "<=")) {
        lastCompare = CMPG;
        match(SYMBOL, "<=");
    } else if (compareToken(SYMBOL, "<")) {
        lastCompare = CMPGE;
        match(SYMBOL, "<");
    } else if (compareToken(SYMBOL, ">")) {
        lastCompare = CMPLE;
        match(SYMBOL, ">"); 
    } else if (compareToken(SYMBOL, ">=")) {
        lastCompare = CMPL;
        match(SYMBOL, ">=");
    } else if (compareToken(SYMBOL, "==")) {
        lastCompare = CMPNE;
        match(SYMBOL, "=="); 
    } else {
        lastCompare = CMPE;
        match(SYMBOL, "!=");
    }
}

//DONE
TYPE additive_expression(void) {
    debug("additive-expression"); 

    TYPE type1 = term();
    TYPE type2 = additive_expression_prime(type1); 
    return type2; 
}

//DONE
TYPE additive_expression_prime(TYPE type) {
    debug("additive-expression`");
    if (compareToken(SYMBOL, "+") || compareToken(SYMBOL, "-")) {
        if (type == VOID || type == ERRORTYPE) {
            return ERRORTYPE; 
        }
        
        bool add = compareToken(SYMBOL, "+"); 
        char op1[strlen(lastOperand) + 1];
        strcpy(op1, lastOperand); 

        addop();

        TYPE termType = term(); 

        if (termType == VOID || termType == ERRORTYPE) {
            return ERRORTYPE; 
        }

        //code gen 
        char tempName[7];
        strcpy(tempName, "_t"); 
        char num[5]; 
        snprintf(num, 7, "%d", tempNum++); 
        strcat(tempName, num);
        
        if (add) {
            fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "add", op1, lastOperand, tempName); 
        } else {
            fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "sub", op1, lastOperand, tempName); 
        }

        set_operand(tempName); 

        TYPE type2 = additive_expression_prime(termType); 
        return type2; 
    }

    return type; 

}

//DONE
void addop(void) {
    debug("addop");

    if (compareToken(SYMBOL, "+")) {
        match(SYMBOL, "+");
    } else {
        match(SYMBOL, "-"); 
    }
}

//DONE
TYPE term(void) {
    debug("term");

    TYPE type1 = factor();
    TYPE type2 = term_prime(type1);

    return type2; 
}

//DONE
TYPE term_prime(TYPE type) {
    debug("term`");

    if (compareToken(SYMBOL, "*") || compareToken(SYMBOL, "/")) {
        bool mul = compareToken(SYMBOL, "*"); 
        if (type == VOID || type == ERRORTYPE) {
            return ERRORTYPE; 
        }
        char op1[strlen(lastOperand) + 1];
        strcpy(op1, lastOperand); 
        
        mulop();
        //result of this must be an int! 
        TYPE factorType = factor();
        if (factorType == VOID || factorType == ERRORTYPE) {
            return ERRORTYPE; 
        
        }
        char tempName[7];
        strcpy(tempName, "_t");
        char num[5];
        snprintf(num, 7, "%d", tempNum++);
        strcat(tempName, num);

        //generate mulop code! 
        if (mul) {
            fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "mul", op1, lastOperand, tempName);
        } else {
            fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "div", op1, lastOperand, tempName);
        }

        set_operand(tempName); 
        return term_prime(factorType);
    }

    return type; 
}

//DONE
void mulop(void) {
    debug("mulop");

    if (compareToken(SYMBOL, "*")) {
        match(SYMBOL, "*");
    } else {
        match(SYMBOL, "/");
    }
}


//DONE
TYPE factor(void) {
    debug("factor");

    if (compareToken(NUM, NULL)) {
        char *res = match(NUM, NULL); 
        set_operand(res);

        return INT; 
    } else if (compareToken(ID, NULL)) {
        char *id = match(ID, NULL); 
        SYMBOL_ENTRY *entry = lookup_symbol(table, id); 
        if (entry == NULL) {
            return ERRORTYPE; 
        }

        set_operand(id); 
        TYPE type = factor_prime(entry); 
        return type; 
    } else {
        match(SYMBOL, "(");
        TYPE type = expression(); 
        match(SYMBOL, ")");
        return type; 
    }
}

//DONE
TYPE factor_prime(SYMBOL_ENTRY *entry) {
    debug("factor`");
    

    if (compareToken(SYMBOL, "[")) {
        if (entry->symType != ARRAY) {
            return ERRORTYPE; 
        }

        char op1[strlen(lastOperand) + 1];
        strcpy(op1, lastOperand);

        match(SYMBOL, "[");
        TYPE type = expression(); 
        if (type != INT) {
            return ERRORTYPE; 
        }
        match(SYMBOL, "]");

        char tempName[7];
        strcpy(tempName, "_t");
        char num[5];
        snprintf(num, 5, "%d", tempNum++);
        strcat(tempName, num);
        
        fprintf(codeFile, "%-5d %-10s %-10s %-10d %-10s\n", lineCount++, "mul", lastOperand, 4, tempName);
        set_operand(tempName);
  
        char tempName2[7];
        strcpy(tempName2, "_t");
        char num2[5];
        snprintf(num2, 7, "%d", tempNum++);
        strcat(tempName2, num2);
 
        fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "disp", op1, lastOperand, tempName2);
 
        set_operand(tempName2);
        return entry->type; 
    } else if (compareToken(SYMBOL, "(")) {
        if (entry->symType != FUNC) {
            return ERRORTYPE; 
        }
        SYMBOL_ENTRY *walk = entry->arguments;
        int argCount = 0; 
        while (walk != NULL) {
            argCount++; 
            walk = walk->arguments; 
        }

        char op1[strlen(lastOperand) + 1];
        strcpy(op1, lastOperand);

        match(SYMBOL, "(");
        bool paramsMatch = args(entry); 
        match(SYMBOL, ")");
        if (paramsMatch) {
            char tempName[7];
            strcpy(tempName, "_t");
            char num[5];
            snprintf(num, 5, "%d", tempNum++);
            strcat(tempName, num);

            fprintf(codeFile, "%-5d %-10s %-10s %-10d %-10s\n", lineCount++, "call", op1, argCount, tempName);
            set_operand(tempName); 
            return entry->type; 
        } else {
            return ERRORTYPE; 
        }
    }
}

//DONE
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

//DONE
bool arg_list(SYMBOL_ENTRY *entry) {
    debug("args-list");

    TYPE type = expression(); 
    fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "arg", "", "", lastOperand);
    if ((entry->arguments->symType == ARRAY && type == INTARRAY) || (entry->arguments->symType != ARRAY && type == entry->arguments->type)) {
        return arg_list_prime(entry->arguments->arguments); 
    } else {
        return false; 
    }
}

//DONE
bool arg_list_prime(SYMBOL_ENTRY *entry) { 
    debug("arg-list`");

    if (compareToken(SYMBOL, ",")) {
        match(SYMBOL, ","); 
        TYPE type = expression(); 
        fprintf(codeFile, "%-5d %-10s %-10s %-10s %-10s\n", lineCount++, "arg", "", "", lastOperand);
        if (entry != NULL && ((entry->symType != ARRAY && type == entry->type) || (entry->symType == ARRAY && type == INTARRAY))) {
            return arg_list_prime(entry->arguments); 
        } else {
            return false; 
        }
    }

    if (entry == NULL) {
        return true; 
    } else {
        return false; 
    }
}
