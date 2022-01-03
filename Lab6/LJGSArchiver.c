/**
 * @file LJGSArchiver.c
 * @author Karim Khasan (karimhasan3879@gmail.com)
 * 
 * @brief This program is the solution for Lab6 of the 1st semester of ITMO's C/C++ course
 * 
 * @details The program presents an archiver of a custom extension.
 *          The archiver supports adding files into it, listing all the files the archive consists of 
 *          and extracting files into given folder.
 *          LJGSArchiver supports Huffman lossless data compression in order to store files efficiently.
 *          
 * @version 6.5
 * @date 2022-01-03
 * 
 * @copyright LJGS Software Copyright (c) 2018
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#define SIZE_MULTIPLIER 256
#define MAX_DIR_NAME 255
#define MAX_CODE_LEN 255
#define SYMB_TABLE_COUNT 256
#define SWAP(x, y) \
    tmp = x;       \
    x = y;         \
    y = tmp;

#pragma pack(push, 2)
/* Структура, использумеая для хранения элементов кодового дерева */
typedef struct Huffman_tree_storage {
    long long number_of_entries;
    int symbol;
    struct Node *root;
} Huffman_tree_storage;
#pragma pack(pop)

/* Структура, используемая для хранения представления кода */
typedef struct Code {
    unsigned char code_bits[MAX_CODE_LEN];
    char length;
} Code;

#pragma pack(push, 2)
/* Структура, используемая для хранения узлов кодового дерева */
typedef struct Node {
    int symbol;
    struct Node *left, *right;
    long long frequency;
    Code code;
} Node;
#pragma pack(pop)

/* Структура, хранящая приоритетную очередь для создания кодового дерева */
typedef struct Priority_queue {
    Node *array[SYMB_TABLE_COUNT];
    int head, size;
} Priority_queue;

/* Структура, представляющая элемент кодовой таблицы */
typedef struct Huffman_table {
    unsigned char symbol;
    Code code;
} Huffman_table;

/* Структура для составления таблицы для раскодирования информации */
typedef struct Decode_table {
    int size;
    Huffman_table *codes;
} Decode_table;


static Huffman_tree_storage hash_table[SYMB_TABLE_COUNT] = {0};
void extract(char *archive_name, char *dirname);
void list(char *archive_name);
void create(char* archive_name, int files_number, char *files_names[]);
unsigned char *count_size(unsigned int size);
unsigned int get_size(unsigned char *size_arr);
void count_entries(int files_number, char *files_names[]);
Node *build_code_tree(int files_number, char *files_names[]);
void quick_sort_freq(Huffman_tree_storage *array, int start, int end);
void tree_walk(Node *tree, Huffman_table *table);
void assign_codes(Node *tree);
void quick_sort_tables(Huffman_table *table, int start, int end);
Huffman_table *binsearch(Huffman_table *array, int start, int end, unsigned char *to_seek);
void free_tree(Node **tree);


int main(int argc, char *argv[]) {
    char *archive_name = NULL;
    char dirname[MAX_DIR_NAME] = "";
    /* Парсинг аргументов командной строки */
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--file") == 0) {
            archive_name = argv[i + 1];
            /* Если имя выходного архива не было предоставлено */
            if (archive_name == NULL) {
                printf("Unsupported input format! Try again and specify the name of the file!\n");
                exit(EXIT_FAILURE);
            }
            ++i;
        }
        else if (strcmp(argv[i], "--extract") == 0) {
            if (argc > i + 1) {
                strcpy(dirname, argv[i + 1]);
            }
            extract(archive_name, dirname);
            ++i;
        }
        else if (strcmp(argv[i], "--list") == 0) {
            list(archive_name);
        }
        else if (strcmp(argv[i], "--create") == 0) {
            create(archive_name, argc - 4, &argv[i + 1]);
            break;
        }
        else {
            printf("Unknown argument! Try again!\n");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}


