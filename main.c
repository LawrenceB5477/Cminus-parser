#include "lexer.h"
#include "parser.h"

//Main function 
int main(int argc, char **argv) {

    if (argc < 2) {
        fprintf(stderr, "Please specify an input file name.\n");
        exit(1); 
    }

    parse(argv[1]); 
    printf("SUCCESS\n"); 
    return 0; 
}


