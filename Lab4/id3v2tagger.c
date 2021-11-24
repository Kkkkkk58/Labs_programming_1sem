#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef union ID3v2_header {
    char string_version[10];
    struct Inner_version{
        char tag[3];
        char version;
        char subversion;
        char flags;
        char size_arr[4];
    } fields;
} ID3v2_header;

void show_tags(FILE *song, size_t header_size, char version);
void get_tags(FILE *song, char *tag, size_t header_size, char version);
void set_tags(FILE *song, char *tag, char *new_value, size_t header_size, ID3v2_header *header);

char *get_argument(char *argument);


int main(int argc, char *argv[]) {
    char *filepath = (char *)malloc(128 * sizeof(char));
    filepath = get_argument(argv[1]);

    FILE *song = fopen(filepath, "rb");
    ID3v2_header header;
    fread(&header, sizeof(char), 10, song);
    size_t header_size = 0;
    long multiplier = 128 * 128 * 128;
    for (int i = 0; i < 4; ++i) {
        header_size += (size_t) header.fields.size_arr[i] * multiplier;
        multiplier /= 128;
    }
    set_tags(song, "TIT2", "HUITA", header_size, &header);
    if (strcmp(argv[2], "--show") == 0) {
        show_tags(song, header_size, header.fields.version);
    }
    else if (argv[2][2] == 's') {
        char *frame_name = (char *)malloc(5 * sizeof(char));
        frame_name = get_argument(argv[2]);
        // set_tags();
        free(frame_name);
    }
    else {
        char *frame_name = (char *)malloc(5 * sizeof(char));
        frame_name = get_argument(argv[2]);
        get_tags(song, frame_name, header_size, header.fields.version);
        free(frame_name);
    }
    free(filepath);
    fclose(song);
}


char *get_argument(char *argument) {
    char *name_position = strchr(argument, '=');
    return (name_position + 1);
}


void show_tags(FILE *song, size_t header_size, char version) {
    fseek(song, 10, SEEK_SET);
    short degree = 128;
    if (version - '0' == 3) {
        degree = 256;
    }
    int i = 1;
    while (i < header_size) {
        i += 4;
        char frame_name[5];   
        fread(frame_name, sizeof(char), 4, song);
        if (frame_name[0] == 0) {
            return;
        }
        frame_name[4] = '\0';
        printf("%s ", frame_name);
        i += 4;
        char size_arr[4];
        fread(size_arr, sizeof(char), 4, song);
        long multiplier = degree * degree * degree;
        size_t frame_size = 0;
        for (int i = 0; i < 4; ++i) {
            frame_size += (size_t) size_arr[i] * multiplier;
            multiplier /= degree;
        }
        
        char flags[2];
        fread(flags, sizeof(char), 2, song);
        i += 2;
        char frame_value[frame_size];
        fread(frame_value, sizeof(char), frame_size, song);
        for (int j = 0; j < frame_size; ++j) {
            printf("%c", frame_value[j]);
        }
        printf("\n");
        i += frame_size;
    }
}


void get_tags(FILE *song, char *tag, size_t header_size, char version) {
    fseek(song, 10, SEEK_SET);
    int i = 1;
    int found = 0;
    short degree = 128;
    if (version - '0' == 3) {
        degree = 256;
    }
    while (i < header_size) {
        i += 4;
        char frame_name[5];   
        fread(frame_name, sizeof(char), 4, song);
        if (frame_name[0] == 0) {
            return;
        }
        frame_name[4] = '\0';
        if (strcmp(tag, frame_name) == 0) {
            found = 1;
            printf("%s ", frame_name);
        }
        i += 4;
        char size_arr[4];
        fread(size_arr, sizeof(char), 4, song);

        long multiplier = degree * degree * degree;
        size_t frame_size = 0;
        for (int i = 0; i < 4; ++i) {
            frame_size += (size_t) size_arr[i] * multiplier;
            multiplier /= degree;
        }
        char flags[2];
        fread(flags, sizeof(char), 2, song);
        i += 2;
        char frame_value[frame_size];
        fread(frame_value, sizeof(char), frame_size, song);
        if (found) {
            for (int j = 0; j < frame_size; ++j) {
                printf("%c", frame_value[j]);
            }
            printf("\n");
            return;
        }
        i += frame_size;
    } 
}


void set_tags(FILE *song, char *tag, char *new_value, size_t header_size, ID3v2_header *header) {
    int found = 0;
    int i = 0;
    short degree = 128;
    int curr_size = strlen(new_value);
    int prev_size = 0;
    FILE *new_file = fopen("temp.mp3", "wb");
    if (header->fields.version - '0' == 3) {
        degree = 256;
    }
    char *buffer = (char *)calloc((header_size + curr_size), sizeof(char));
    while (i < header_size - 1) {
        char frame_name[5];   
        fread(frame_name, sizeof(char), 4, song);
        if (frame_name[0] == 0) {
            break;
        }
        frame_name[4] = '\0';
        if (strcmp(tag, frame_name) == 0) {
            found = 1;
        }
        memmove(&buffer[i], frame_name, 4);
        i += 4;
        char size_arr[4];
        fread(size_arr, sizeof(char), 4, song);
        long multiplier = degree * degree * degree;
        size_t frame_size = 0;
        for (int i = 0; i < 4; ++i) {
            frame_size += (size_t) size_arr[i] * multiplier;
            multiplier /= degree;
        }
        if (found == 1) {
            prev_size = frame_size;
            long long divider = 128;
            if (header->fields.version - '0' == 3) {
                divider = 256;
            }
            int tmp = curr_size;
            for (int j = 0; j < 4; ++j) {
                size_arr[3 - j] = (char) tmp % divider;
                tmp /= divider;
            }        
        }
        memmove(&buffer[i], size_arr, 4);
        i += 4;
        char flags[2];
        fread(flags, sizeof(char), 2, song);
        memmove(&buffer[i], flags, 2);
        i += 2;
        if (found == 1) {
            frame_size = curr_size;
            memmove(&buffer[i], new_value, frame_size);
            found = 2;
            fseek(song, prev_size, SEEK_CUR);
        }
        else {
            char frame_value[frame_size];
            fread(frame_value, sizeof(char), frame_size, song);
            memmove(&buffer[i], frame_value, frame_size);
        }
        i += frame_size;
    } 
    header_size = header_size - prev_size + curr_size;
    long divider = 128;
    if (header->fields.version - '0' == 3) {
        divider = 256;
    }
    size_t tmp = header_size;
    for (i = 0; i < 4; ++i) {
        header->fields.size_arr[3 - i] = (char) tmp % divider;
        tmp /= divider;
    }
    for (i = 0; i < 4; ++i) {
        printf("%d\n", header->fields.size_arr[3 - i]);
    }
    for (i = 0; i < 10; ++i) {
        fputc(header->string_version[i], new_file);
    }
    printf("%d\n", header_size);
    for (i = 0; i < header_size; ++i) {
        fputc(buffer[i], new_file);
        printf("%c", buffer[i]);
    }
    fflush(new_file);
    free(buffer);
}