/* Функция, создающая архив из предоставленных файлов */
void create(char *archive_name, int files_number, char *files_names[]) {

    FILE *archive = fopen(archive_name, "wb");
    /* Если не удалось создать файл архива */
    if (archive == NULL) {
        printf("Unable to create the archive file! Try again!\n");
        exit(EXIT_FAILURE);
    }

    Node *tree = build_code_tree(files_number, files_names); /* Кодовое дерево с новыми кодами символов */
    Huffman_table code_table[SYMB_TABLE_COUNT];              /* Новая кодовая таблица на основе полученного дерева */
    for (int i = 0; i < SYMB_TABLE_COUNT; ++i) {
        code_table[i].code.length = -1;
    }
    tree_walk(tree, code_table);
    free_tree(&tree);

    int symbols_count = 0, position = 0;
    char *header_buf = (char *)calloc(SYMB_TABLE_COUNT * MAX_CODE_LEN, sizeof(char));
    /* Заполнение информации о кодовой таблице */
    for (int i = 0; i < SYMB_TABLE_COUNT; ++i) {
        if (code_table[i].code.length != -1) {
            memmove(&header_buf[position], &(code_table[i].symbol), 1);
            memmove(&header_buf[position + 1], &(code_table[i].code.length), 1);
            memmove(&header_buf[position + 2], code_table[i].code.code_bits, code_table[i].code.length);
            position += 2 + code_table[i].code.length;
            ++symbols_count;
        }
    }

    /* Заполнение информации о количестве закодированных символов */
    char symbols_number_arr[2] = {0};
    if (symbols_count == SYMB_TABLE_COUNT) {
        symbols_number_arr[1] = 1;
    }
    else {
        symbols_number_arr[0] = symbols_count;
    }
    fwrite(symbols_number_arr, sizeof(char), 2, archive);

    /* Запись кодовой таблицы */
    unsigned char *header_size = count_size(position);
    fwrite(header_size, sizeof(unsigned char), 4, archive);
    free(header_size);
    fwrite(header_buf, sizeof(char), position, archive);
    free(header_buf);

    /* Последовательное открытие данных файлов с заменой кодов символов на новые */
    for (int i = 0; i < files_number; ++i) {
        FILE *archive_content = fopen(files_names[i], "rb");
        /* Если предоставленный файл не открылся - переходим к следующему */
        if (archive_content == NULL) {
            printf("Unable to open the file named \"%s\". Skipping...\n", files_names[i]);
            continue;
        }
        printf("Adding \"%s\" to the archive...", files_names[i]);

        /* Заносим имя файла в архив */
        fwrite(files_names[i], strlen(files_names[i]) + 1, 1, archive);
        /* Временный файл с доступным именем для записи кодированного файла */
        char tmp_name[11] = {0};
        tmpnam(tmp_name);
        FILE *tmp = fopen(&tmp_name[1], "wb+");
        if (tmp == NULL) {
            printf("Some errors occured when compressing the file! Skipping it...\n");
            continue;
        }
        int symbol;
        char curr_byte = 0, byte_length = 0, tail = 0;

        /* Считывание и обработка символов предоставленного файла */
        while ((symbol = fgetc(archive_content)) != EOF) {
            /* Кодирование символа согласно кодовой таблице */
            Code encoded = code_table[symbol].code;
            for (int i = 0; i < encoded.length; ++i) {
                if ((encoded.code_bits[i] & 0x1) != 0) {
                    curr_byte |= (1 << (7 - byte_length));
                }
                else {
                    curr_byte &= ~(1 << (7 - byte_length));
                }
                ++byte_length;
                if (byte_length == 8) {
                    fputc(curr_byte, tmp);
                    curr_byte = byte_length = 0;
                }
            }
        }

        /* Дополнение текущего байта */
        if (byte_length != 8) {
            tail = 8 - byte_length;
            fputc(curr_byte, tmp);
        }
        fclose(archive_content);

        /* Подсчёт размера сжатого файла и запись размера, приведённого к представлению в виде массива */
        unsigned int compressed_size = ftell(tmp);
        unsigned char *compressed_size_array = count_size(compressed_size);
        fwrite(compressed_size_array, 4, 1, archive);
        free(compressed_size_array);
        fputc(tail, archive);

        /* Запись кодированного файла в архив */
        fseek(tmp, 0, SEEK_SET);
        char *file_buf = (char *)malloc(compressed_size * sizeof(char));
        fread(file_buf, sizeof(char), compressed_size, tmp);
        fwrite(file_buf, compressed_size, 1, archive);
        free(file_buf);
        fclose(tmp);
        remove(&tmp_name[1]);
        printf(" Done!\n");
    }

    printf("All valid given files were added to the archive named \"%s\"\n", archive_name);
    fclose(archive);
}


