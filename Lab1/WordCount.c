#include <stdio.h>
#include <string.h>
#include <ctype.h>

int options(char *option);
int getLines(FILE *fin, int mode);
enum comandlet {
    
    option_list = 1,
    num_of_lines,
    size_in_bytes,
    num_of_words

};
int main(int argc, char *argv[]) {
    /* Выводим сообщение, сигнализирующее о неверном количестве аргументов */
    if (argc != 3) {
        printf("\nUnsupported input format! Try using this:\n"
        "WordCount.exe [OPTION] filename\n\n"
        "To get the list of available options, type this:\n"
        "\tWordCount.exe --options list\n"
        "or:\n"
        "\tWordCount.exe -o list\n\n");
        return 1;
    }

    int option = options(argv[1]); /* Запрашиваемая опция */
    char *path = argv[2]; /* Путь к файлу */
    FILE *fin = fopen(path, "r"); /* Открытие файла по указанному пути для чтения */

    /* Если файл не открылся */
    if (!fin && option > 1) { 
        printf("\nUnfortunately, there are some problems with opening the file.\n"
        "Check if it is located in the right directory.\n"
        "Also make sure that you spelt its name correctly.\n\n");
        return 2;
    }

    switch (option) {
        
        case 1: /* Для -o, --options - список параметров и описание результата */
            printf("\nAvailable options:\n\n"
            "-o, --options      To see this menu\n"
            "-l, --lines        Returns the quantity of lines in the file\n"
            "-c, --bytes        Returns size of the file in bytes\n"
            "-w, --words        Returns the quantity of words in the file\n\n");
            break;

        case 2: /* Для -l, --lines - подсчёт строк */
            printf("\nThe quantity of lines: %d\n\n", getLines(fin, 2));
            break;

        case 3: /* Для -c, --bytes - размер файла в байтах */
            printf("\nSize of the file: %d bytes\n\n", getLines(fin, 3));
            break;

        case 4: /* Для -w, --words - подсчёт слов */
            printf("\nThe quantity of words: %d\n\n", getLines(fin, 4));
            break;

        default: /* При вводе неопознанного аргумента */
            printf("\nOops! It seems that there is no such option!\n"
            "See the list of available options by typing:\n"
            "WordCount.exe --options list\n\n");
            break;

    }
    
    fclose(fin);
    return 0;
}

/* Функция ассоциирует команду с числовым кодом для использования в switch */
int options(char *argument) {
    enum comandlet;

    if (strcmp(argument, "-o") == 0 || strcmp(argument, "--options") == 0) {
        return option_list;
    }
    if (strcmp(argument, "-l") == 0 || strcmp(argument, "--lines") == 0) {
        return num_of_lines;
    }
    if (strcmp(argument, "-c") == 0 || strcmp(argument, "--bytes") == 0) {
        return size_in_bytes;
    }
    if (strcmp(argument, "-w") == 0 || strcmp(argument, "--words") == 0) {
        return num_of_words;
    }
    return 0;
}


/* Функция осуществляет действия с файлом согласно требуемой опции */
int getLines(FILE *fin, int mode) {
    int c, counter, inWord, isEmpty;
    counter = inWord = 0;
    isEmpty = 1;

    switch (mode) {
        
        case 2: /* Подсчёт количества строк */
            while ((c = fgetc(fin)) != EOF) {
                isEmpty = 0; /* Если считан символ, то файл не пуст */
                if (c == '\n') {
                    counter++; /* Считаем количество переводов строки */
                }
            }
            /* Если файл пуст - 0 строк, иначе - (количество переводов + 1) */
            if (!isEmpty){
                counter++; 
            }
            break;
        
        case 3: /* Подсчёт размера файла в байтах */
            while ((c = fgetc(fin)) != EOF) {
                counter++;  /* Подсчёт количества символов */
                if (c == '\n'){
                    counter++; /* Увеличение из-за возврата каретки в Win */
                }
            }
            break;
        
        case 4: /* Подсчёт количества слов */
            c = fgetc(fin);
            while (c != EOF) {
                /* Если символ не является пробельным */
                if (!isspace(c)) {
                    /* Если зашли в новое слово */
                    if (!inWord) {
                        inWord = 1;  
                        counter++;
                    }
                } 
                else {
                    inWord = 0;
                }
                c = fgetc(fin);
            }
            break;
        
    }
    return counter;
}