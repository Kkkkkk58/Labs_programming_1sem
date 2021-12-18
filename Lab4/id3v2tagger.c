#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>
#define ID3V2_HEADER_SIZE 10

/* Тип, хранящий представление заголовка ID3v2 */
typedef union ID3v2_header {
    char string_version[ID3V2_HEADER_SIZE];
    struct Header {
        char tag[3];
        char version;
        char subversion;
        unsigned char flags;
        unsigned char size_arr[4];
    } fields;
} ID3v2_header;

/* Тип, хранящий представление фрейма ID3v2 */
typedef union ID3v2_frame {
    char string_version[ID3V2_HEADER_SIZE];
    struct Frame {
        char frame_name[4];
        unsigned char size_arr[4];
        unsigned char flags[2];
    } fields;
} ID3v2_frame;


void show_tags(FILE *song, int header_size, char version);
void get_tags(FILE *song, char *tag, int header_size, char version);
void set_tags(FILE *song, char *tag, char *new_value, int header_size, ID3v2_header *header);
char *get_argument(char *argument);
void save_image(char *frame_value, int frame_size);
void get_info(char *frame_name, char *frame_value, int frame_size);
void change_size(ID3v2_frame *frame, int new_size, int divisor);


int main(int argc, char *argv[]) {
    /* Неподдерживаемый формат ввода */
    if (argc < 3) {
        printf("Unsupported input format! Try again!");
        exit(EXIT_FAILURE);
    }
    char *filepath = get_argument(argv[1]);
    FILE *song = fopen(filepath, "rb");
    if (song == NULL) {
        printf("Some errors while reading the mp3 file. Try again!");
        exit(EXIT_FAILURE);
    }
    ID3v2_header header;
    fread(&header, sizeof(ID3v2_header), 1, song);

    /* Проверка существования расширенного заголовка */
    int ext_header_exists = 0;
    if (header.fields.flags != 0) {
        if (header.fields.flags >> 7 == 1) {
            ext_header_exists = 1;
        }
        header.fields.flags = '\0';
    }

    /* Подсчёт размера тега */
    int header_size = 0;
    int multiplier = 128 * 128 * 128;
    for (int i = 0; i < 4; ++i) {
        header_size += header.fields.size_arr[i] * multiplier;
        multiplier /= 128;
    }
    int start_position = ID3V2_HEADER_SIZE;

    /* Подсчёт размера расширенного заголовка, если он присутствует */
    if (ext_header_exists) {
        int ext_header_size = 0;
        /* В четвертой версии все размеры даны с помощью synchsafe integer */
        int degree = 128;
        if (header.fields.version == 3) {
            degree = 256;
            start_position += 4;
        }
        int multiplier = degree * degree * degree;
        unsigned char ext_size_arr[4];
        fread(ext_size_arr, 4, sizeof(char), song);
        for (int i = 0; i < 4; ++i) {
            ext_header_size += ext_size_arr[i] * multiplier;
            multiplier /= degree;
        }
        start_position += 6 + ext_header_size;
    }

    int show_enabled = 0;   /* Если запрашивается опция --show */
    int get_enabled = 0;    /* Если запрашивается опция --get */
    int set_enabled = 0;    /* Если запрашивается опция --set */
    char *frame_to_get, *frame_to_set, *value_to_set;

    /* Парсинг аргументов командной строки */
    for (int arg_index = 2; arg_index < argc; ++arg_index) {
        if (strcmp(argv[arg_index], "--show") == 0) {
            show_enabled = 1;
        }
        else if (argv[arg_index][2] == 'g') {
            frame_to_get = get_argument(argv[arg_index]);
            get_enabled = 1;
        }
        else {
            frame_to_set = get_argument(argv[arg_index]);
            if (argc > arg_index + 1) {
                ++arg_index;
                value_to_set = get_argument(argv[arg_index]);
                set_enabled = 1;
            }
            else {
                printf("Unsupported input format! Try again!");
                exit(EXIT_FAILURE);
            }
        }
    }

    /* Если требуется вывод всех фреймов */
    if (show_enabled) {
        fseek(song, start_position, SEEK_SET);
        show_tags(song, header_size, header.fields.version);
    }

    /* Если запрошен конкретный фрейм */
    if (get_enabled) {
        fseek(song, start_position, SEEK_SET);
        get_tags(song, frame_to_get, header_size, header.fields.version);
    }

    /* Если нужно поменять || дописать фрейм */
    if (set_enabled) {
        fseek(song, start_position, SEEK_SET);
        set_tags(song, frame_to_set, value_to_set, header_size, &header);
        /* Замена старого файла на созданный после изменения тега */
        fclose(song);
        song = NULL;
        remove(filepath);
        rename("temp.mp3", filepath);
        printf("Done!");
    }

    if (song != NULL) {
        fclose(song);
    }
    return 0;
}


