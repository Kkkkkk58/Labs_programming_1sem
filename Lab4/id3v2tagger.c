#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <io.h>
#include <windows.h>
#include <mmsystem.h>


typedef union ID3v2_header {
    char string_version[10];
    struct Inner_version{
        char tag[3];
        char version;
        char subversion;
        char flags;
        unsigned char size_arr[4];
    } fields;
} ID3v2_header;


void show_tags(FILE *song, long header_size, char version);
void get_tags(FILE *song, char *tag, long header_size, char version);
void set_tags(FILE *song, char *tag, char *new_value, long header_size, ID3v2_header *header);
char *get_argument(char *argument);
void info(char frame_name, char *frame_value, long frame_size);
void out_image(char *frame_value, long frame_size);
void get_info(char *frame_name, char *frame_value, long frame_size);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Unsupported input format! Try again!");
        return 1;
    }
    char *filepath = get_argument(argv[1]);
    FILE *song = fopen(filepath, "rb");
    if (song == NULL) {
        printf("Some errors while reading the file. Try again!");
        return 1;
    }
    ID3v2_header header;
    fread(&header, sizeof(char), 10, song);
    int ext_header_exists = 0;
    int footer_exists = 0;
    if (header.fields.flags != 0) {
        if (header.fields.flags & (char) 0x80 == (char) 0x80) {
            ext_header_exists = 1;
        }
    }
    long header_size = 0;
    long multiplier = 128 * 128 * 128;
    for (int i = 0; i < 4; ++i) {
        header_size += header.fields.size_arr[i] * multiplier;
        multiplier /= 128;
    }
    header.fields.flags = '\0';
    int start_position = 10;
    if (ext_header_exists) {
        long ext_header_size = 0;
        int degree = 128;
        if (header.fields.version == 3) {
            degree = 256;
            start_position += 4;
        }
        long multiplier = degree * degree * degree;
        unsigned char ext_size_arr[4];
        fread(ext_size_arr, 4, sizeof(char), song);
        for (int i = 0; i < 4; ++i) {
            ext_header_size += ext_size_arr[i] * multiplier;
            multiplier /= degree;
        }
        start_position += 6 + ext_header_size;
    }
    int show_enabled = 0;
    int get_enabled = 0;
    int set_enabled = 0;
    char *frame_to_get;
    char *frame_to_set;
    char *value_to_set;
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
                return 1;
            }
        }
    }
    if (show_enabled) {
        fseek(song, start_position, SEEK_SET);
        show_tags(song, header_size, header.fields.version);
    }
    if (get_enabled) {
        fseek(song, start_position, SEEK_SET);
        get_tags(song, frame_to_get, header_size, header.fields.version);
    }
    if (set_enabled) {
        fseek(song, start_position, SEEK_SET);
        set_tags(song, frame_to_set, value_to_set, header_size, &header);
        fclose(song);
        song = NULL;
        remove(filepath);
        rename("temp.mp3", filepath);
    }
    if (song != NULL) {
        fclose(song);
    }
    return 0;
}


char *get_argument(char *argument) {
    char *name_position = strchr(argument, '=');
    return (name_position + 1);
}


void show_tags(FILE *song, long header_size, char version) {
    int degree = 128;
    if (version == 3) {
        degree = 256;
    }
    int i = 0;
    while (i < header_size) {
        printf("%d %d", i, header_size);
        char frame_name[5];   
        fread(frame_name, sizeof(char), 4, song);
        if (frame_name[0] == 0) {
            return;
        }
        frame_name[4] = '\0';
        printf("%s\n", frame_name);
        /* ЕБАНИ СТРУКТУРУ */
        unsigned char size_arr[4];
        fread(size_arr, sizeof(char), 4, song);
        long multiplier = degree * degree * degree;
        long frame_size = 0;
        for (int i = 0; i < 4; ++i) {
            frame_size += size_arr[i] * multiplier;
            multiplier /= degree;
        }
        char flags[2];
        fread(flags, sizeof(char), 2, song);
        char *frame_value =(char *)malloc(frame_size * sizeof(char));
        fread(frame_value, sizeof(char), frame_size, song);
        // get_info(frame_name, frame_value, frame_size);
        free(frame_value);
        frame_value = NULL;
        i += 10 + frame_size;
    }
}


void get_tags(FILE *song, char *tag, long header_size, char version) {
    int i = 0;
    int found = 0;
    short degree = 128;
    if (version == 3) {
        degree = 256;
    }
    while (i < header_size) {
        char frame_name[5];   
        fread(frame_name, sizeof(char), 4, song);
        if (frame_name[0] == 0) {
            break;
        }
        frame_name[4] = '\0';
        if (strcmp(tag, frame_name) == 0) {
            found = 1;
            printf("%s ", frame_name);
        }
        unsigned char size_arr[4];
        fread(size_arr, sizeof(char), 4, song);

        long multiplier = degree * degree * degree;
        long frame_size = 0;
        for (int i = 0; i < 4; ++i) {
            frame_size += size_arr[i] * multiplier;
            multiplier /= degree;
        }
        char flags[2];
        fread(flags, sizeof(char), 2, song);
        i += 10;
        char *frame_value = (char *)malloc(frame_size * sizeof(char));
        fread(frame_value, sizeof(char), frame_size, song);
        if (found) {
            int j = 0;
            if (frame_size > 3 && frame_value[0] == (char)0x1 
            && frame_value[1] == (char)0xff && frame_value[2] == (char)0xfe) {
                j = 3;
            }
            while (j < frame_size) {
                printf("%c", frame_value[j]);
                ++j;
            }
            printf("\n");
            return;
        }
        free(frame_value);
        frame_value = NULL;
        i += frame_size;
    } 
    printf("Tag %s not found\n", tag);
}


