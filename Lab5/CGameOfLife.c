#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <io.h>
#define SIZE_COUNT_MULTIPLIER 256
#define MAX_DIR_NAME 255
#define MAX_ITER_DEFAULT 1000
#define DUMP_FREQ_DEFAULT 1
#define DISPLAY_OFFSET_DEFAULT 0

/* Объединение для хранения информации основного хедера BMP-файла */
typedef union BMP_file_header {
    char string_version[14];
    /* Структура для хранения всех полей с информацией */
    struct BMP_file_header_fields {
        char file_type[2];
        unsigned char file_size[4];
        char reserved[4];
        unsigned char pixel_data_offset[4];
    } fields;
} BMP_file_header;

/* Объeдинение для хранения данных информационного хедера BMP-файла */
typedef union BMP_info_header {
    char string_version[40];
    /* Структура для хранения всех полей с данными */
    struct BMP_info_header_fields {
        char header_size[4];
        char image_width[4];
        char image_height[4];
        char planes[2];
        char bits_per_pixel[2];
        char compression[4];
        unsigned char image_size[4];
        char x_pixels_per_meter[4];
        char y_pixels_per_meter[4];
        unsigned char total_colours[4];
        unsigned char important_colours[4];
    } fields;
} BMP_info_header;

/* Объединение для хранения информации цветовой палитры BMP-файла */
typedef union Colour_palette_monochrome {
    char string_version[8];
    /* Структура для хранения значений цветов в палитре */
    struct BGR_Colours {
        unsigned char first_colour[4];
        unsigned char second_colour[4];
    } colours;
} Colour_values;

static int center_x, center_y;
size_t get_size(unsigned char *size_arr);
int *set_generation(unsigned char *game_field, unsigned char *new_generation, size_t height, size_t width);
int coords(int x, int y, size_t width);
int *get_neighbours(int x, int y, size_t height, size_t width);
void wipe_cell(unsigned char *field, int x, int y, size_t height, size_t width);
void create_cell(unsigned char *field, int x, int y, size_t height, size_t width);
unsigned char *set_colour(unsigned char *argument);
void display_image(char *file_path, int display_offset);
void get_desktop_center(void);


