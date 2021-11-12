#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#define MAXLEN 400             /* Максимальное количество символов в строке лога (с запасом) */
#define NO_TIME_WINDOW_MODE -1 /* Режим работы без определения временного окна */

long maxnum = 0;                                /* Переменная, хранящая наибольшее число запросов */
char time_start[] = "xxx xxx 00 00:00:00 0000"; /* Строка, содержащая время начала временного окна */
char time_end[] = "xxx xxx 00 00:00:00 0000";   /* Строка, содержащая время конца временного окна */

/* Перечисление месяцев */
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

/* Структура узла списка */
struct Node {
    struct Node *prev, *next;
    time_t request_time;
};

/* Структура списка-очереди */
struct List {
    struct Node *head, *tail;
    size_t size;
};

time_t get_argument(int argc, char **argv);
int get_request_and_window(char line[], FILE *requests_list, struct List *queue, time_t time_window);
int get_month(char month_name[]);
struct tm get_time(char time[]);
void free_list(struct List **list);
void push_to_list(struct List *list, time_t value);
void extract(struct List *list);
void allocation_check(void *allocated);


int main(int argc, char *argv[]) {
    FILE *input_log = fopen("access_log_Jul95", "r");
    FILE *requests_list = fopen("list_of_requests", "w");
    /* Если возникли проблемы с файлами */
    if (input_log == NULL) {
        printf("Some problems while reading the file! Try again!\n");
        exit(EXIT_FAILURE);
    }
    if (requests_list == NULL) {
        printf("Some problems while creating the file!\n");
        exit(EXIT_FAILURE);
    }
    /* Переменная, хранящая значение временного окна в секундах */
    time_t time_window = get_argument(argc, argv);

    char line[MAXLEN];
    int counter = 0;
    /* Инициализация очереди */
    struct List *queue = (struct List *)malloc(sizeof(struct List));
    allocation_check(queue);
    queue->size = 0;
    queue->head = queue->tail = NULL;
    /* Работа с файлом по считыванию строк и подсчёту нужных данных */
    while (fgets(line, MAXLEN, input_log)) {
        counter += get_request_and_window(line, requests_list, queue, time_window);
    }
    /* Вывод ответа на поставленные вопросы */
    fprintf(requests_list, "\nThe number of 5xx errors: %d", counter);
    printf("\nThe number of 5xx errors: %d\nThe list is in \"list_of_requests\" file!\n\n", counter);
    if (time_window != NO_TIME_WINDOW_MODE) {
        printf("The biggest number of requests within %ld seconds is %d\n", time_window, maxnum);
        printf("These requests were made from [%s] to [%s]\n\n", time_start, time_end);
    }

    free_list(&queue);
    fclose(input_log);
    fclose(requests_list);
    return 0;
}