/* Функция для извлечения файлов из архива */
void extract(char *archive_name, char *dirname) {
    FILE *archive = fopen(archive_name, "rb");
    /* Если архив не открылся */
    if (archive == NULL) {
        printf("Unable to open provided archive! Try again!\n");
        exit(EXIT_FAILURE);
    }

    /* Считывание информации о количестве символов в кодовой таблице */
    unsigned char header_size[2];
    fread(header_size, sizeof(unsigned char), 2, archive);
    int offset = header_size[0];
    if (header_size[1] == 1) {
        offset = SYMB_TABLE_COUNT;
    }
    fseek(archive, 4, SEEK_CUR);

    /* Создание multimap по длине кода с цепочками кодов и соответствующих символов */
    Decode_table code_multimap[MAX_CODE_LEN];
    for (int i = 0; i < MAX_CODE_LEN; ++i) {
        code_multimap[i].size = 0;
        code_multimap[i].codes = NULL;
    }

    /* Заполнение multimap для последующего декодирования файлов */
    for (int i = 0; i < offset; ++i) {
        Huffman_table table;
        memset(table.code.code_bits, 0, MAX_CODE_LEN);
        fread(&(table.symbol), sizeof(unsigned char), 1, archive);
        fread(&(table.code.length), sizeof(unsigned char), 1, archive);
        fread((table.code.code_bits), sizeof(unsigned char), table.code.length, archive);
        int index = table.code.length - 1;
        code_multimap[index].size += 1;
        code_multimap[index].codes = (Huffman_table *)realloc(code_multimap[index].codes, code_multimap[index].size * sizeof(Huffman_table));
        code_multimap[index].codes[code_multimap[index].size - 1] = table;
    }

    /* Сортировка внутренних цепочек multimap для последующего использования двоичного поиска */
    for (int i = 0; i < MAX_CODE_LEN; ++i) {
        if (code_multimap[i].size > 1) {
            quick_sort_tables(code_multimap[i].codes, 0, code_multimap[i].size - 1);
        }
    }

    /* Создание указанной папки */
    char path[MAX_DIR_NAME] = {0};
    if (strcmp(dirname, "") != 0) {
        mkdir(dirname);
        sprintf(path, "%s\\", dirname);
    }

    int symbol = 0;
    int prev_position = ftell(archive);
    /* Считывание информации из архива */
    while (symbol != EOF) {
        while ((symbol = fgetc(archive)) != '\0' && symbol != EOF) {
            ;
        }
        if (symbol == EOF) {
            break;
        }

        /* Считывание имени файла из архива */
        int new_position = ftell(archive);
        int name_size = new_position - prev_position;
        char file_name[name_size + 1];
        fseek(archive, prev_position, SEEK_SET);
        fread(file_name, sizeof(char), name_size, archive);
        printf("Extracting \"%s\"...\n", file_name);

        /* Изменение имени файла для сохранения в указанной папке */
        char full_name[MAX_DIR_NAME] = {0};
        strcpy(full_name, path);
        strcat(full_name, file_name);

        /* Создание файла */
        FILE *extracted_file = fopen(full_name, "wb");
        /* Считывание размера сжатого файла и "хвоста" */
        unsigned char compressed_size_arr[4];
        fread(compressed_size_arr, sizeof(unsigned char), 4, archive);
        unsigned int compressed_size = get_size(compressed_size_arr);
        char tail;
        fread(&tail, sizeof(char), 1, archive);

        /* Проход по файлу и поиск символов с получающимся кодом */
        int pos = 0;
        unsigned char curr_code[MAX_CODE_LEN] = {0};
        char byte_length = 0, bits_in_byte = 8;
        while (pos < compressed_size) {
            unsigned char curr_byte = fgetc(archive);
            ++pos;
            int mult = 128;

            /* Разделение текущего байта на биты */
            if (pos == compressed_size) {
                bits_in_byte = 8 - tail;
            }
            for (int i = 0; i < bits_in_byte; ++i) {
                if ((curr_byte & mult) != 0) {
                    strcat(curr_code, "1");
                }
                else {
                    strcat(curr_code, "0");
                }
                mult /= 2;
                ++byte_length;

                /* Если существуют коды такой длины - совершаем поиск в цепочке */
                if (code_multimap[byte_length - 1].size != 0) {
                    Huffman_table *temp = binsearch(code_multimap[byte_length - 1].codes, 0, code_multimap[byte_length - 1].size - 1, curr_code);
                    /* Если элемент нашёлся, записываем символ в файл, продолжаем работу с текущим байтом */
                    if (temp != NULL) {
                        fputc(temp->symbol, extracted_file);
                        memset(curr_code, 0, MAX_CODE_LEN);
                        byte_length = 0;
                    }
                }
                
            }
        }

        fclose(extracted_file);
        /* Обновление положения начала блока с файлом для работы со следующим файлом архива */
        prev_position = ftell(archive);
    }

    if (strcmp(dirname, "") == 0) {
        strcpy(dirname, "current");
    }
    printf("All files were extracted from \"%s\" archive and saved to %s directory\n", archive_name, dirname);
    fclose(archive);
}