int main(int argc, char *argv[]) {
    /* Установка значений по умолчанию настраиваемых параметров */
    char dirname[MAX_DIR_NAME] = "", *filename;
    unsigned int max_iter = MAX_ITER_DEFAULT, dump_freq = DUMP_FREQ_DEFAULT, \
    display_enabled = 0, display_offset = DISPLAY_OFFSET_DEFAULT;
    unsigned char *first_colour = NULL, *second_colour = NULL;

    /* Считывание аргументов командной строки */
    for (int i = 1; i < argc; ++i) {
        /* Указание BMP-файла с начальной ситуацией игры */
        if (strcmp(argv[i], "--input") == 0) {
            filename = argv[i + 1];
            ++i;
        }
        /* Указание директории для сохранения изображений */
        else if (strcmp(argv[i], "--output") == 0) {
            strcpy(dirname, argv[i + 1]);
            ++i;
        }
        /* Максимальное количество поколений */
        else if (strcmp(argv[i], "--max_iter") == 0) {
            max_iter = atoi(argv[i + 1]);
            ++i;
        }
        /* Частота сохранения изображений */
        else if (strcmp(argv[i], "--dump_freq") == 0) {
            dump_freq = atoi(argv[i + 1]);
            ++i;
        }
        /* Указание на включение опции показа изображений */
        else if (strcmp(argv[i], "--show") == 0) {
            display_enabled = 1;
            get_desktop_center();
        }
        /* Частота показа изображений */
        else if (strcmp(argv[i], "--show_freq") == 0) {
            display_offset = atoi(argv[i + 1]);
            ++i;
        }
        /* Указание на смену первого цвета изображения */
        else if (strcmp(argv[i], "--first_colour") == 0) {
            first_colour = set_colour(argv[i + 1]);
            ++i;
        }
        /* Указание на смену второго цвета изображения */
        else if (strcmp(argv[i], "--second_colour") == 0) {
            second_colour = set_colour(argv[i + 1]);
            ++i;
        }
        /* Отображение справки или обработка неверного аргумента */
        else {
            if (strcmp("--help", argv[i]) != 0) {
                printf("Unsupported argument! Try again!\n");
            }
            printf("\nAvailable options:\n\n"
            "--input [filename]\tThe name of the bmp file with the first generation depicted\n\n"
            "--output [dirname]\tThe name of the folder to save data in\n\n"
            "--max_iter [number]\tDetermines how many generations should be created\n"
            "\t\t\tBy default is 1000\n\n"
            "--dump_freq [number]\tSets frequency of saving pictures for current generations\n"
            "\t\t\tBy default is 1\n\n"
            "--show\t\t\tEnables the option of showing images on the screen\n"
            "\t\t\tBy default is disabled\n\n"
            "--show_freq [number]"
            "\tIf --show enabled, sets frequency of changing pictures on the screen\n"
            "\t\t\tBy default is 0 - no pause between pictures\n\n"
            "--first_colour [hex-colour-value]\tChanges the first colour of the picture\n\n"
            "--second_colour [hex-colour-value]\tChanges the second colour of the picture\n\n");
            if (strcmp(argv[i], "--help") != 0) {
                exit(EXIT_FAILURE);
            }
            else {
                exit(EXIT_SUCCESS);
            }
        }
    }

    FILE *initial_state_image = fopen(filename, "rb");
    /* Если файл невозможно открыть */
    if (initial_state_image == NULL) {
        printf("Unable to open the given file! Try again!\n");
        exit(EXIT_FAILURE);
    }

    /* Считывание данных BMP-файла */
    BMP_file_header file_header;
    fread(&file_header, sizeof(BMP_file_header), 1, initial_state_image);
    size_t file_size = get_size(file_header.fields.file_size);
    size_t data_offset = get_size(file_header.fields.pixel_data_offset);
    BMP_info_header info_header;
    fread(&info_header, sizeof(BMP_info_header), 1, initial_state_image);

    /* Если изображение - не монохромное */
    if (info_header.fields.bits_per_pixel[0] != (char) 1) {
        printf("The picture provided is not monochrome! Try again with a suitable image!\n");
        exit(EXIT_FAILURE);
    }

    Colour_values palette;
    fread(&palette, sizeof(Colour_values), 1, initial_state_image);
    if (first_colour != NULL) {
        strcpy(palette.colours.first_colour, first_colour);
        free(first_colour);
    }
    if (second_colour != NULL) {
        strcpy(palette.colours.second_colour, second_colour);
        free(second_colour);
    }

    size_t binary_data_size = file_size - data_offset;
    size_t height = get_size(info_header.fields.image_height);
    size_t width = get_size(info_header.fields.image_width);
    size_t size = height * width;
    unsigned char *file_buffer = (unsigned char *)malloc(binary_data_size);
    fread(file_buffer, binary_data_size, sizeof(unsigned char), initial_state_image);
    fclose(initial_state_image);
    unsigned char *game_field = (unsigned char *)calloc(size, sizeof(unsigned char));

    /* Преобразование изображения в поле, раскладывание байтов на отдельные пиксели */
    for (int i = 0; i < size / 8; ++i) {
        unsigned char current_byte = file_buffer[i];
        for (int j = 7; j >= 0; --j) {
            game_field[8 * i + j] = current_byte & 0x01;
            current_byte >>= 1;
        }
    }

    /* Задание количества соседей клеткам в начальной ситуации */
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if ((game_field[coords(x, y, width)]) & 0x01) {
                create_cell(game_field, x, y, height, width);
            }
        }
    }    

    /* Создание директории, в которую будут сохраняться изображения */
    char path[MAX_DIR_NAME] = {0};
    if (strcmp(dirname, "") != 0) { 
        mkdir(dirname);
        sprintf(path, "%s\\", dirname);
    }
    char new_name[MAX_DIR_NAME] = {0};
    strcpy(new_name, path);
    strcat(new_name, "0.bmp");
    /* Изменение цветов в исходном изображении */
    if (first_colour != NULL || second_colour != NULL) {
        FILE *first_generation_coloured = fopen(new_name, "wb");
        fwrite(file_header.string_version, sizeof(BMP_file_header), 1, first_generation_coloured);
        fwrite(info_header.string_version, sizeof(BMP_info_header), 1, first_generation_coloured);
        fwrite(palette.string_version, sizeof(Colour_values), 1, first_generation_coloured);
        fwrite(file_buffer, binary_data_size, 1, first_generation_coloured);
        fclose(first_generation_coloured);
        free(file_buffer);
    }
    else {
        CopyFile(filename, new_name, TRUE);
    }

    /* Отображение первой картинки, если требуется */
    if (display_enabled) {
        display_image(new_name, display_offset);
        printf("Press Enter to start creating new generations\n");
        getchar();
    }

    int stats_cells_born = 0;   /* Переменная, хранящая данные о количестве рождённых клеток */
    int stats_cells_died = 0;   /* Переменная, хранящая данные о количестве умерших клеток */
    int generations_count = 1;  /* Переменная, хранящая данные о текущем количестве поколений */

    /* Основной цикл программы */
    unsigned char *new_generation = (unsigned char *)calloc(height, width);
    while (generations_count <= max_iter) {
        int *game_stats = set_generation(game_field, new_generation, height, width);
        memcpy(game_field, new_generation, height * width);

        /* Сохранение текущего поколения в файл */
        if (generations_count % dump_freq == 0) {
            unsigned char *interpretation = (unsigned char *)calloc(binary_data_size, \
            sizeof(unsigned char));
            for (int k = 0; k < size / 8; ++k) {
                unsigned char single_byte;
                for (int j = 0; j < 8; ++j) {
                    if (game_field[8 * k + j] & 0x1) {
                        single_byte |= (1 << (7 - j));
                    } 
                    else {
                        single_byte &= ~(1 << (7 - j));
                    }
                }
                interpretation[k] = single_byte;
            }
            char name_buf[15];
            itoa(generations_count, name_buf, 10);
            strcat(name_buf, ".bmp");
            char file_path[MAX_DIR_NAME];
            strcpy(file_path, path);
            strcat(file_path, name_buf);
            FILE *generation_image = fopen(file_path, "wb");
            fwrite(file_header.string_version, sizeof(BMP_file_header), 1, generation_image);
            fwrite(info_header.string_version, sizeof(BMP_info_header), 1, generation_image);
            fwrite(palette.string_version, sizeof(Colour_values), 1, generation_image);
            fwrite(interpretation, binary_data_size, 1, generation_image);
            fclose(generation_image);

            /* Отображение текущего изображения */
            if (display_enabled) {
                display_image(file_path, display_offset);
            }
        } 

        /* Обработка ситуации, когда не осталось живых клеток */
        if (game_stats[1] == 0) {
            free(game_stats);
            printf("Population died at %d iteration\n", generations_count);
            break;
        }

        /* Обработка случая, когда популяция клеток перестаёт меняться */
        if (game_stats[0] == 1) {
            free(game_stats);
            printf("Population remains stable after %d iterations\n", generations_count);
            break;
        }

        stats_cells_born += game_stats[2];
        stats_cells_died += game_stats[3]; 
        free(game_stats);
        ++generations_count;
    }
    free(game_field);
    free(new_generation);

    /* Вывод результатов */
    if (strcmp(dirname, "") == 0) {
        strcpy(dirname, "current");
    }
    if (generations_count == max_iter + 1) {
        printf("The colony managed to survive for %d generations\n", max_iter);
    }
    printf("%d new cells were born, %d cells died through that time\n", stats_cells_born, stats_cells_died);
    printf("Results are saved in %s directory\n", dirname);

    return 0;
}


