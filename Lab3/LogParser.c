#include <stdio.h>
#include <string.h>
#include <time.h>
#define MAXLEN 400
#define NOT_5XX_ERR "0"

int get_request(char line[], FILE *requests_list);


int main(int argc, char argv[]) {
    clock_t tic = clock();

    FILE *input_log = fopen("access_log_Jul95", "r");
    FILE *requests_list = fopen("list_of_requests", "w");
    if (!input_log) {
        printf("Some problems while reading the file! Try again!\n");
    }
    char line[MAXLEN];
    int counter = 0;

    while (fgets(line, MAXLEN, input_log)) {

        counter += get_request(line, requests_list);

    }
    fprintf(requests_list, "\nThe number of 5xx errors: %d", counter);
    fclose(input_log);
    fclose(requests_list);

    clock_t toc = clock();
    printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);
    return 0;
}


int get_request(char line[], FILE *requests_list) {
    int is_5xx_err = 0;
    int first_quot = -1;
    int second_quot = -1;
    int i = 0;
    while (second_quot == -1) {
        if (line[i] == '"')  {
            if (first_quot == -1) {
                first_quot = i;
            }
            else {
                second_quot = i;
            }
        }
        ++i;
    }

    if (line[i + 1] == '5') {
        is_5xx_err = 1;
        char request[MAXLEN] = {0};
        int request_len = second_quot - first_quot - 1;
        strncpy(request, &line[first_quot + 1], request_len);
        request[request_len + 1] = '\0';
        fprintf(requests_list, "%s\n", request);
    }
    return is_5xx_err;
}