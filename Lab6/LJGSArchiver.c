#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SIZE_MULTIPLIER 256
#define SWAP(x, y)        \
    Huffman_tree tmp = x; \
    x = y;                \
    y = tmp;

#pragma pack(push, 1)
typedef struct Huffman_tree {
    long long number_of_entries;
    unsigned char symbol;
} Huffman_tree;
#pragma pack(pop)


static Huffman_tree hash_table[256] = {0};
void extract(char *archive_name);
void list(char *archive_name);
void create(char* archive_name, int files_number, char *files_names[]);
unsigned char *count_size(unsigned int size);
unsigned int get_size(unsigned char *size_arr);
void count_entries(int files_number, char *files_names[]);
// void quick_sort(Huffman_tree *array, int start, int end);


int main(int argc, char *argv[]) {
    char *archive_name = NULL;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--file") == 0) {
            archive_name = argv[i + 1];
            if (archive_name == NULL) {
                printf("Unsupported input format! Try again and specify the name of the file!\n");
                exit(EXIT_FAILURE);
            }
            ++i;
        }
        else if (strcmp(argv[i], "--extract") == 0) {
            extract(archive_name);
        }
        else if (strcmp(argv[i], "--list") == 0) {
            list(archive_name);
        }
        else if (strcmp(argv[i], "--create") == 0) {
            create(archive_name, argc - 4, &argv[4]);
            break;
        }
        else {
            printf("Unknown argument! Try again!\n");
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}


void create(char *archive_name, int files_number, char *files_names[]) {
    FILE *archive = fopen(archive_name, "wb");
    if (archive == NULL) {
        printf("Unable to create the archive file! Try again!\n");
    }
    count_entries(files_number, files_names);
    // for (int i = 0; i < 256; ++i) {
    //     printf("%d %c %d\n", i, hash_table[i].symbol, hash_table[i].number_of_entries);
    // }
    for (int i = 0; i < files_number; ++i) {
        FILE *archive_content = fopen(files_names[i], "rb");
        if (archive_content == NULL) {
            printf("Unable to open the file named \"%s\". Skipping...\n", files_names[i]);
            continue;
        }
        printf("Adding \"%s\" to the archive...", files_names[i]);
        fwrite(files_names[i], strlen(files_names[i]) + 1, 1, archive);
        fseek(archive_content, 0, SEEK_END);
        unsigned int size = ftell(archive_content);
        unsigned char *size_array = count_size(size);
        fwrite(size_array, 4, 1, archive);
        free(size_array);
        fseek(archive_content, 0, SEEK_SET);
        char *file_buf = (char *)malloc(size * sizeof(char));
        fread(file_buf, sizeof(char), size, archive_content);
        fwrite(file_buf, size, 1, archive);
        free(file_buf);
        fclose(archive_content);
        printf(" Done!\n");
    }
    printf("All valid given files were added to the archive named \"%s\"", archive_name);
    fclose(archive);
}


void extract(char *archive_name) {
    FILE *archive = fopen(archive_name, "rb");
    if (archive == NULL) {
        printf("Unable to open provided archive! Try again!\n");
        exit(EXIT_FAILURE);
    }
    int symbol = 0;
    int prev_position = 0;
    while (symbol != EOF) {
        while ((symbol = fgetc(archive)) != '\0' && symbol != EOF) {
            ;
        }
        if (symbol == EOF) {
            break;
        }
        int new_position = ftell(archive);
        int name_size = new_position - prev_position;
        char *file_name = (char *)malloc(name_size * sizeof(char));
        fseek(archive, prev_position, SEEK_SET);
        fread(file_name, sizeof(char), name_size, archive);
        printf("Extracting \"%s\"...\n", file_name);
        FILE *extracted_file = fopen(file_name, "wb");
        unsigned char size_arr[4];
        fread(size_arr, sizeof(unsigned char), 4, archive);
        unsigned int data_size = get_size(size_arr);
        char *file_buf = (char *)malloc(data_size * sizeof(char));
        fread(file_buf, sizeof(char), data_size, archive);
        fwrite(file_buf, sizeof(char), data_size, extracted_file); 
        fclose(extracted_file);
        prev_position = ftell(archive);
    }
    printf("All files were extracted from \"%s\" archive", archive_name);
    fclose(archive);
}


void list(char *archive_name) {
    FILE *archive = fopen(archive_name, "rb");
    if (archive == NULL) {
        printf("Unable to open provided archive! Try again!\n");
        exit(EXIT_FAILURE);
    }
    printf("The archive \"%s\" contains following files:\n", archive_name);
    int symbol = 0;
    while (symbol != EOF) {
        while ((symbol = fgetc(archive)) != '\0' && symbol != EOF) {
            printf("%c", symbol);
        }
        if (symbol == EOF) {
            return;
        }
        printf("\n");
        unsigned char size_arr[4];
        fread(size_arr, sizeof(unsigned char), 4, archive);
        unsigned int data_size = get_size(size_arr);
        fseek(archive, data_size, SEEK_CUR);
    }
    fclose(archive);

}


unsigned char* count_size(unsigned int size) {
    unsigned char *size_array = (unsigned char *)calloc(4, sizeof(unsigned char));
    for (int i = 0; i < 4; ++i) {
        size_array[i] = size % SIZE_MULTIPLIER;
        size /= SIZE_MULTIPLIER;
    }
    return size_array;
}


unsigned int get_size(unsigned char *size_arr) {
    int multiplier_value = 1;
    unsigned int size = 0;
    for (int i = 0; i < 4; ++i) {
        size += multiplier_value * size_arr[i];
        multiplier_value *= SIZE_MULTIPLIER;
    }
    return size;
}


void count_entries(int files_number, char *files_names[]) {
    for (int i = 0; i < 256; ++i) {
        hash_table[i].symbol = i;
    }
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
    // quick_sort(hash_table, 0, 255);
}


// void quick_sort(Huffman_tree *array, int start, int end) {
//     Huffman_tree pivot = array[(start + end) / 2];
//     int i = start;
//     int j = end;
//     while (i <= j) {
//         while (array[i].number_of_entries < pivot.number_of_entries) {
//             ++i;
//         }
//         while (array[j].number_of_entries > pivot.number_of_entries) {
//             --j;
//         }
//         if (i <= j) {
//             SWAP(array[i], array[j]);
//             ++i;
//             --j;
//         }
//     }
//     if (start < j) {
//         quick_sort(array, start, j);
//     }
//     if (end > i) {
//         quick_sort(array, i, end);
//     }
// }