/* Функция для получения значения, переданного в качестве аргумента командной строки */
char *get_argument(char *argument) {
    char *name_position = strchr(argument, '=');
    return (name_position + 1);
}


/* Функция для отображения всей метаинформации */
void show_tags(FILE *song, int header_size, char version) {
    int degree = 128;
    if (version == 3) {
        degree = 256;
    }

    int i = 0;
    while (i < header_size) {
        ID3v2_frame frame;
        fread(&frame, sizeof(ID3v2_frame), 1, song);
        /* Название фрейма не начинается с '\0' */
        if (frame.fields.frame_name[0] == '\0') {
            return;
        }

        /* Более удобное представление имени фрейма */
        char frame_name[5];
        memmove(frame_name, frame.string_version, 4);
        frame_name[4] = '\0';
        int multiplier = degree * degree * degree;
        int frame_size = 0;
        
        /* Подсчёт размера фрейма */
        for (int i = 0; i < 4; ++i) {
            frame_size += frame.fields.size_arr[i] * multiplier;
            multiplier /= degree;
        }

        /* Получение значения текущего поля метаинформации */
        char *frame_value = (char *)malloc(frame_size * sizeof(char));
        fread(frame_value, sizeof(char), frame_size, song);
        get_info(frame_name, frame_value, frame_size);
        free(frame_value);
        frame_value = NULL;
        i += ID3V2_HEADER_SIZE + frame_size;
    }
}


/* Функция для вывода определенного поля метаинформации */
void get_tags(FILE *song, char *tag, int header_size, char version) {
    int degree = 128;
    if (version == 3) {
        degree = 256;
    }

    int i = 0;
    int found = 0;
    while (i < header_size) {
        ID3v2_frame frame;
        fread(&frame, sizeof(ID3v2_frame), 1, song);
        /* Название фрейма не начинается с '\0' */
        if (frame.fields.frame_name[0] == '\0') {
            break;
        }

        /* Более удобное представление имени фрейма */
        char frame_name[5];
        memmove(frame_name, frame.string_version, 4);
        frame_name[4] = '\0';

        /* Если требуемый фрейм обнаружен */
        if (strcmp(tag, frame_name) == 0) {
            found = 1;
        }

        /* Подсчёт размера фрейма */
        int multiplier = degree * degree * degree;
        int frame_size = 0;
        for (int i = 0; i < 4; ++i) {
            frame_size += frame.fields.size_arr[i] * multiplier;
            multiplier /= degree;
        }

        /* Получение значения нужного поля метаинформации */
        if (found) {
            char *frame_value = (char *)malloc(frame_size * sizeof(char));
            fread(frame_value, sizeof(char), frame_size, song);
            get_info(tag, frame_value, frame_size);
            free(frame_value);
            return;
        }
        else {
            fseek(song, frame_size, SEEK_CUR);
        }
        i += ID3V2_HEADER_SIZE + frame_size;
    } 

    /* Если фрейм не был найден */
    printf("Tag %s not found\n", tag);
}


