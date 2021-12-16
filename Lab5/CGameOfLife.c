#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include <io.h>
#define SIZE_COUNT_MULTIPLIER 256

typedef union BMP_file_header {
    char string_version[14];
    struct BMP_file_header_fields {
        char file_type[2];
        unsigned char file_size[4];
        char reserved[4];
        unsigned char pixel_data_offset[4];
    } fields;
} BMP_file_header;

typedef union BMP_core_header {
    char string_version[12];
    struct BMP_core_header_fields {
        char header_size[4];
        unsigned char image_width[2];
        unsigned char image_height[2];
        char planes[2];
        char bits_per_pixel[2];
    } fields;
} BMP_core_header;

typedef union BMP_info_v3_header {
    char string_version[40];
    struct BMP_info_v3_header_fields {
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
} BMP_info_v3_header;

typedef union BMP_info_v4_header {
    char string_version[108];
    struct BMP_info_v4_header_fields {
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
        char other_values[68];
    } fileds;
} BMP_info_v4_header;

typedef union BMP_info_v5_header {
    char string_version[124];
    struct BMP_info_v5_header_fields {
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
        char other_values[84];
    } fields;
} BMP_info_v5_header;

typedef union Colour_palette_monochrome {
    char string_version[8];
    struct Colours {
        unsigned char first_color[4];
        unsigned char second_color[4];
    } colours;
} Colour_values;

static int center_x, center_y;
unsigned int get_size(unsigned char *size_arr);
int *set_generation(unsigned char *game_field, unsigned char *new_generation, int height, int width);
int coords(int x, int y, int width);
void wipe_cell(unsigned char *field, int x, int y, int height, int width);
void bear_cell(unsigned char *field, int x, int y, int height, int width);
unsigned char *set_colour(unsigned char *argument);
void display_image(char *file_path, int display_offset);
void get_desktop_center(void);

int main(int argc, char *argv[]) {
    char *filename;
    char dirname[255] = "";
    int max_iter = 100, dump_freq = 1, display_enabled = 0, display_offset = 0;
    unsigned char *first_colour = NULL, *second_colour = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--input") == 0) {
            filename = argv[i + 1];
            ++i;
        }
        else if (strcmp(argv[i], "--output") == 0) {
            strcpy(dirname, argv[i + 1]);
            ++i;
        }
        else if (strcmp(argv[i], "--max_iter") == 0) {
            max_iter = atoi(argv[i + 1]);
            ++i;
        }
        else if (strcmp(argv[i], "--dump_freq") == 0) {
            dump_freq = atoi(argv[i + 1]);
            ++i;
        }
        else if (strcmp(argv[i], "--show") == 0) {
            display_enabled = 1;
            get_desktop_center();
        }
        else if (strcmp(argv[i], "--show_freq") == 0) {
            display_offset = atoi(argv[i + 1]);
            ++i;
        }
        else if (strcmp(argv[i], "--first_colour") == 0) {
            first_colour = set_colour(argv[i + 1]);
            ++i;
        }
        else if (strcmp(argv[i], "--second_colour") == 0) {
            second_colour = set_colour(argv[i + 1]);
            ++i;
        }
        else {
            printf("Unsupported argument! Try again!\n\n"
            "Available options:\n\n"
            "--input [filename]\tThe name of the bmp file with the first generation depicted\n\n"
            "--output [dirname]\tThe name of the folder to save data in\n\n"
            "--max_iter [number]\tDetermines how many generations should be created\n"
            "\t\t\tBy default is 100\n\n"
            "--dump_freq [number]\tSets frequency of saving pictures for current generations\n"
            "\t\t\tBy default is 1\n\n"
            "--show\t\t\tEnables the option of showing images on the screen\n"
            "\t\t\tBy default is disabled\n\n"
            "--show_freq [number]\tIf --show enabled, sets frequency of changing pictures on the screen\n"
            "\t\t\tBy default is 0 - no pause between pictures\n\n"
            "--first_colour [hex-colour-value]\tChanges the first colour of the picture\n\n"
            "--second_colour [hex-colour-value]\tChanges the second colour of the picture");
            exit(EXIT_FAILURE);
        }
    }
    FILE *initial_state_image = fopen(filename, "rb");
    if (initial_state_image == NULL) {
        printf("Unable to open the given file! Try again!\n");
        exit(EXIT_FAILURE);
    }
    BMP_file_header file_header;
    fread(&file_header, sizeof(BMP_file_header), 1, initial_state_image);
    size_t file_size = get_size(file_header.fields.file_size);
    size_t data_offset = get_size(file_header.fields.pixel_data_offset);
    BMP_info_v3_header info_header;
    fread(&info_header, sizeof(BMP_info_v3_header), 1, initial_state_image);
    if (info_header.fields.bits_per_pixel[0] != (char)1) {
        printf("The picture provided is not monochrome! Try again with a suitable image!\n");
        exit(EXIT_FAILURE);
    }
    Colour_values palette;
    fread(&palette, sizeof(Colour_values), 1, initial_state_image);
    if (first_colour != NULL) {
        strcpy(palette.colours.first_color, first_colour);
        free(first_colour);
    }
    if (second_colour != NULL) {
        strcpy(palette.colours.second_color, second_colour);
        free(second_colour);
    }
    unsigned int binary_data_size = file_size - data_offset;
    unsigned int height = get_size(info_header.fields.image_height);
    unsigned int width = get_size(info_header.fields.image_width);
    unsigned int size = height * width;
    unsigned char *file_buffer = (unsigned char *)malloc(binary_data_size);
    fread(file_buffer, binary_data_size, sizeof(unsigned char), initial_state_image);
    fclose(initial_state_image);
    unsigned char *game_field = (unsigned char *)calloc(size, sizeof(unsigned char));
    for (int i = 0; i < size / 8; ++i) {
        unsigned char current_byte = file_buffer[i];
        for (int j = 7; j >= 0; --j) {
            game_field[8 * i + j] = current_byte & 0x01;
            current_byte >>= 1;
        }
    }
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if ((game_field[coords(x, y, width)]) & 0x01) {
                bear_cell(game_field, x, y, height, width);
            }
        }
    }    
    unsigned char *new_generation = (unsigned char *)calloc(height, width);
    char path[255] = {0};
    if (strcmp(dirname, "") != 0) { 
        mkdir(dirname);
        sprintf(path, "%s\\", dirname);
    }
    char new_name[255] = {0};
    strcpy(new_name, path);
    strcat(new_name, "0.bmp");
    if (first_colour != NULL || second_colour != NULL) {
        FILE *first_generation_coloured = fopen(new_name, "wb");
        fwrite(file_header.string_version, sizeof(BMP_file_header), 1, first_generation_coloured);
        fwrite(info_header.string_version, sizeof(BMP_info_v3_header), 1, first_generation_coloured);
        fwrite(palette.string_version, sizeof(Colour_values), 1, first_generation_coloured);
        fwrite(file_buffer, binary_data_size, 1, first_generation_coloured);
        fclose(first_generation_coloured);
        free(file_buffer);
    }
    else {
        CopyFile(filename, new_name, TRUE);
    }
    if (display_enabled) {
        display_image(filename, display_offset);
    }
    int stats_cells_born = 0;
    int stats_cells_died = 0;
    for (int i = 1; i <= max_iter; ++i) {
        int *game_continues = set_generation(game_field, new_generation, height, width);
        memcpy(game_field, new_generation, height * width);
        if (i % dump_freq == 0) {
            unsigned char *interpretation = (unsigned char *)calloc(binary_data_size, sizeof(unsigned char));
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
            itoa(i, name_buf, 10);
            strcat(name_buf, ".bmp");
            char file_path[255];
            strcpy(file_path, path);
            strcat(file_path, name_buf);
            FILE *generation_image = fopen(file_path, "wb");
            fwrite(file_header.string_version, sizeof(BMP_file_header), 1, generation_image);
            fwrite(info_header.string_version, sizeof(BMP_info_v3_header), 1, generation_image);
            fwrite(palette.string_version, sizeof(Colour_values), 1, generation_image);
            fwrite(interpretation, binary_data_size, 1, generation_image);
            fclose(generation_image);
            if (display_enabled) {
                display_image(file_path, display_offset);
            }
        } 
        if (game_continues[1] == 0) {
            free(game_continues);
            printf("Population died at %d iteration\n", i);
            break;
        }
        if (game_continues[0] == 1) {
            free(game_continues);
            printf("Population remains stable after %d iterations\n", i);
            break;
        }
        stats_cells_born += game_continues[2];
        stats_cells_died += game_continues[3]; 
        free(game_continues);
    }
    if (strcmp(dirname, "") == 0) {
        strcpy(dirname, "current");
    }
    printf("The colony managed to survive %d generations\n", max_iter);
    printf("%d new cells were born, %d cells died through that time\n", stats_cells_born, stats_cells_died);
    printf("Results are saved in %s directory\n", dirname);
    return 0;
}