/* Функция для перечисления файлов, хранящихся в архиве */
void list(char *archive_name) {
    FILE *archive = fopen(archive_name, "rb");
    /* Если архив не открылся */
    if (archive == NULL) {
        printf("Unable to open provided archive! Try again!\n");
        exit(EXIT_FAILURE);
    }
    printf("The archive \"%s\" contains following files:\n", archive_name);

    /* Пропуск информации о количестве символов */
    fseek(archive, 2, SEEK_SET);
    /* Считывание информации о размере хедера (кодовой таблицы) и пропуск хедера */
    unsigned char header_size[4];
    fread(header_size, sizeof(unsigned char), 4, archive);
    unsigned int offset = get_size(header_size);
    fseek(archive, offset, SEEK_CUR);
    int symbol = 0;

    /* Считывание информации из файла */
    while (symbol != EOF) {
        while ((symbol = fgetc(archive)) != '\0' && symbol != EOF) {
            printf("%c", symbol);
        }
        if (symbol == EOF) {
            break;
        }
        printf("\n");
        /* Считывание информации о размере файла в архиве и пропуск бинарной информации */
        unsigned char size_arr[4];
        fread(size_arr, sizeof(unsigned char), 4, archive);
        unsigned int data_size = get_size(size_arr);
        fseek(archive, data_size + 1, SEEK_CUR);
    }
    fclose(archive);
}


/* Перевод численного размера массива в представление в виде массива */
unsigned char *count_size(unsigned int size) {
    unsigned char *size_array = (unsigned char *)calloc(4, sizeof(unsigned char));
    for (int i = 0; i < 4; ++i) {
        size_array[i] = size % SIZE_MULTIPLIER;
        size /= SIZE_MULTIPLIER;
    }
    return size_array;
}


/* Перевод представления размера массивом в численное представление */
unsigned int get_size(unsigned char *size_arr) {
    int multiplier_value = 1;
    unsigned int size = 0;
    for (int i = 0; i < 4; ++i) {
        size += multiplier_value * size_arr[i];
        multiplier_value *= SIZE_MULTIPLIER;
    }
    return size;
}


