#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <io.h>


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

int main(int argc, char *argv[]) {
    char *filepath = get_argument(argv[1]);
    FILE *song = fopen(filepath, "rb");
    ID3v2_header header;
    fread(&header, sizeof(char), 10, song);
    if (header.fields.flags != 0) {
        ;
    }
    long header_size = 0;
    long multiplier = 128 * 128 * 128;
    for (int i = 0; i < 4; ++i) {
        header_size += header.fields.size_arr[i] * multiplier;
        multiplier /= 128;
    }
    if (strcmp(argv[2], "--show") == 0) {
        show_tags(song, header_size, header.fields.version);
    }
    else if (argv[2][2] == 's') {
        char *frame_name = get_argument(argv[2]);
        char *new_value = get_argument(argv[3]);
        set_tags(song, frame_name, new_value, header_size, &header);
        fclose(song);
        remove(filepath);
        rename("temp.mp3", filepath);
    }
    else {
        char *frame_name = get_argument(argv[2]);
        get_tags(song, frame_name, header_size, header.fields.version);
    }
    fclose(song);
    return 0;
}


char *get_argument(char *argument) {
    char *name_position = strchr(argument, '=');
    return (name_position + 1);
}


void show_tags(FILE *song, long header_size, char version) {
    fseek(song, 10, SEEK_SET);
    int degree = 128;
    if (version == 3) {
        degree = 256;
    }
    int i = 1;
    while (i < header_size) {
        char frame_name[5];   
        fread(frame_name, sizeof(char), 4, song);
        if (frame_name[0] == 0) {
            return;
        }
        frame_name[4] = '\0';
        printf("%s ", frame_name);
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
        char *frame_value =(char *)malloc(frame_size * sizeof(char));
        fread(frame_value, sizeof(char), frame_size, song);
        if (strcmp("APIC", frame_name) == 0) {
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
                while (frame_value[j] != (char) 0xff && frame_value[j + 1] != (char) 0xd8) {
                    ++j;
                }
            }
            else {
                while (frame_value[j] != (char) 0x89 && frame_value[j + 1] != (char) 0x50 
                && frame_value[j + 2] != (char) 0x4e && frame_value[j + 3] != (char) 0x47) {
                    ++j;
                }
            }
            FILE *image = fopen(buff_name, "wb");
            printf("Your picture was saved in %s file", buff_name);
            while (j < frame_size) {
                fputc(frame_value[j], image);
                ++j;
            }
            fflush(image);
            fclose(image);
        }
        else {
            int pos = 0;
            while(frame_value[pos] == '\0') {
                ++pos;
            }
            if (frame_value[pos] == (char)0x1 && frame_value[pos + 1] == (char)0xff && frame_value[pos + 2] == (char)0xfe) {
                pos += 3;
            }
            while (pos < frame_size) {
                printf("%c", frame_value[pos]);
                ++pos;
            }
        }
        free(frame_value);
        frame_value = NULL;
        printf("\n");
        i += frame_size;
    }
}


void get_tags(FILE *song, char *tag, long header_size, char version) {
    fseek(song, 10, SEEK_SET);
    int i = 1;
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
    int curr_size = strlen(new_value) + 1;
    int prev_size = 0;
    FILE *new_file = fopen("temp.mp3", "wb");
    int old_header_size = header_size;
    if (header->fields.version == 3) {
        degree = 256;
    }
    char *buffer = (char *)malloc((header_size + curr_size + 10) * sizeof(char));
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
        char flags[2];
        fread(flags, sizeof(char), 2, song);
        memmove(&buffer[i], flags, 2);
        i += 2;
        if (found == 1) {
            frame_size = curr_size - 1;
            memmove(&buffer[i], "\0", 1);
            i += 1;
            memmove(&buffer[i], new_value, frame_size);
            found = 2;
            fseek(song, prev_size, SEEK_CUR);
        }
        else {
            char *frame_value = (char *)malloc(frame_size * sizeof(char));
            fread(frame_value, sizeof(char), frame_size, song);
            memmove(&buffer[i], frame_value, frame_size);
            free(frame_value);
            frame_value = NULL;
        }
        i += frame_size;
        
    } 
    if (found == 0) {
        memmove(&buffer[i], tag, 4);
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
        memmove(&buffer[i], "\0", 1);
        i += 1;
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


// void info(char frame_name, char *frame_value, long frame_size) {
//     int pos = 0;
//     int encode = 0;
//     while (frame_value[pos] == '\0') {
//         ++pos;
//     }
//     wchar_t *new_frame_value;
//     MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, frame_value, frame_size, new_frame_value, 0);
//     if (frame_value[pos] == (char)0x1 && frame_value[pos + 1] == (char)0xff && frame_value[pos + 2] == (char)0xfe) {
//         pos += 3;
//         encode = 1;
//     }    
    
//     switch (frame_name) {
//         case 'T':
//             while (pos < frame_size) {
//                 if (frame_value[pos] == '\0') {
//                     printf("\n\t");
//                 }
//                 else if (!encode){
//                     printf("%c", frame_value[pos]);
//                 }
//                 else {
//                     wprintf(new_frame_value);
//                 }
//                 ++pos;
//             }
//             break;
//         case 'A':
//             printf("image\n");
//             break;
//         default:
//             while (pos < frame_size) {
//                 if (frame_value[pos] == '\0') {
//                     printf("\n\t");
//                 }
//                 else {
//                     printf("%c", frame_value[pos]);
//                 }
//                 ++pos;
//             }
//             break;
//     }
// }