unsigned int get_size(unsigned char *size_arr) {
    int multiplier_value = 1;
    unsigned int size = 0;
    for (int i = 0; i < 4; ++i) {
        size += multiplier_value * size_arr[i];
        multiplier_value *= SIZE_COUNT_MULTIPLIER;
    }
    return size;
}


int coords(int x, int y, int width) {
    return width * y + x;
}


void bear_cell(unsigned char *field, int x, int y, int height, int width) {
    field[coords(x, y, width)] |= 0x01;
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
    field[coords(x + left, y, width)] += 0x02;
    field[coords(x + right, y, width)] += 0x02;
    field[coords(x, y + bottom, width)] += 0x02;
    field[coords(x, y + top, width)] += 0x02;
    field[coords(x + left, y + top, width)] += 0x02;
    field[coords(x + left, y + bottom, width)] += 0x02;
    field[coords(x + right, y + top, width)] += 0x02;
    field[coords(x + right, y + bottom, width)] += 0x02;
}


void wipe_cell(unsigned char *field, int x, int y, int height, int width) {
    field[coords(x, y, width)] &= ~(0x01);
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
    field[coords(x + left, y, width)] -= 0x02;
    field[coords(x + right, y, width)] -= 0x02;
    field[coords(x, y + bottom, width)] -= 0x02;
    field[coords(x, y + top, width)] -= 0x02;
    field[coords(x + left, y + top, width)] -= 0x02;
    field[coords(x + left, y + bottom, width)] -= 0x02;
    field[coords(x + right, y + top, width)] -= 0x02;
    field[coords(x + right, y + bottom, width)] -= 0x02;  
}