/* Функция для подсчёта частоты встречаемости символов в предоставленных файлах */
void count_entries(int files_number, char *files_names[]) {
    /* Заполнение полей символов */
    for (int i = 0; i < SYMB_TABLE_COUNT; ++i) {
        hash_table[i].symbol = i;
    }

    /* Подсчёт встречаемости символов в файлах */
    for (int i = 0; i < files_number; ++i) {
        FILE *curr_file = fopen(files_names[i], "rb");
        if (curr_file == NULL) {
            continue;
        }
        int symbol;
        while ((symbol = fgetc(curr_file)) != EOF) {
            hash_table[symbol].number_of_entries += 1;
        }
        fclose(curr_file);
    }

    /* Сортировка полей по частоте */
    quick_sort_freq(hash_table, 0, SYMB_TABLE_COUNT - 1);
}


/* Функция для построения кодового дерева */
Node *build_code_tree(int files_number, char *files_names[]) {
    count_entries(files_number, files_names);
    int symbols_count = SYMB_TABLE_COUNT;

    /* Инициализация деревьев в полях таблицы для символов, встречающихся в файлах */
    for (int i = 0; i < SYMB_TABLE_COUNT; ++i) {
        if (hash_table[i].number_of_entries == 0) {
            --symbols_count;
        }
        else {
            hash_table[i].root = (Node *)malloc(sizeof(Node));
            hash_table[i].root->symbol = hash_table[i].symbol;
            hash_table[i].root->frequency = hash_table[i].number_of_entries;
            hash_table[i].root->left = hash_table[i].root->right = NULL;
            hash_table[i].root->code.code_bits[0] = 0;
            hash_table[i].root->code.length = 0;
        }
    }

    /* Если файлы пусты, не кодируем */
    if (symbols_count == 0) {
        return NULL;
    }

    int start = SYMB_TABLE_COUNT - symbols_count; /* Индекс массива, начиная с которого представленные символы входят в файлы */
    Priority_queue queue;                         /* Приоритетная очередь */
    /* Заполнение ячеек приоритетной очереди */
    for (int i = 0; i < symbols_count; ++i) {
        queue.array[i] = hash_table[start + i].root;
    }
    queue.head = 0;
    queue.size = symbols_count;

    /* Объединение поддеревьев с наименьшей частотой встречаемости */
    while (queue.size >= 2) {
        Node *least_1 = queue.array[queue.head];
        Node *least_2 = queue.array[queue.head + 1];
        ++(queue.head);

        /* Создание нового связывающего узла кодового дерева */
        Node *new_node = (Node *)malloc(sizeof(Node));
        new_node->left = least_1;
        new_node->right = least_2;
        new_node->frequency = least_1->frequency + least_2->frequency;
        new_node->symbol = -1;
        new_node->code.code_bits[0] = 0;
        new_node->code.length = 0;

        /* "Сдвиг" очереди */
        --(queue.size);
        queue.array[queue.head] = new_node;
        
        /* Восстановление приоритетных свойств очереди */
        for (int i = 0; i < queue.size - 1; ++i) {
            if (queue.array[queue.head + i]->frequency > queue.array[queue.head + i + 1]->frequency) {
                Node *tmp = queue.array[queue.head + i];
                queue.array[queue.head + i] = queue.array[queue.head + i + 1];
                queue.array[queue.head + i + 1] = tmp;
            }
            else {
                break;
            }
        }
    }

    /* Присвоение кодов элементам дерева */
    assign_codes(queue.array[queue.head]);
    return queue.array[queue.head];
 }


/* Составление новых кодов символов */
void assign_codes(Node *tree) {
    if (tree != NULL) {
        /* Если элемент находится в левом поддереве - добавляем коду символа 0 */
        if (tree->left != NULL) {
            tree->left->code.length = tree->code.length + 1;
            strcpy(tree->left->code.code_bits, tree->code.code_bits);
            strcat(tree->left->code.code_bits, "0");
            assign_codes(tree->left);
        }
        /* Если элемент находится в правом поддереве - добавляем коду символа 1 */
        if (tree->right != NULL) {
            tree->right->code.length = tree->code.length + 1;
            strcpy(tree->right->code.code_bits, tree->code.code_bits);
            strcat(tree->right->code.code_bits, "1");
            assign_codes(tree->right);
        }
    }
}