/*Функция для преобразования информации о различных размерах в BMP-файле */
size_t get_size(unsigned char *size_arr) {
    int multiplier_value = 1;
    size_t size = 0;
    for (int i = 0; i < 4; ++i) {
        size += multiplier_value * size_arr[i];
        multiplier_value *= SIZE_COUNT_MULTIPLIER;
    }
    return size;
}


/* Функция для преобразования координат в индекс одномерного массива */
int coords(int x, int y, size_t width) {
    return width * y + x;
}


/* Функция для получения координат соседей клетки */
int *get_neighbours(int x, int y, size_t height, size_t width) {
    int left, right, top, bottom;
    if (x == 0) {
        left = width - 1;
    }
    else {
        left = -1;
    }
    if (x == width - 1) {
        right = -width + 1;
    }
    else {
        right = 1;
    }
    if (y == 0) {
        bottom = height - 1;
    }
    else {
        bottom = -1;
    }
    if (y == height - 1) {
        top = -height + 1;
    }
    else {
        top = 1;
    }
    int *neighbours_coords = (int *)malloc(8 * sizeof(int));
    neighbours_coords[0] = coords(x + left, y, width);
    neighbours_coords[1] = coords(x + right, y, width);
    neighbours_coords[2] = coords(x, y + bottom, width);
    neighbours_coords[3] = coords(x, y + top, width);
    neighbours_coords[4] = coords(x + left, y + top, width);
    neighbours_coords[5] = coords(x + left, y + bottom, width);
    neighbours_coords[6] = coords(x + right, y + top, width);
    neighbours_coords[7] = coords(x + right, y + bottom, width);
    return neighbours_coords;
}