/* Получение значения аргументов командной строки */
time_t get_argument(int argc, char **argv) {
    /* Если аргументов нет, программа не ищет временное окно */
    if (argc == 1) {
        return NO_TIME_WINDOW_MODE;
    }
    /* Если дан 1 аргумент, программа рассматривает его как значение временного окна в секундах */
    if (argc == 2) {
        return atol(argv[1]);
    }
    /* Если дано 2 аргумента, перевод зависит от ключа, поданного программе */
    if (argc == 3) {
        time_t time_window = 0;
        /* Ключ "-a" - перевод из вида Y:M:d:h:m:s */
        if (strcmp(argv[1], "-a") == 0) {
            char *argument = argv[2];
            char *time_pos;
            long multiplier = 12 * 30 * 24 * 3600;
            char *year = strtok_r(argument, ":", &time_pos);
            time_window += atol(year) * multiplier;
            char *month = strtok_r(NULL, ":", &time_pos);
            multiplier /= 12;
            time_window += atol(month) * multiplier;
            char *day = strtok_r(NULL, ":", &time_pos);
            multiplier /= 30;
            time_window += atol(day) * multiplier;
            char *hour = strtok_r(NULL, ":", &time_pos);
            multiplier /= 24;
            time_window += atol(hour) * multiplier;
            char *minutes = strtok_r(NULL, ":", &time_pos);
            multiplier /= 60;
            time_window += atol(minutes) * multiplier;
            char *seconds = time_pos;
            time_window += atol(seconds);
            return time_window;
        }
        time_window = atol(argv[2]);
        /* Ключ "-s" - для ввода значения в секундах */
        if (strcmp(argv[1], "-s") == 0) {
            return time_window;
        }
        time_window *= 60;
        /* Ключ "-m" для ввода значения в минутах */
        if (strcmp(argv[1], "-m") == 0) {
            return time_window;
        }
        time_window *= 60;
        /* Ключ "-h" для ввода значения в часах */
        if (strcmp(argv[1], "-h") == 0) {
            return time_window;
        }
        time_window *= 24;
        /* Ключ "-d" для ввода значения в днях */
        if (strcmp(argv[1], "-d") == 0) {
            return time_window;
        }
        time_window *= 30;
        /* Ключ "-M" для ввода значения в месяцах */
        if (strcmp(argv[1], "-M") == 0) {
            return time_window;
        }
        time_window *= 12;
        /* Ключ "-Y" для ввода значения в годах */
        if (strcmp(argv[1], "-Y") == 0) {
            return time_window;
        }
        /* Если ключ не распознан */
        printf("\nThe argument is of incompatible type. Try again!\n"
               "Available flags: -s, -m, -h, -d, -M, -Y and -a\n"
               "Correct input format for -a is Y:M:d:h:m:s\n\n");
        exit(EXIT_FAILURE);
    }
}


/* Ведение подсчёта 5xx ошибок, учёта запросов и работы с временным окном */
int get_request_and_window(char line[], FILE *requests_list, struct List *queue, time_t time_window) {
    int is_5xx_err = 0;
    char *line_pos = strchr(line, '[');
    /* Проверка на наличие времени запроса в строке лога */
    if (line_pos == NULL) {
        return is_5xx_err;
    }
    char *time = strtok_r(line_pos, "]", &line_pos);
    /* Работа с временным окном с помощью очереди на двусвязном списке */
    if (time_window != NO_TIME_WINDOW_MODE) {
        struct tm time_of_request = get_time(time + 1);
        time_t time_value = mktime(&time_of_request);
        /* Если очередной запрос укладывается во временное окно, записываем его в конец очереди */
        if (queue->size == 0 || time_value - queue->head->request_time < time_window) {
            push_to_list(queue, time_value);
            /* Если размер количества запросов во временном окне больше текущего максимума */
            if (queue->size > maxnum) {
                /* Обновление значений начала и конца временного окна, максимума запросов */
                char *buffer = ctime(&(queue->head->request_time));
                strncpy(time_start, buffer, strlen(time_start));
                buffer = ctime(&(queue->tail->request_time));
                strncpy(time_end, buffer, strlen(time_end));
                maxnum = queue->size;
            }
        }
        /* Если запрос уже не влезает в рамки временного окна - сдвигаем очередь */
        else {
            while (queue->size > 0 && time_value - queue->head->request_time >= time_window) {
                extract(queue);
            }
            push_to_list(queue, time_value);
        }
    }
    line_pos = strchr(line_pos, '"');
    char *request = strtok_r(line_pos, "\"", &line_pos);
    int code_index = line_pos + 1 - line;
    if (line[code_index] == '5') {
        fprintf(requests_list, "%s\n", request);
        is_5xx_err = 1;
    }
    return is_5xx_err;
}


/* Перевод времени из заданного формата в структуру tm заголовочного файла time.h */
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


/* Перевод буквенного обозначения месяца в численный формат */
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


/* Добавление нового узла в конец списка */
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


/* Извлечение первого элемента списка */
void extract(struct List *list) {
    struct Node *prev = list->head;
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


/* Освобождение памяти, выделенной для элементов, оставшихся в списке */
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


/* Проверка на выделение памяти */
void allocation_check(void *allocated) {
    if (allocated == NULL) {
        printf("\nERROR: Cannot allocate memory\nTry launching the program again\n");
        exit(EXIT_FAILURE);
    }
}
