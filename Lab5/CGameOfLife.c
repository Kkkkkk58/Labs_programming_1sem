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

typedef union Colour_palette_monochrome {
    char string_version[8];
    struct Colours {
        unsigned char first_color[4];
        unsigned char second_color[4];
    } colours;
} Colour_values;

unsigned int get_size(unsigned char *size_arr);
int *new_generation(unsigned char *game_field, unsigned char *tmp, int height, int width);
int coords(int x, int y, int width);
void clearcell(unsigned char *field, int x, int y, int height, int width);
void setcell(unsigned char *field, int x, int y, int height, int width);

int main(int argc, char *argv[]) {
    char *filename;
    char dirname[255] = "";
    int max_iter = 100, dump_freq = 1;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--input") == 0) {
            filename = argv[i + 1];
            i += 1;
        }
        else if (strcmp(argv[i], "--output") == 0) {
            strcpy(dirname, argv[i + 1]);
            i += 1;
        }
        else if (strcmp(argv[i], "--max_iter") == 0) {
            max_iter = atoi(argv[i + 1]);
            i += 1;
        }
        else if (strcmp(argv[i], "--dump_freq") == 0) {
            dump_freq = atoi(argv[i + 1]);
            i += 1;
        }
        else {
            printf("Unsupported argument");
            exit(EXIT_FAILURE);
        }
    }
    FILE *initial_state_image = fopen(filename, "rb");
    BMP_file_header file_header;
    fread(&file_header, sizeof(BMP_file_header), 1, initial_state_image);
    size_t file_size = get_size(file_header.fields.file_size);
    unsigned int data_offset = get_size(file_header.fields.pixel_data_offset);
    BMP_info_v3_header info_header;
    fread(&info_header, sizeof(BMP_info_v3_header), 1, initial_state_image);
    Colour_values colours;
    fread(&colours, sizeof(Colour_values), 1, initial_state_image);
    unsigned int binary_data_size = file_size - data_offset;
    unsigned int height = get_size(info_header.fields.image_height);
    unsigned int width = get_size(info_header.fields.image_width);
    unsigned int size = height * width;
    unsigned char *file_buffer = (unsigned char *)malloc(binary_data_size);
    fread(file_buffer, binary_data_size, 1, initial_state_image);
    fclose(initial_state_image);
    unsigned char *game_field = (unsigned char *)calloc(size, 1);
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
                setcell(game_field, x, y, height, width);
            }
        }
    }    
    unsigned char *tmp = (unsigned char *)calloc(height, width);
    char path[255] = {0};
    if (strcmp(dirname, "") != 0) { 
        mkdir(dirname);
        sprintf(path, "%s\\", dirname);
    }
    char new_name[255] = {0};
    strcpy(new_name, path);
    strcat(new_name, "0.bmp");
    CopyFile(filename, new_name, TRUE);
 
    for (int i = 1; i <= max_iter; ++i) {
        int *game_continues = new_generation(game_field, tmp, height, width);
        memcpy(game_field, tmp, height * width);
        if (i % dump_freq == 0) {
            unsigned char *interpretation = (unsigned char *)calloc(1, binary_data_size);
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
            FILE *temp = fopen(file_path, "wb");
            fwrite(file_header.string_version, sizeof(BMP_file_header), 1, temp);
            fwrite(info_header.string_version, sizeof(BMP_info_v3_header), 1, temp);
            fwrite(colours.string_version, sizeof(Colour_values), 1, temp);
            fwrite(interpretation, binary_data_size, 1, temp);
            fclose(temp);
     
            BITMAP bm;
            HDC hdc_window = GetDC(NULL);
            HBITMAP hBitmap = (HBITMAP) LoadImage(NULL, file_path, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
            GetObject(hBitmap, sizeof(BITMAP), &bm);
            HDC hdcMem = CreateCompatibleDC(hdc_window);
            SelectObject(hdcMem,hBitmap);
            BitBlt(hdc_window, 10, 20, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
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
        free(game_continues);
    }
    if (strcmp(dirname, "") != 0) {
        printf("Results are saved in %s directory\n", dirname);
    }
    free(file_buffer);
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


void setcell(unsigned char *field, int x, int y, int height, int width) {
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


void clearcell(unsigned char *field, int x, int y, int height, int width) {
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


int *new_generation(unsigned char *game_field, unsigned char *tmp, int height, int width) {
    int size = height * width;
    memcpy(tmp, game_field, size);
    int life_exists = 0;
    int life_remains_stable = 1;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            unsigned char current = game_field[coords(x, y, width)];
            if (current == 0) {
                continue;
            }
            int neighbours = (current & 0xe) / 2;
            if (current & 0x01) {
                if (neighbours != 2 && neighbours != 3) {
                    life_remains_stable  = 0;
                    clearcell(tmp, x, y, height, width);
                }
                else {
                    life_exists = 1;
                }
            }
            else {
                if (neighbours == 3) {
                    setcell(tmp, x, y, height, width);
                    life_exists = 1;
                    life_remains_stable = 0;
                }
            }
        }
    }
    int *game_continues = (int *)malloc(sizeof(int));
    game_continues[0] = life_remains_stable;
    game_continues[1] = life_exists;
    return game_continues;
}