/* Функция для редактирования и добавления метаданных */
void set_tags(FILE *song, char *tag, char *new_value, int header_size, ID3v2_header *header) {
    FILE *new_file = fopen("temp.mp3", "wb");
    if (new_file == NULL) {
        printf("Unable to create a temporary file! Try again!\n");
        exit(EXIT_FAILURE);
    }

    int degree = 128;
    if (header->fields.version == 3) {
        degree = 256;
    }
    int divisor = degree;
    int found = 0;
    int old_header_size = header_size;
    int curr_size = strlen(new_value);
    int prev_size = 0;
    /* Буфер для записи тега */
    char *buffer = (char *)malloc((header_size + curr_size + ID3V2_HEADER_SIZE + 5) * sizeof(char));
    int buff_index = 0;
    int iterator = 0;
    while (iterator < header_size - 1) {
        ID3v2_frame frame;
        fread(&frame, sizeof(ID3v2_frame), 1, song);
        /* Название фрейма не начинается с '\0' */
        if (frame.fields.frame_name[0] == '\0') {
            break;
        }

       /* Более удобное представление имени фрейма */
        char frame_name[5];
        memmove(frame_name, frame.string_version, 4);
        frame_name[4] = '\0';

        /* Если заданный фрейм был найден */
        if (strcmp(tag, frame_name) == 0) {
            found = 1;
        }

        /* Подсчёт размера фрейма */
        int multiplier = degree * degree * degree;
        int frame_size = 0;
        for (int j = 0; j < 4; ++j) {
            frame_size += frame.fields.size_arr[j] * multiplier;
            multiplier /= degree;
        }

        /* Обработка найденного фрейма */
        if (found == 1) {
            /* Сохранение предыдущего размера фрейма для перехода к новому в исходном файле */
            prev_size = frame_size;

            /* Обработка текстового фрейма */
            if (frame_name[0] == 'T') {
                change_size(&frame, curr_size + 1, divisor);  
                memmove(&buffer[buff_index], frame.string_version, ID3V2_HEADER_SIZE);
                buff_index += ID3V2_HEADER_SIZE;
                memmove(&buffer[buff_index], "\0", 1);
                buff_index += 1;
            }

            /* Обработка фрейма пользовательских ссылок */
            else if (strcmp(frame_name, "WXXX") == 0) {
                change_size(&frame, curr_size + 2, divisor);    
                memmove(&buffer[buff_index], frame.string_version, ID3V2_HEADER_SIZE);
                buff_index += ID3V2_HEADER_SIZE;
                memmove(&buffer[buff_index], "\0\0", 2);
                buff_index += 2;
            }

            /* Обработка фрейма комментария */
            else if (strcmp(frame_name, "COMM") == 0 || strcmp(frame_name, "USLT") == 0) {
                change_size(&frame, curr_size + 5, divisor);       
                memmove(&buffer[buff_index], frame.string_version, ID3V2_HEADER_SIZE);
                buff_index += ID3V2_HEADER_SIZE;
                memmove(&buffer[buff_index], "\0rus\0", 5);
                buff_index += 5;
            }

            /* Обработка фрейма изображения */
            else if (strcmp(frame_name, "APIC") == 0) {
                FILE *substitute_pic = fopen(new_value, "rb");
                if (substitute_pic == NULL) {
                    printf("Unable to load the image! Try again!\n");
                    remove("temp.mp3");
                    exit(EXIT_FAILURE);
                }
                fseek(substitute_pic, 0, SEEK_END);
                int image_size = ftell(substitute_pic);
                char *image_buf = (char *)malloc(image_size * sizeof(char));
                fseek(substitute_pic, 0, SEEK_SET);
                fread(image_buf, image_size, 1, substitute_pic);
                fclose(substitute_pic);
                int extension_size = 13;
                if (image_buf[0] == (char)0xff && image_buf[1] == (char)0xd8) {
                    ++extension_size;
                }
                buffer = realloc(buffer, header_size + ID3V2_HEADER_SIZE + image_size + extension_size);
                change_size(&frame, image_size + extension_size, divisor);
                memmove(&buffer[buff_index], frame.string_version, ID3V2_HEADER_SIZE);
                buff_index += ID3V2_HEADER_SIZE;
                if (extension_size == 14) {
                    memmove(&buffer[buff_index], "\0image/jpeg\0\03\0", extension_size);
                }
                else {
                    memmove(&buffer[buff_index], "\0image/png\0\03\0", extension_size);
                }
                buff_index += extension_size;
                memmove(&buffer[buff_index], image_buf, image_size);
                buff_index += image_size;
                free(image_buf);
            }

            /* Обработка остальных фреймов */
            else {
                change_size(&frame, curr_size, divisor); 
                memmove(&buffer[buff_index], frame.string_version, ID3V2_HEADER_SIZE);
                buff_index += ID3V2_HEADER_SIZE;
            }    
            if (strcmp(frame_name, "APIC") != 0) {
                memmove(&buffer[buff_index], new_value, curr_size);
                buff_index += curr_size;
            }
            found = 2;

            /* Переход к позиции в исходном файле, где завершался оригинальный фрейм */
            fseek(song, prev_size, SEEK_CUR);
            printf("Changing %s tag value to %s\n", frame_name, new_value);
        }

        /* Запись текущего фрейма в буфер */
        else {
            memmove(&buffer[buff_index], frame.string_version, ID3V2_HEADER_SIZE);
            buff_index += ID3V2_HEADER_SIZE;
            char *frame_value = (char *)malloc(frame_size * sizeof(char));
            fread(frame_value, sizeof(char), frame_size, song);
            memmove(&buffer[buff_index], frame_value, frame_size);
            free(frame_value);
            frame_value = NULL;
            buff_index += frame_size;
        }
    iterator += ID3V2_HEADER_SIZE + frame_size;
    }

    /* Если фрейм не был найден - добавляем его */ 
    if (found == 0) {
        ID3v2_frame frame;
        memmove(frame.string_version, tag, 4);
        memmove(frame.fields.flags, "\0\0", 2);

        /* Обработка текстового фрейма */
        if (tag[0] == 'T') {
            change_size(&frame, curr_size + 1, divisor);  
            memmove(&buffer[buff_index], frame.string_version, ID3V2_HEADER_SIZE);
            buff_index += ID3V2_HEADER_SIZE;
            memmove(&buffer[buff_index], "\0", 1);
            buff_index += 1;
        }

        /* Обработка фрейма пользовательских ссылок */
        else if (strcmp(tag, "WXXX") == 0) {
            change_size(&frame, curr_size + 2, divisor);    
            memmove(&buffer[buff_index], frame.string_version, ID3V2_HEADER_SIZE);
            buff_index += ID3V2_HEADER_SIZE;
            memmove(&buffer[buff_index], "\0\0", 2);
            buff_index += 2;
        }

        /* Обработка фрейма комментария */
        else if (strcmp(tag, "COMM") == 0 || strcmp(tag, "USLT") == 0) {
            change_size(&frame, curr_size + 5, divisor);
            memmove(&buffer[buff_index], frame.string_version, ID3V2_HEADER_SIZE);
            buff_index += ID3V2_HEADER_SIZE;
            memmove(&buffer[buff_index], "\0rus\0", 5);
            buff_index += 5;
        }

        /* Обработка фрейма картинки */
        else if (strcmp(tag, "APIC") == 0) {
            FILE *new_pic = fopen(new_value, "rb");
            if (new_pic == NULL) {
                printf("Unable to load the image! Try again!\n");
                remove("temp.mp3");
                exit(EXIT_FAILURE);
            }
            fseek(new_pic, 0, SEEK_END);
            int image_size = ftell(new_pic);
            char *image_buf = (char *)malloc(image_size * sizeof(char));
            fseek(new_pic, 0, SEEK_SET);
            fread(image_buf, image_size, 1, new_pic);
            fclose(new_pic);
            int extension_size = 13;
            if (image_buf[0] == (char)0xff && image_buf[1] == (char)0xd8) {
                ++extension_size;
            }
            buffer = realloc(buffer, header_size + ID3V2_HEADER_SIZE + image_size + extension_size);
            change_size(&frame, image_size + extension_size, divisor);
            memmove(&buffer[buff_index], frame.string_version, ID3V2_HEADER_SIZE);
            buff_index += ID3V2_HEADER_SIZE;
            if (extension_size == 14) {
                memmove(&buffer[buff_index], "\0image/jpeg\0\03\0", extension_size);
            }
            else {
                memmove(&buffer[buff_index], "\0image/png\0\03\0", extension_size);
            }
            buff_index += extension_size;
            memmove(&buffer[buff_index], image_buf, image_size);
            buff_index += image_size;
            free(image_buf);
        }

        /* Обработка оставшихся фреймов */
        else {
            change_size(&frame, curr_size, divisor); 
            memmove(&buffer[buff_index], frame.string_version, ID3V2_HEADER_SIZE);
            buff_index += ID3V2_HEADER_SIZE;
        }

        /* Запись заданного значения */
        if (strcmp(tag, "APIC") != 0) {
            memmove(&buffer[buff_index], new_value, curr_size);
            buff_index += curr_size;
        }
        printf("Adding %s tag with %s value\n", tag, new_value);
    }

    /* Изменение головного фрейма */
    int new_header_size = buff_index;
    int header_divisor = 128;
    int tmp = new_header_size;
    for (int i = 0; i < 4; ++i) {
        header->fields.size_arr[3 - i] = (char) (tmp % header_divisor);
        tmp /= header_divisor;
    }
    
    /* Запись головного фрейма в файл */
    fwrite(header->string_version, ID3V2_HEADER_SIZE, 1, new_file);
    /* Запись всех сохранённых фреймов в файл */
    fwrite(buffer, new_header_size, 1, new_file);
    free(buffer);
    /* Заполнение файла оставшейся информацией, не связанной с извлечёнными метаданными */
    fseek(song, 0, SEEK_END);
    int bin_data_ends = ftell(song);
    fseek(song, old_header_size + ID3V2_HEADER_SIZE, SEEK_SET);
    int bin_data_begins = ftell(song);
    int bin_data_size = bin_data_ends - bin_data_begins;
    char *bin_data = (char *)malloc(bin_data_size * sizeof(char));
    fread(bin_data, bin_data_size, 1, song);
    fwrite(bin_data, bin_data_size, 1, new_file);
    fclose(new_file);
}


