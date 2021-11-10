#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#define MAXLEN 400

int counter = 0;

enum Months {
    JANUARY,
    FEBRUARY,
    MARCH,
    APRIL,
    MAY,
    JUNE,
    JULY,
    AUGUST,
    SEPTEMBER,
    OCTOBER,
    NOVEMBER,
    DECEMBER
};

/*
struct Node {
    time_t time;
    struct Node *next;
};

struct Node *list_head, *link_to_prev;
*/
long long index = 0;
int get_request(char line[], FILE *requests_list, struct tm **time_window);
int get_month(char month_name[]);
struct tm get_time(char time[]);


int main(int argc, char argv[]) {
    clock_t tic = clock();
    FILE *input_log = fopen("access_log_Jul95", "r");
    FILE *requests_list = fopen("list_of_requests", "w");
    if (!input_log) {
        printf("Some problems while reading the file! Try again!\n");
    }
    if (!requests_list) {
        printf("Some problems while creating the file!\n");
    }

    char line[MAXLEN];
    int counter = 0;
    struct tm *time_window = (struct tm *)malloc(100000000 * sizeof(struct tm));
    while (fgets(line, MAXLEN, input_log)) {

        counter += get_request(line, requests_list, &time_window);

    }
    fprintf(requests_list, "\nThe number of 5xx errors: %d", counter);
    fclose(input_log);
    fclose(requests_list);  
    
    clock_t toc = clock();
    printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);
    return 0;
}


int get_request(char line[], FILE *requests_list, struct tm **time_window) {
    int is_5xx_err = 0;
    char *save_line_pos;
    strtok_r(line, "[", &save_line_pos);
    char *time = strtok_r(NULL, "]", &save_line_pos);
    struct tm time_of_request = get_time(time);
    time_t required = mktime(&time_of_request);
    time_window[index] = required;
    //stop
    strtok_r(NULL, "\"", &save_line_pos);
    char *request = strtok_r(NULL, "\"", &save_line_pos);
    int i = save_line_pos + 1 - line;
    if (line[i] == '5') {
        fprintf(requests_list, "%s\n", request);
        is_5xx_err = 1;
    }
    return is_5xx_err;
}


struct tm get_time(char time[]) {
    struct tm time_of_request;
    char *save_time_pos;
    char *day = strtok_r(time, "/", &save_time_pos);
    time_of_request.tm_mday = atoi(day);
    char *month = strtok_r(NULL, "/", &save_time_pos);
    time_of_request.tm_mon = get_month(month);
    char *year = strtok_r(NULL, ":", &save_time_pos);
    time_of_request.tm_year = atoi(year) - 1900;
    char *hour = strtok_r(NULL, ":", &save_time_pos);
    time_of_request.tm_hour = atoi(hour);
    char *minutes = strtok_r(NULL, ":", &save_time_pos);
    time_of_request.tm_min = atoi(minutes);
    char *seconds = strtok_r(NULL, " ", &save_time_pos);
    time_of_request.tm_sec = atoi(seconds);
    return time_of_request;
}

int get_month(char month_name[]) {
    if (strcmp(month_name, "Jan") == 0) {
        return JANUARY;
    }
    if (strcmp(month_name, "Feb") == 0) {
        return FEBRUARY;
    }
    if (strcmp(month_name, "Mar") == 0) {
        return MARCH;
    }
    if (strcmp(month_name, "Apr") == 0) {
        return APRIL;
    }
    if (strcmp(month_name, "May") == 0) {
        return MAY;
    }
    if (strcmp(month_name, "Jun") == 0) {
        return JUNE;
    }
    if (strcmp(month_name, "Jul") == 0) {
        return JULY;
    }
    if (strcmp(month_name, "Aug") == 0) {
        return AUGUST;
    }
    if (strcmp(month_name, "Sep") == 0) {
        return SEPTEMBER;
    }
    if (strcmp(month_name, "Oct") == 0) {
        return OCTOBER;
    }
    if (strcmp(month_name, "Nov") == 0) {
        return NOVEMBER;
    }
    if (strcmp(month_name, "Dec") == 0) {
        return DECEMBER;
    }   
}