/* Обход кодового дерева с целью заполнения полей узлов с встреченными символами */ 
void tree_walk(Node *tree, Huffman_table *table) {
    if (tree != NULL) {
        /* Если узел - значащий, то заполняем его поля символом и кодом символа */
        if (tree->symbol != -1) {
            table[tree->symbol].symbol = tree->symbol;
            table[tree->symbol].code = tree->code;
        }
        /* Обходим левую ветвь */
        if (tree->left != NULL) {
            tree_walk(tree->left, table);
        }
        /* Обходим правую ветвь */
        if (tree->right != NULL) {
            tree_walk(tree->right, table);
        }
    }
}


/* Функция для сортировки массива по количеству встреч символа в файлах на основе рекурсивного алгоритма быстрой сортировки */
void quick_sort_freq(Huffman_tree_storage *array, int start, int end) {
    Huffman_tree_storage pivot = array[(start + end) / 2];      /* Опорный элемент */
    int i = start;
    int j = end;

    while (i <= j) {
        while (array[i].number_of_entries < pivot.number_of_entries) {
            ++i;
        }
        while (array[j].number_of_entries > pivot.number_of_entries) {
            --j;
        }
        if (i <= j) {
            Huffman_tree_storage tmp;
            SWAP(array[i], array[j]);
            ++i;
            --j;
        }
    }

    if (start < j) {
        quick_sort_freq(array, start, j);
    }
    if (end > i) {
        quick_sort_freq(array, i, end);
    }
}


/* Функция для сортировки элементов кодовой таблицы на основе рекурсивного алгоритма быстрой сортировки */
void quick_sort_tables(Huffman_table *array, int start, int end) {
    Huffman_table pivot = array[(start + end) / 2];     /* Опорный элемент */
    int i = start;
    int j = end;

    while (i <= j) {
        while (strcmp(array[i].code.code_bits, pivot.code.code_bits) < 0) {
            ++i;
        }
        while (strcmp(array[j].code.code_bits, pivot.code.code_bits) > 0) {
            --j;
        }
        if (i <= j) {
            Huffman_table tmp;
            SWAP(array[i], array[j]);
            ++i;
            --j;
        }
    }

    if (start < j) {
        quick_sort_tables(array, start, j);
    }
    if (end > i) {
        quick_sort_tables(array, i, end);
    }
}


/* Функция, осуществляющая бинарный поиск по массиву кодов символов для раскодирования файлов */
Huffman_table *binsearch(Huffman_table *array, int start, int end, unsigned char to_seek[]) {
    int mid = (start + end) / 2;    /* Индекс середины массива */
    /* Если индекс начала массива не меньше индекса конца и искомый элемент не в середине - элемент не найден */
    if (start >= end && (strcmp(array[mid].code.code_bits, to_seek) != 0)) {
        return NULL;
    }
    /* Возвращаем указатель на найденный элемент массива */
    if (strcmp(array[mid].code.code_bits, to_seek) == 0) {
        return &(array[mid]);
    }
    /* Если код элемента меньше нужного в лексикографическом порядке - ищем в правой части */
    if (strcmp(array[mid].code.code_bits, to_seek) < 0) {
        return binsearch(array, mid + 1, end, to_seek);
    }
    /* Если код элемента больше нужного в лексикографическом порядке - ищем в левой части */
    if (strcmp(array[mid].code.code_bits, to_seek) > 0) {
        return binsearch(array, start, mid - 1, to_seek);
    }
    /* Элемент не найден */
    return NULL;
}


/* Функция для освобождения памяти, выделенной узлам кодового дерева */
void free_tree(Node **tree) {
    while (*tree != NULL) {
        if ((*tree)->left != NULL) {
            free_tree(&((*tree)->left));
        }
        if ((*tree)->right != NULL) {
            free_tree(&((*tree)->right));
        }
        else {
            free(*tree);
            *tree = NULL;
        }
    }
}