/* Функция, интерпретирующая данные фрейма в зависимости от его типа */
void get_info(char *frame_name, char *frame_value, int frame_size) {
    printf("%s ", frame_name);
    int pos = 0;
    switch (frame_name[0]) {
        /* Обработка изображений */
        case 'A':
            if (strcmp(frame_name, "APIC") == 0) {
                save_image(frame_value, frame_size);
                return;
            }
            break;
        /* Обработка текстовых фреймов и фрейма Ownership */
        case 'T': case 'O': 
            if (frame_size > pos + 2 && frame_value[pos] == (char)0x1 
            && frame_value[pos + 1] == (char)0xff && frame_value[pos + 2] == (char)0xfe) {
                pos += 3;
            }
            while (pos < frame_size) {
                printf("%c", frame_value[pos]);
                ++pos;
            }
            break;
        /* Обработка комментариев и коммерческого фрейма */
        case 'C':
            if (strcmp(frame_name, "COMM") == 0) {
                while (pos < 4) {
                    printf("%c", frame_value[pos]);
                    ++pos;
                }
                printf("\n\t");
            }            
            while (pos < frame_size) {
                if (frame_size > pos + 2 && frame_value[pos] == (char)0x1 && 
                frame_value[pos + 1] == (char)0xff && frame_value[pos + 2] == (char)0xfe) {
                    pos += 3;
                }
                if (strcmp(frame_name, "COMR") == 0) {
                    if (frame_size > pos + 5) {
                        /* Обработка картинки в коммерческом фрейме */
                        if (frame_value[pos] == 'i' && frame_value[pos + 1] == 'm' 
                        && frame_value[pos + 2] == 'a' && frame_value[pos + 3] == 'g' 
                        && frame_value[pos + 4] == 'e' && frame_value[pos + 5] == '\\') {
                            save_image(&frame_value[pos - 1], frame_size - pos);
                            return;
                        }
                    }
                }
                printf("%c", frame_value[pos]);
                ++pos;
            }
            break;
        /* Обработка идентификатора файла, правил использования, транскрипции текста */
        case 'U':
            if (strcmp(frame_name, "UFID") == 0) {
                printf("%s", frame_value);
                return;
            }
            pos = 1;
            while (pos < frame_size) {
                if (strcmp(frame_name, "USLT") == 0 && frame_value[pos] == '\0') {
                    printf("\n\t");
                }
                else {
                    printf("%c", frame_value[pos]);
                }
                ++pos;
            }
            break;
        /* Обработка ссылок */
        case 'W':
            if (strcmp(frame_name, "WXXX") == 0) {
                if (frame_size > pos + 2 && frame_value[pos] == (char)0x1 
                && frame_value[pos + 1] == (char)0xff && frame_value[pos + 2] == (char)0xfe) {
                    pos += 3;
                }
            }
            while (pos < frame_size) {
                printf("%c", frame_value[pos]);
                ++pos;
            }
            break;
        /* Обработка данных о группе */
        case 'G':
            if (strcmp(frame_name, "GRID") == 0) {
                printf("%s", frame_value);
                break;
            }
        default:
            break;
    }
    printf("\n");
}


