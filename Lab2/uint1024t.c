#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#define BASE 1000000000 /* Система счисления, в которой хранятся разряды числа */
#define SIZE 35         /* Максимальное количество разрядов в числе в (10 ^ 9) с/c */
#define DIGIT_SIZE 9    /* Максимальное количество цифр в одном разряде числа в (10 ^ 9) с/c */
#define MAXLEN 310      /* Максимальное число разрядов в числе в 10 с/c + 1 */

/* Перечисление доступных операций */
enum Operations {
    ADDITION = 1,
    SUBTRACTION,
    MULTIPLICATION,
    COMPARISON,
    ALL
};

/* Перечисление результатов сравнения чисел */
enum Comparison_values {
    LESS = -1,
    EQUALS,
    GREATER
};

/* Структура, задающая тип данных uint1024_t*/
struct Big_integer {
    uint32_t digits[SIZE];
};
typedef struct Big_integer uint1024_t;

uint1024_t from_uint(unsigned int x);
uint1024_t add_op(uint1024_t x, uint1024_t y);
int comparison_op(uint1024_t x, uint1024_t y);
void printf_comparison_res(int comparison_res);
uint1024_t subtr_op(uint1024_t x, uint1024_t y);
uint1024_t mult_op(uint1024_t x, uint1024_t y);
void printf_value(uint1024_t x);
void scanf_value(uint1024_t *x);
uint1024_t get_first_number(void);
uint1024_t get_second_number(void);
int get_operator(void);
void allocation_check(char *allocated);


int main() {
    printf("\nThis program performs arithmetic operations with large unsigned numbers"
            "\nAvailable functions: addition (+), subtraction (-), multiplication (*), "
            "comparison (?)\nYou can also perform all of these by typing \'all\'"
            "\nNOTE: It is impossible to subtract from a smaller value\n");

    int starting_again = 1;  /* Необходимость запустить программу в очередной раз */
    while (starting_again) {
        uint1024_t number1 = get_first_number();
        uint1024_t number2 = get_second_number();
        int operator = get_operator();

        printf("\nThe first number  = ");
        printf_value(number1);
        printf("The second number = ");
        printf_value(number2);

        switch (operator) {
            /* Вывод суммы чисел */
            case ADDITION:
                printf("Sum = ");
                printf_value(add_op(number1, number2));
                break;

            /* Вывод разности чисел */
            case SUBTRACTION:
                printf("Difference = ");
                printf_value(subtr_op(number1, number2));
                break;

            /* Вывод произведения чисел */
            case MULTIPLICATION:
                printf("Product = ");
                printf_value(mult_op(number1, number2));
                break;
            
            /* Вывод результата сравнения чисел */
            case COMPARISON:
                printf_comparison_res(comparison_op(number1, number2));
                break;

            /* Вывод результатов всех операций */
            case ALL:
                printf_comparison_res(comparison_op(number1, number2));
                printf("Sum = ");
                printf_value(add_op(number1, number2));
                printf("Difference = ");
                printf_value(subtr_op(number1, number2));
                printf("Product = ");
                printf_value(mult_op(number1, number2));
                break;                
        }
        /* Приглашение пользователя к повторному выполнению операций */
        printf("\nContinue? Y/N\n");
        char answer[2];
        scanf("%s", answer);
        starting_again = (strcmp(answer, "Y") == 0) ? 1 : 0;
    }
    return 0;
}


/* Генерация из числа */
uint1024_t from_uint(unsigned int x) {
    uint1024_t number;
    int i = 0;
    /* Перевод числа в (10 ^ 9) с/c и запись в разряды */
    while (x > 0) {
        number.digits[i] = x % BASE;
        x /= BASE;
        ++i;
    }
    /* Обнуление оставшихся разрядов */
    while (i < SIZE) {
        number.digits[i] = 0;
        ++i;
    }
    return number;
}


/* Сложение */
uint1024_t add_op(uint1024_t x, uint1024_t y) {
    uint1024_t sum;
    int carry = 0;  /* Перенос */
    /* Реализация сложения с переносом "в столбик" */
    for (int i = 0; i < SIZE; ++i) {
        int full_sum = x.digits[i] + y.digits[i] + carry;
        sum.digits[i] = full_sum % BASE;
        carry = full_sum / BASE;
    }
    return sum;
}