void set_tags(FILE *song, char *tag, char *new_value, long header_size, ID3v2_header *header) {
    int found = 0;
    int i = 0;
    short degree = 128;
    int curr_size = strlen(new_value);
    if (tag[0] == 'T') {
        ++curr_size;
    }
    int prev_size = 0;
    FILE *new_file = fopen("temp.mp3", "wb");
    int old_header_size = header_size;
    if (header->fields.version == 3) {
        degree = 256;
    }
    char *buffer = (char *)malloc((header_size + curr_size + 10) * sizeof(char));
    int iter = 0;
    while (iter < header_size - 1) {
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
        iter += 4;
        unsigned char size_arr[4];
        fread(size_arr, sizeof(char), 4, song);
        long multiplier = degree * degree * degree;
        long frame_size = 0;
        for (int j = 0; j < 4; ++j) {
            frame_size += size_arr[j] * multiplier;
            multiplier /= degree;
        }
        if (found == 1) {
            prev_size = frame_size;
            long divider = 128;
            if (header->fields.version == 3) {
                divider = 256;
            }
            long tmp = curr_size;
            for (int j = 0; j < 4; ++j) {
                size_arr[3 - j] = (char) tmp % divider;
                tmp /= divider;
            }        
        }
        memmove(&buffer[i], size_arr, 4);
        i += 4;
        iter += 4;
        char flags[2];
        fread(flags, sizeof(char), 2, song);
        memmove(&buffer[i], flags, 2);
        i += 2;
        iter += 2;
        if (found == 1) {
            memmove(&buffer[i], "\0", 1);
            i += 1;
            memmove(&buffer[i], new_value, curr_size - 1);
            found = 2;
            fseek(song, prev_size, SEEK_CUR);
            i += curr_size - 1;
        }
        else {
            char *frame_value = (char *)malloc(frame_size * sizeof(char));
            fread(frame_value, sizeof(char), frame_size, song);
            memmove(&buffer[i], frame_value, frame_size);
            free(frame_value);
            frame_value = NULL;
            i += frame_size;
        }
    iter += frame_size;
        
    } 
    if (found == 0) {
        printf("%c\n", buffer[i]);
        memmove(&buffer[i], tag, 4);
        printf("%s\n", tag);
        i += 4;
        long divider = 128;
        if (header->fields.version == 3) {
            divider = 256;
        }
        char new_size_arr[4];
        long tmp = curr_size;
        for (int j = 0; j < 4; ++j) {
            new_size_arr[3 - j] = (char) tmp % divider;
            tmp /= divider;
        }  
        memmove(&buffer[i], new_size_arr, 4);
        i += 4;
        memmove(&buffer[i], "\0\0", 2);
        i += 2;
        // memmove(&buffer[i], "\0", 1);
        // i += 1;
        memmove(&buffer[i], new_value, curr_size);
        i += curr_size;
        curr_size += 10;
        
    }
    header_size = i;
    int divider = 128;
    long tmp = header_size;
    for (i = 0; i < 4; ++i) {
        header->fields.size_arr[3 - i] = (char) tmp % divider;
        tmp /= divider;
    }
    for (i = 0; i < 10; ++i) {
        fputc(header->string_version[i], new_file);
    }
    for (i = 0; i < header_size; ++i) {
        fputc(buffer[i], new_file);
    }
    fflush(new_file);
    free(buffer);
    fseek(song, old_header_size + 10, SEEK_SET);
    while (!feof(song)) {
        char c = fgetc(song);
        fputc(c, new_file);
    }
    char c = fgetc(song);
    fputc(c, new_file);
    fflush(new_file);
    fclose(new_file);
}


void get_info(char *frame_name, char *frame_value, long frame_size) {
    printf("%s ", frame_name);
    int pos = 0;
    switch (frame_name[0]) {
        case 'A':
            if (strcmp(frame_name, "APIC") == 0) {
                out_image(frame_value, frame_size);
                return;
            }
            break;
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
                        if (frame_value[pos] == 'i' && frame_value[pos + 1] == 'm' 
                        && frame_value[pos + 2] == 'a' && frame_value[pos + 3] == 'g' 
                        && frame_value[pos + 4] == 'e' && frame_value[pos + 5] == '\\') {
                            out_image(&frame_value[pos - 1], frame_size - pos);
                            return;
                        }
                    }
                }
                printf("%c", frame_value[pos]);
                ++pos;
            }
            break;
        case 'U':
            if (strcmp(frame_name, "UFID") == 0) {
                printf("%s", frame_value);
                return;
            }
            pos = 1;
            while (pos < frame_size) {
                printf("%c", frame_value[pos]);
                ++pos;
            }
            break;
        case 'W':
            printf("%s", frame_value);
            break;
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


void out_image(char *frame_value, long frame_size) {
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
    while (j < frame_size) {
        fputc(frame_value[j], image);
        ++j;
    }
    fflush(image);
    fclose(image);
}
