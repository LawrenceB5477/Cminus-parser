#include "lexer.h"
//Global variables for buffer manipulation 
//Buffer for the current line of input 
char *buffer = NULL;
//Current position in the buffer, the character that is being processed 
unsigned bufferPos = 0; 
//File input is coming from 
FILE* inputFile = NULL; 

//Function definitions -------------------------------
void freeTokenStruct(TOKEN_STRUCT *token) {
    free(token->lexeme); 
    free(token); 
}

//Prints a token struct 
void printToken(TOKEN_STRUCT *token) {
    switch (token->token) {
        case ID: 
            printf("ID: ");
            break; 
        case NUM:
            printf("NUM: ");
            break;
        case KEYWORD: 
            printf("KEYWORD: ");
            break; 
        case SYMBOL: 
            //printf("SYMBOL: ");
            break; 
        case ERROR: 
            printf("ERROR: "); 
            break; 
        case ENDFILE: 
            //shouldn't be possible...
            break; 
        default:
            break; 
    }
    if (token->token != ENDFILE) {
        printf("%s\n", token->lexeme); 
    }
}

//Utility function to fill out the lexeme of a token 
void fillLexeme(TOKEN_STRUCT *token, int start, int stop) {
    int size = stop - start + 1; 
    char *lexeme = malloc(sizeof(char) * size); 
    int i = start; 
    for (; i < stop; i++) {
        lexeme[i - start] = buffer[i]; 
    }
    lexeme[i - start] = '\0'; 
    token->lexeme = lexeme; 
}

//Utility function to see if character is part of the language 
int isValidCharacter(int c) {
    int valid = FALSE; 
    char test[] = {c, '\0'};
    if (isalnum(c) || isspace(c) || strstr("+-/*><=!;,(){}[]", test) != NULL) {
        valid = TRUE;
    }
    return valid; 
}

//Read the next line of input into the buffer, adding a null character to the end (makes line printable)
//Returns EOF when the end of the file is reached 
int readNextLine(void) {
    int bufferCapacity = 100; 
    int bufferSize = 0; 
    bufferPos = 0; 
 
    char *temp = realloc(buffer, sizeof(char) * 100); 

    if (temp == NULL) {
        fprintf(stderr, "Critical error allocating buffer memory\n");
        exit(1); 
    }
    
    buffer = temp; 

    int current = fgetc(inputFile); 

    //Handle if we are at the end of the file 
    if (current == EOF) {
        return EOF; 
    }

    while (current != '\n' && current != EOF) {
        //Expand buffer if needed 
        if (bufferSize == bufferCapacity) {
            bufferCapacity *= 2; 
            char *newBuffer = realloc(buffer, sizeof(char) * bufferCapacity); 
            if (newBuffer == NULL) {
                fprintf(stderr, "Critical error, buffer memory used up\n"); 
                exit(1); 
            } 
            buffer = newBuffer; 
        }
        buffer[bufferSize++] = current; 
        current = fgetc(inputFile); 
    }


    //Make the buffer suitable for printing 
    char *tempBuffer = realloc(buffer, sizeof(char) * bufferSize + 1); 
    if (!tempBuffer) {
        fprintf(stderr, "Critical error, buffer memory used up\n"); 
        exit(1); 
    }
    buffer = tempBuffer; 
    buffer[bufferSize] = '\0';

    //We want an EOF to be recognized by itself 
    if (current == EOF) {
        ungetc(current, inputFile); 
    } 

    return 1; 
}

//Skips all white space, setting bufferpos to the next non-whitespace character, or returns EOF if needed 
int skipSpace() {
    while (isspace(buffer[bufferPos])) {
        bufferPos++; 
    }

    //If we reached the end of the line...
    if (buffer[bufferPos] == '\0') {
        int result = readNextLine(); 
        while (result != EOF) {
            printf("INPUT: %s\n", buffer); 
            
            //Chew up all beginning white space 
            while (isspace(buffer[bufferPos])) {
                 bufferPos++; 
            }
            if (buffer[bufferPos] == '\0') {
                result = readNextLine(); 
            } else {
                break; 
            }
        }

        if (result == EOF) {
            return EOF; 
        }
    }

    return 0;
}

