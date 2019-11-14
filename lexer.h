#ifndef LEXER_HEADER 
#define LEXER_HEADER 

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h> 
#define TRUE 1
#define FALSE 0

//States of the DFA 
typedef enum {
    START, INNUM, INID, LT, GT, EQ, NOT, FWDSLASH, LINECMMNT, BLCKCMMNT, 
    ASTER, ERRORSTATE, FINISH
} STATE; 

//Token types 
typedef enum {
    ID, NUM, KEYWORD, SYMBOL, ERROR, ENDFILE
} TOKEN; 

//Token structure 
typedef struct {
    TOKEN token; 
    char *lexeme; 
} TOKEN_STRUCT; 


//Functions 
//Frees a token struct 
void freeTokenStruct(TOKEN_STRUCT *token);

//Prints a token Struct 
void printToken(TOKEN_STRUCT *token);

//Utility function to fill out the lexeme of a token 
void fillLexeme(TOKEN_STRUCT *token, int start, int stop);

//Returns TRUE if c is a valid character in the language 
int isValidCharacter(int c);

//Reads the next line of input from the file. Returns EOF if EOF is reached
int readNextLine(void);

//Skips space until the first non-whitespace character is found. Returns EOF if EOF reached 
int skipSpace(void);

//Returns the next token. Token has token field set to ENDFILE if EOF reached 
TOKEN_STRUCT* nextToken(void);

#endif