/* Сравнение */
int comparison_op(uint1024_t x, uint1024_t y) {
    for (int i = SIZE - 1; i >= 0; --i) {
        if (x.digits[i] > y.digits[i]) {
            return GREATER;
        }
        if (x.digits[i] < y.digits[i]) {
            return LESS;
        }
    }
    return EQUALS;
}


/* Вычетание */
uint1024_t subtr_op(uint1024_t x, uint1024_t y) {
    uint1024_t difference;
    int comparison_res = comparison_op(x, y);
    int borrow = 0;  /* Занимаемое */

    switch (comparison_res) {
        /* Если уменьшаемое меньше вычетаемого */
        case LESS:
            printf("Unable to subtract from the smaller value! :-");
            difference = from_uint(0);
            break;

        /* Если уменшьшаемое равно вычетаемому */
        case EQUALS:
            difference = from_uint(0);
            break;
        
        /* Если уменьшаемое больше вычетаемого */
        case GREATER:
            /* Реализация вычетания с заниманием "в столбик" */
            for (int i = 0; i < SIZE; ++i) {
                int full_diff = x.digits[i] - y.digits[i] - borrow;
                if (full_diff >= 0) {
                    difference.digits[i] = full_diff;
                    borrow = 0;
                }
                else {
                    difference.digits[i] = BASE + full_diff;
                    borrow = 1;
                }
            }
            break;
    }
    return difference;
}


/* Умножение */
uint1024_t mult_op(uint1024_t x, uint1024_t y) {
    uint1024_t product;
    /* Обнуление разрядов */
    for (int i = 0; i < SIZE; ++i) {
        product.digits[i] = 0;
    }
    /* Нахождение длин чисел */ 
    int size_x, size_y;
    size_x = size_y = SIZE - 1;
    while (x.digits[size_x] == 0) {
        --size_x;
    }
    while (y.digits[size_y] == 0) {
        --size_y;
    }
    /* Если сумма длин чисел превосходит индекс максимального разряда результата */
    if (size_x + size_y >= SIZE) {
        printf("Unable to multiply these numbers due to overflow! :-");
        return product;
    } 
    /* Умножение с переносом "в столбик" */
    uint64_t current;
    int carry = 0;  /* Перенос */
    int i = 0;
    while (i <= size_x) {
        int j = 0;
        while (j <= size_y || carry != 0) {
            if (j <= size_y) {
                current = (uint64_t)x.digits[i] * (uint64_t)y.digits[j];
                current += product.digits[i + j] + carry;
            }
            else if (i + j < SIZE) {
                current = product.digits[i + j] + carry;
            }
            /* Если возник перенос в несуществующий индекс разряда результата */
            else {
                printf("Unable to multiply these numbers due to overflow! :-");
                return from_uint(0);
            }
            product.digits[i + j] = current % BASE;
            carry = current / BASE;
            ++j;
        }
        ++i;
    }
    return product;
}


/* Вывод в стандартный поток вывода */
void printf_value(uint1024_t x) {
    /* Нахождение длины числа */
    int index = SIZE - 1;
    while (index >= 0) {
        if (x.digits[index] == 0) {
            --index;
        }
        else {
            break;
        }
    }
    /* Если число - ноль */
    if (index < 0) {
        printf("%d\n", 0);
        return;
    }

    char *output_string = (char *)calloc(MAXLEN, sizeof(char));
    allocation_check(output_string);
    int is_first_figit = 1; /* Индикатор того, что текущий разряд - старший */
    while (index >= 0) {
        char *buffer = (char *)calloc(DIGIT_SIZE + 1, sizeof(char));
        allocation_check(buffer);
        /* Интерпретируем 0 в значащем разряде как 9 нулей */
        if (x.digits[index] == 0) {
            buffer = "000000000";
        }
        else {
            sprintf(buffer, "%d", x.digits[index]);
            int length = strlen(buffer);
            /* Дополняем разряд ведущими нулями, если они отсутствуют */
            if (length < DIGIT_SIZE && !is_first_figit) {
                char *new_buffer = (char *)calloc(DIGIT_SIZE, sizeof(char));
                allocation_check(new_buffer);
                int j = length;
                while (j < DIGIT_SIZE) {
                    new_buffer[j - length] = '0';
                    ++j;
                }
                new_buffer[j] = '\0';
                strcat(new_buffer, buffer);
                strcpy(buffer, new_buffer);
                free(new_buffer);
            }
        }
        strcat(output_string, buffer);
        free(buffer);
        --index;
        is_first_figit = 0;
    }
    printf("%s\n", output_string);
    free(output_string);
}


