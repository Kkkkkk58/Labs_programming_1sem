#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[]) {
    char *filename = NULL;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--file") == 0) {
            filename = argv[i + 1];
            if (filename == NULL) {
                printf("Unsupported input format! Try again and specify the name of the file!\n");
                exit(EXIT_FAILURE);
            }
            ++i;
        }
        else if (strcmp(argv[i], "--extract") == 0) {
            ++i;
        }
        else if (strcmp(argv[i], "--list") == 0) {
            ++i;
        }
        else {
            ;
        }
    }
    
    return 0;
}