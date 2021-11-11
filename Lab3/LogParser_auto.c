#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#define MAXLEN 400

long maxnum = 0;
char time_start[] = "xxx xxx 00 00:00:00 0000";
char time_end[] = "xxx xxx 00 00:00:00 0000";

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

struct Node {
    struct Node *prev, *next;
    time_t request_time;
};

struct List {
    struct Node *head, *tail;
    size_t size;
};

int get_request(char line[], FILE *requests_list, struct List *queue, time_t window_time);
int get_month(char month_name[]);
struct tm get_time(char time[]);
void free_list(struct List **list);
void push_to_list(struct List *list, time_t value);
void extract(struct List *list);
void allocation_check(void *allocated);


int main(int argc, char *argv[]) {
    clock_t tic = clock();
    FILE *input_log = fopen("access_log_Jul95", "r");
    FILE *requests_list = fopen("list_of_requests", "w");
    if (input_log == NULL) {
        printf("Some problems while reading the file! Try again!\n");
        exit(EXIT_FAILURE);
    }
    if (requests_list == NULL) {
        printf("Some problems while creating the file!\n");
        exit(EXIT_FAILURE);
    }
    time_t window_time = atol(argv[1]);

    char line[MAXLEN];
    int counter = 0;
    struct List *queue = (struct List *)malloc(sizeof(struct List));
    allocation_check(queue);
    queue->size = 0;
    queue->head = queue->tail = NULL;
    while (fgets(line, MAXLEN, input_log)) {

        counter += get_request(line, requests_list, queue, window_time);

    }
    fprintf(requests_list, "\nThe number of 5xx errors: %d", counter);
    printf("\nThe number of 5xx errors: %d\nThe list is in \"list_of_requests\" file!\n", counter);
    printf("The biggest number of requests within %s is %d\n", argv[1], maxnum);
    printf("These requests were made since %s up to %s\n\n", time_start, time_end);
    free_list(&queue);
    fclose(input_log);
    fclose(requests_list);
    clock_t toc = clock();
    printf("Elapsed: %f seconds\n", (double)(toc - tic) / CLOCKS_PER_SEC);
    return 0;
}


int get_request(char line[], FILE *requests_list, struct List *queue, time_t window_time) {
    int is_5xx_err = 0;
    char *line_pos = strchr(line, '[');
    if (line_pos == NULL) {
        return is_5xx_err;
    }
    char *time = strtok_r(line_pos, "]", &line_pos);
    struct tm time_of_request = get_time(time + 1);
    time_t time_value = mktime(&time_of_request);

    if (queue->size == 0 || time_value - queue->head->request_time < window_time) {
        push_to_list(queue, time_value);
        if (queue->size > maxnum) {
            char *buffer = ctime(&(queue->head->request_time));
            strncpy(time_start, buffer, strlen(time_start));
            buffer = ctime(&(queue->tail->request_time));
            strncpy(time_end, buffer, strlen(time_end));
            maxnum = queue->size;
        }
    }
    else {
        while (queue->size > 0 && time_value - queue->head->request_time >= window_time) {
            extract(queue);
        }
        push_to_list(queue, time_value);
    }
    
    line_pos = strchr(line_pos, '"');
    char *request = strtok_r(line_pos, "\"", &line_pos);
    int i = line_pos + 1 - line;
    if (line[i] == '5') {
        fprintf(requests_list, "%s\n", request);
        is_5xx_err = 1;
    }
    return is_5xx_err;
}


struct tm get_time(char time[]) {
    struct tm time_of_request = {0};
    char *time_pos;
    char *day = strtok_r(time, "/", &time_pos);
    time_of_request.tm_mday = atoi(day);
    char *month = strtok_r(NULL, "/", &time_pos);
    time_of_request.tm_mon = get_month(month);
    char *year = strtok_r(NULL, ":", &time_pos);
    time_of_request.tm_year = atoi(year) - 1900;
    char *hour = strtok_r(NULL, ":", &time_pos);
    time_of_request.tm_hour = atoi(hour);
    char *minutes = strtok_r(NULL, ":", &time_pos);
    time_of_request.tm_min = atoi(minutes);
    char *seconds = strtok_r(NULL, " ", &time_pos);
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


void push_to_list(struct List *list, time_t value) {
    struct Node *new_node = (struct Node *)malloc(sizeof(struct Node));
    allocation_check(new_node);
    new_node->request_time = value;
    new_node->next = NULL;
    new_node->prev = list->tail;
    if (list->tail != NULL) {
        list->tail->next = new_node;
    }
    list->tail = new_node;
    if (list->head == NULL) {
        list->head = new_node;
    }
    list->size = list->size + 1;
}


void extract(struct List *list) {
    struct Node *prev;

    prev = list->head;
    list->head = list->head->next;
    if (list->head != NULL) {
        list->head->prev = NULL;
    }
    if (prev == list->tail) {
        list->tail = NULL;
    }
    free(prev);
    list->size = list->size - 1;


}


void free_list(struct List **list) {
    struct Node *current = (*list)->head;
    struct Node *next = NULL;
    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    free(*list);
}


void allocation_check(void *allocated) {
    if (allocated == NULL) {
        printf("\nERROR: Cannot allocate memory\nTry launching the program again\n");
        exit(EXIT_FAILURE);
    }
}