/* Функция, задающая на поле новую клетку и изменяющая состояние её соседей */
void create_cell(unsigned char *field, int x, int y, size_t height, size_t width) {
    /* Установка состояния клетки в единицу - клетка жива */
    field[coords(x, y, width)] |= 0x01;

    /* Изменение состояния соседей */
    int *neighbours_coords = get_neighbours(x, y, height, width);
    for (int i = 0; i < 8; ++i) {
        field[neighbours_coords[i]] += 0x02;
    }
    free(neighbours_coords);
}


/* Функция, убивающая клетку и изменяющая состояние её соседей */
void wipe_cell(unsigned char *field, int x, int y, size_t height, size_t width) {
    /* Установка состояния клетки в ноль - клетка мертва */
    field[coords(x, y, width)] &= ~(0x01);

    /* Изменение состояния соседей */
    int *neighbours_coords = get_neighbours(x, y, height, width);
    for (int i = 0; i < 8; ++i) {
        field[neighbours_coords[i]] -= 0x02;
    }
    free(neighbours_coords);
}


/* Функция, синтезирующая очередное поколение клеток */
int *set_generation(unsigned char *game_field, unsigned char *new_generation, size_t height, size_t width) {
    size_t size = height * width;
    memcpy(new_generation, game_field, size);
    int life_exists = 0, life_remains_stable = 1, cells_born = 0, cells_died = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char current = game_field[coords(x, y, width)];
            /* Пропускаем мёртвую клетку без соседей */
            if (current == 0) {
                continue;
            }

            int neighbours = (current & 0xE) >> 1;
            /* Если клетка жива */
            if (current & 0x01 != 0) {
                /* Клетка умирает от перенаселения или от одиночества */
                if (neighbours != 2 && neighbours != 3) {
                    life_remains_stable  = 0;
                    cells_died += 1;
                    wipe_cell(new_generation, x, y, height, width);
                }
                /* Выживает */
                else {
                    life_exists = 1;
                }
            }
            /* Если клетка мертва */
            else {
                /* Клетка рождается */
                if (neighbours == 3) {
                    create_cell(new_generation, x, y, height, width);
                    life_exists = 1;
                    life_remains_stable = 0;
                    cells_born += 1;
                }
            }
        }
    }
    /* Подведение статистики поколения */
    int *game_stats = (int *)malloc(4 * sizeof(int));
    game_stats[0] = life_remains_stable;
    game_stats[1] = life_exists;
    game_stats[2] = cells_born;
    game_stats[3] = cells_died;
    return game_stats;
}


/* Функция для отображения картинок */
void display_image(char *file_path, int display_offset) {
    BITMAP bm;
    HDC hdc_window = GetDC(NULL);
    HBITMAP h_bitmap = (HBITMAP)LoadImage(NULL, file_path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    GetObject(h_bitmap, sizeof(BITMAP), &bm);
    HDC hdc_mem = CreateCompatibleDC(hdc_window);
    SelectObject(hdc_mem, h_bitmap);
    BitBlt(hdc_window, center_x, center_y, \
    bm.bmWidth, bm.bmHeight, hdc_mem, 0, 0, SRCCOPY);
    Sleep(display_offset);
}


/* Функция для получения координат центра экрана */
void get_desktop_center(void) {
    center_x = GetSystemMetrics(SM_CXSCREEN) / 2;
    center_y = GetSystemMetrics(SM_CYSCREEN) / 2;
}


/* Функция для перевода hex-кода цвета в формат, используемый в палитре BMP-файла */
unsigned char *set_colour(unsigned char *argument) {
    unsigned char *colour_interpretation = (unsigned char *)calloc(4, sizeof(unsigned char));
    for (int j = 0; j < 3; ++j) {
        unsigned char channel_value = 0;
        int multiplier = 16;
        for (int k = 0; k < 2; ++k) {
            unsigned char symbol = argument[2 * j + k];
            if (symbol <= '9') {
                channel_value += (symbol - '0') * multiplier;
            }
            else if (symbol <= 'F') {
                channel_value += (symbol - 'A' + 10) * multiplier;
            }
            else {
                channel_value += (symbol - 'a' + 10) * multiplier;
            }
            multiplier = 1;
        }
        colour_interpretation[2 - j] = channel_value;
    }
    return colour_interpretation;
}