TOKEN_STRUCT* nextToken(void) {
    TOKEN_STRUCT *token = malloc(sizeof(TOKEN_STRUCT)); 

    //Initialize the buffer 
    if (buffer == NULL) {
        if (readNextLine() == EOF) {
            token->token = ENDFILE;
            return token; 
        } else {
            printf("INPUT: %s\n", buffer); 
        }
    }

    //Set buffer position to the first non-whitespace character 
    if (skipSpace() == EOF) {
        token->token = ENDFILE; 
        return token; 
    }
   
    STATE state = START; 
    int start = bufferPos; 

    //Main state machine logic 
    while (state != FINISH) {
        int current = buffer[bufferPos]; 
        int advance = TRUE; 

        switch (state) {
            case START: 
                if (!isValidCharacter(current)) {
                    state = ERRORSTATE; 
                    token->token = ERROR; 
                } else if (isdigit(current)) { 
                    state = INNUM; 
                    token->token = NUM; 
                } else if (isalpha(current)) {
                    state = INID; 
                    token->token = ID; 
                } else if (current == '<') {
                    token->token = SYMBOL; 
                    state = LT; 
                } else if (current == '>') {
                    token->token = SYMBOL; 
                    state = GT;
                } else if (current == '=') {
                    token->token = SYMBOL; 
                    state = EQ;
                } else if (current == '!') {
                    token->token = SYMBOL; 
                    state = NOT; 
                } else if (current == '/') {
                    token->token = SYMBOL; 
                    state = FWDSLASH; 
                } else {
                    //Handles all single character tokens 
                    state = FINISH;
                    token->token = SYMBOL; 
                }
                break; 
            case INNUM: 
                if (!isdigit(current)) {
                    advance = FALSE; 
                    state = FINISH; 
                } 
                break; 
            case INID: 
                if (!isalpha(current)) {
                    advance = FALSE; 
                    state = FINISH; 
                }
                break; 
            // >, <, and = may be part of a two character symbol
            case LT:
            case GT:
            case EQ: 
                if (current == '=') {
                    state = FINISH; 
                } else {
                    advance = 0; 
                    state = FINISH; 
                }
                break; 
            case NOT: 
                if (current == '=') {
                    state = FINISH; 
                } else {
                    state = FINISH; 
                    advance = 0; 
                    token->token = ERROR; 
                }
                break; 
            case FWDSLASH: 
                if (current == '/') {
                    state = LINECMMNT; 
                } else if (current == '*') {
                    state = BLCKCMMNT; 
                } else {
                    state = FINISH; 
                    advance = 0; 
                }
                break; 
            case LINECMMNT: 
                //Get to the end of the line 
                while (buffer[bufferPos] != '\0') {
                    bufferPos++; 
                }

                //Reset the token scanning 
                if (skipSpace() == EOF) {
                    token->token = ENDFILE; 
                    return token; 
                }; 

                start = bufferPos; 
                advance = FALSE;
                state = START; 
                break; 
            case BLCKCMMNT: 
                //Chew up characters until * or EOF is reached  
                while (TRUE) {
                    if (buffer[bufferPos] == '\0') {
                        //Reached EOF while in block comment 
                        if (skipSpace() == EOF) {
                            token->token = ENDFILE; 
                            return token; 
                        }; 
                    } else if (buffer[bufferPos] == '*') {
                        state = ASTER; 
                        break; 
                    } else {
                        bufferPos++; 
                    }
                }
                break; 
            case ASTER: 
                while (current == '*') {
                    bufferPos++;
                    current = buffer[bufferPos];
                }

                //End of block comment, rest token scan 
                if (current == '/') {
                    bufferPos++; 

                    if (skipSpace() == EOF) {
                        token->token = ENDFILE; 
                        return token; 
                    }; 

                    //Reset the token scanning 
                    start = bufferPos; 
                    advance = FALSE;
                    state = START; 
                } else {
                    state = BLCKCMMNT; 
                }
                break; 
            case ERRORSTATE: 
                if (isspace(current) || current == '\0') {
                    advance = 0;
                    state = FINISH; 
                }
                break; 
            case FINISH: 
                //shouldn't be possible...
                break; 
        }

        //Advance bufferpos
        if (advance) {
            bufferPos++; 
        }
    }

    fillLexeme(token, start, bufferPos); 

    //Check for keywords 
    if (token->token == ID) {
        char *lex = token->lexeme; 
        if (strcmp(lex, "else") == 0 || strcmp(lex, "if") == 0 
        || strcmp(lex, "int") == 0 || strcmp(lex, "return") == 0
        || strcmp(lex, "void") == 0 || strcmp(lex, "while") == 0) {
            token->token = KEYWORD; 
        } 
    }

    return token; 
}