/* Чтение из стандартного потока ввода */
void scanf_value(uint1024_t *x) {
    char *input_string = (char *)calloc(MAXLEN, sizeof(char));
    allocation_check(input_string);
    scanf("%s", input_string);
    int i, j = strlen(input_string);
    /* Если число целиком помещается в один разряд в (10 ^ 9) c/c */
    if (j <= DIGIT_SIZE) {
        x->digits[0] = atoi(input_string);
        i = 1;
    }
    else {
        /* Запись в текущий разряд по 9 цифр */
        j -= DIGIT_SIZE;
        for (i = 0; i < SIZE, j >= 0; ++i, j -= DIGIT_SIZE) {
            char *digit = (char *)calloc(DIGIT_SIZE, sizeof(char));
            allocation_check(digit);
            strncpy(digit, &input_string[j], DIGIT_SIZE);
            x->digits[i] = atoi(digit);
            free(digit);
        }
        /* Если остались цифры, которые не были записаны */
        if (j > -DIGIT_SIZE && j < 0) {
            char *digit = (char *)calloc(DIGIT_SIZE + j, sizeof(char));
            allocation_check(digit);
            strncpy(digit, &input_string[0], DIGIT_SIZE + j);
            x->digits[i] = atoi(digit);
            free(digit);
            ++i;
        }
    }
    free(input_string);
    /* Заполнение оставшихся разрядов нулями */
    while (i < SIZE) {
        x->digits[i] = 0;
        ++i;
    }
}


/* Получение первого числа */
uint1024_t get_first_number(void) {
    printf("\nIs your first number greater than 4,294,967,295? Y/N\n");
    char answer[2];
    scanf("%s", answer);
    uint1024_t big_integer_1;
    if (strcmp(answer, "Y") == 0) {
        printf("Input your first number: ");
        scanf_value(&big_integer_1);
    }
    else if (strcmp(answer, "N") == 0) {
        printf("Input your first number: ");
        unsigned int number;
        scanf("%d", &number);
        big_integer_1 = from_uint(number);
    }
    else {
        printf("Try once again!\n");
        return get_first_number();
    }
    return big_integer_1;
}


/* Получение второго числа */
uint1024_t get_second_number(void) {
    printf("\nIs your second number greater than 4,294,967,295? Y/N\n");
    char answer[2];
    scanf("%s", answer);
    uint1024_t big_integer_2;
    if (strcmp(answer, "Y") == 0) {
        printf("Input your second number: ");
        scanf_value(&big_integer_2);
    }
    else if (strcmp(answer, "N") == 0) {
        printf("Input your second number: ");
        unsigned int number;
        scanf("%d", &number);
        big_integer_2 = from_uint(number);
    }
    else {
        printf("Try once again!\n");
        return get_second_number();
    }
    return big_integer_2;
}


/* Получение оператора */
int get_operator(void) {
    printf("\nChoose one of available operators: + || - || * || ? || all\n");
    char operator[4];
    scanf("%s", operator);
    if (strcmp(operator, "+") == 0) {
        return ADDITION;
    }
    if (strcmp(operator, "-") == 0) {
        return SUBTRACTION;
    }
    if (strcmp(operator, "*") == 0) {
        return MULTIPLICATION;
    }
    if (strcmp(operator, "?") == 0) {
        return COMPARISON;
    }
    if (strcmp(operator, "all") == 0) {
        return ALL;
    }
    printf("Try once again!\n");
    return get_operator();
}


/* Представление результатов сравнения */
void printf_comparison_res(int comparison_res) {
    switch(comparison_res) {
        /* Если первое число больше */
        case GREATER:
            printf("\nThe first number is greater than the second one\n");
            break;

        /* Если первое число меньше */
        case LESS:
            printf("\nThe first number is less than the second one\n");
            break;

        /* Если числа равны */
        case EQUALS:
            printf("\nThe numbers are equal\n");
            break;
    }
}


/* Проверка на выделение памяти для массива char */
void allocation_check(char *allocated) {
    if (allocated == NULL) {
        printf("\nERROR: Cannot allocate memory\nTry launching the program again");
        exit(EXIT_FAILURE);
    }
}