/* Функция для извлечения изображения из метаинформации mp3-файла */
void save_image(char *frame_value, int frame_size) {
    char buff_name[] = "tmp_image.xxxx";
    int j = 0;
    while (j < 4) {
        buff_name[10 + j] = frame_value[7 + j];
        ++j;
    }
    j += 7;
    if (frame_value[j] == '\0') {
        j += 3;
    }
    else {
        j += 2;
    }
    if (buff_name[10] == 'j') {
        while (frame_value[j] != (char)0xff && frame_value[j + 1] != (char)0xd8) {
            ++j;
        }
    }
    else {
        while (frame_value[j] != (char)0x89 && frame_value[j + 1] != (char)0x50 
        && frame_value[j + 2] != (char)0x4e && frame_value[j + 3] != (char)0x47) {
            ++j;
        }
    }
    FILE *image = fopen(buff_name, "wb");
    if (image == NULL) {
        printf("Unable to save a picture in a file! Try again!\n");
        return;
    }
    printf("Your picture was saved in %s file\n", buff_name);
    fwrite(&frame_value[j], frame_size - j, 1, image);
    fflush(image);
    fclose(image);
}


/* Функция, изменяющая поля тега фрейма, отвечающие за размер данных */
void change_size(ID3v2_frame *frame, int new_size, int divisor) {
    for (int j = 0; j < 4; ++j) {
        frame->fields.size_arr[3 - j] = (char) (new_size % divisor);
        new_size /= divisor;
    }
}