int *set_generation(unsigned char *game_field, unsigned char *new_generation, int height, int width) {
    int size = height * width;
    memcpy(new_generation, game_field, size);
    int life_exists = 0;
    int life_remains_stable = 1;
    int new_cells_born = 0;
    int cells_died = 0;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char current = game_field[coords(x, y, width)];
            if (current == 0) {
                continue;
            }
            int neighbours = (current & 0xE) / 2;
            if (current & 0x01) {
                if (neighbours != 2 && neighbours != 3) {
                    life_remains_stable  = 0;
                    cells_died += 1;
                    wipe_cell(new_generation, x, y, height, width);
                }
                else {
                    life_exists = 1;
                }
            }
            else {
                if (neighbours == 3) {
                    bear_cell(new_generation, x, y, height, width);
                    life_exists = 1;
                    life_remains_stable = 0;
                    new_cells_born += 1;
                }
            }
        }
    }
    int *game_continues = (int *)malloc(4 * sizeof(int));
    game_continues[0] = life_remains_stable;
    game_continues[1] = life_exists;
    game_continues[2] = new_cells_born;
    game_continues[3] = cells_died;
    return game_continues;
}


void display_image(char *file_path, int display_offset) {
    BITMAP bm;
    HDC hdc_window = GetDC(NULL);
    HBITMAP h_bitmap = (HBITMAP)LoadImage(NULL, file_path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
    GetObject(h_bitmap, sizeof(BITMAP), &bm);
    HDC hdc_mem = CreateCompatibleDC(hdc_window);
    SelectObject(hdc_mem, h_bitmap);
    BitBlt(hdc_window, center_x, center_y, 
    bm.bmWidth, bm.bmHeight, hdc_mem, 0, 0, SRCCOPY);
    Sleep(display_offset);
}


void get_desktop_center(void) {
    center_x = GetSystemMetrics(SM_CXSCREEN) / 2;
    center_y = GetSystemMetrics(SM_CYSCREEN) / 2;
}


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
        colour_interpretation[j] = channel_value;
    }
    return colour_interpretation;
}
