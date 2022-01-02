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
    int symbol;
    struct Node *root;
} Huffman_tree;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct Code {
    unsigned char code_bytes[2];
    char length;
} Code;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct Node {
    int symbol;
    struct Node *left, *right;
    long long frequency;
    Code code;
} Node;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct Priority_queue {
    Node *array[256];
    int head, tail, size;
} Priority_queue;
#pragma pack(pop)

#pragma pack(push, 1)
typedef union Huffman_table {
    char string_version[4];
    struct Fields {
        unsigned char symbol;
        Code code;
    } fields;
} Huffman_table;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct Multimap {
    int size;
    Huffman_table *codes;
} Multimap;
#pragma pack(pop)


static Huffman_tree hash_table[256] = {0};
void extract(char *archive_name);
void list(char *archive_name);
void create(char* archive_name, int files_number, char *files_names[]);
unsigned char *count_size(unsigned int size);
unsigned int get_size(unsigned char *size_arr);
Node *count_entries(int files_number, char *files_names[]);
void quick_sort(Huffman_tree *array, int start, int end);
void tree_walk(Node *tree, Huffman_table *table);
void make_codes(Node *tree);
void quick_sort_tables(Huffman_table *table, int start, int end);
Huffman_table *binsearch(Huffman_table *array, int start, int end, unsigned char *to_seek);

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


void create(char *archive_name, int files_number, char *files_names[]) {
    FILE *archive = fopen(archive_name, "wb");
    if (archive == NULL) {
        printf("Unable to create the archive file! Try again!\n");
    }
    Node *tree = count_entries(files_number, files_names);
    Huffman_table new_hash_table[256];
    for (int i = 0; i < 256; ++i) {
        new_hash_table[i].fields.code.length = -1;
    }
    tree_walk(tree, new_hash_table);
    int sign_cnt = 0;
    char *header_buf = (char *)calloc(4 * 256, sizeof(char));
    int pos = 0;
    for (int i = 0; i < 256; ++i) {
        if (new_hash_table[i].fields.code.length != -1) {
            // fwrite(new_hash_table[i].string_version, 4, 1, archive);
            memmove(&header_buf[pos], new_hash_table[i].string_version, 4);
            pos += 4;
            ++sign_cnt;
        }
    }
    char size_header[2] = {0};
    if (sign_cnt == 256) {
        size_header[1] = 1;
    }
    else {
        size_header[0] = sign_cnt;
    }
    fwrite(size_header, sizeof(char), 2, archive);
    fwrite(header_buf, sizeof(char), 4 * sign_cnt, archive);
    free(header_buf);
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
        // fseek(archive_content, 0, SEEK_END);
        // unsigned int size = ftell(archive_content);
        // fseek(archive_content, 0, SEEK_SET);
        FILE *tmp = fopen("tmp", "wb");
        int symbol;
        char curr_byte = 0, byte_length = 0;
        char tail = 0;
        while ((symbol = fgetc(archive_content)) != EOF) {
            Code encoded = new_hash_table[symbol].fields.code;
            int mult = 128;
            // printf("%c %d %d\n", symbol, encoded.code_bytes[0], encoded.length);
            for (int i = 0; i < encoded.length; ++i) {
                // printf("%d\n", curr_byte);
                if (i < 8) {
                    // printf("%d\n", encoded.code_bytes[0] & mult);
                    if ((encoded.code_bytes[0] & mult) != 0) {
                        curr_byte |= (1 << (7 - byte_length));
                    }
                    else {
                        curr_byte &= ~(1 << (7 - byte_length));
                    }
                    mult /= 2;
                    ++byte_length;
                    if (byte_length == 8) {
                        fputc(curr_byte, tmp);
                        curr_byte = byte_length = 0;
                    }
                }
                else {
                    mult = (i == 8) ? 128 : 128 / (2 * (i - 8));
                    if ((encoded.code_bytes[1] & mult) != 0) {
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
        }
        if (byte_length != 8) {
            tail = 8 - byte_length;
            fputc(curr_byte, tmp);
        }
        unsigned int original_size = ftell(archive_content);
        fclose(archive_content);
        unsigned char *original_size_array = count_size(original_size);
        fwrite(original_size_array, 4, 1, archive);
        free(original_size_array);
        unsigned int compressed_size = ftell(tmp);
        fclose(tmp);
        FILE *tmp_r = fopen("tmp", "rb");
        unsigned char *compressed_size_array = count_size(compressed_size);
        fwrite(compressed_size_array, 4, 1, archive);
        free(compressed_size_array);
        fputc(tail, archive);
        fseek(tmp, 0, SEEK_SET);
        char *file_buf = (char *)malloc(compressed_size * sizeof(char));
        fread(file_buf, sizeof(char), compressed_size, tmp_r);
        fwrite(file_buf, compressed_size, 1, archive);
        free(file_buf);
        fclose(tmp_r);
        remove("tmp");
        // unsigned char *size_array = count_size(size);
        // fwrite(size_array, 4, 1, archive);
        // free(size_array);
        // fseek(archive_content, 0, SEEK_SET);
        // char *file_buf = (char *)malloc(size * sizeof(char));
        // fread(file_buf, sizeof(char), size, archive_content);
        // fwrite(file_buf, size, 1, archive);
        // free(file_buf);

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
    unsigned char header_size[2];
    fread(header_size, sizeof(unsigned char), 2, archive);
    int offset = header_size[0];
    if (header_size[1] == 1) {
        offset = 256;
    }
    Multimap multimap[16];
    for (int i = 0; i < 16; ++i) {
        multimap[i].size = 0;
        multimap[i].codes = NULL;
    }
    for (int i = 0; i < offset; ++i) {
        Huffman_table table;
        fread(&(table.string_version), sizeof(char), 4, archive);
        int index = table.fields.code.length - 1;
        ++(multimap[index].size);
        multimap[index].codes = (Huffman_table *)realloc(multimap[index].codes, multimap[index].size * sizeof(multimap[index]));
        multimap[index].codes[multimap[index].size - 1] = table;
    }
    for (int i = 0; i < 16; ++i) {
        if (multimap[i].size > 1) {
            quick_sort_tables(multimap[i].codes, 0, multimap[i].size - 1);
        }
    }
    for (int i = 0; i < 16; ++i) {
        if (multimap[i].size != 0) {
            printf("LEN %d\n", i + 1);
            for (int j = 0; j < multimap[i].size; ++j) {
                printf("%c %d %d\n", multimap[i].codes[j].fields.symbol, multimap[i].codes[j].fields.code.code_bytes[0], multimap[i].codes[j].fields.code.code_bytes[1]);
            }
        }
    }
    // for (int i = 0; i < 16; ++i) {
    //     if (multimap[i].size != 0) {
    //         printf("LEN %d, %d ELEM\n", i, multimap[i].size);
    //         for (int j = 0; j < multimap[i].size; ++j) {
    //             printf("%c %d %d\n", multimap[i].codes[j].fields.symbol, multimap[i].codes[j].fields.code.code_bytes[0], multimap[i].codes[j].fields.code.code_bytes[1]);
    //         }
    //     }
    // }
    // exit(0);
    int symbol = 0;
    int prev_position = ftell(archive);
    int i = 0;
    char *names[] = {"1.jpg", "2.jpg", "3.jpg", "4.jpg"};
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
        FILE *extracted_file = fopen("tmp.txt", "wb");
        ++i;
        unsigned char original_size_arr[4];
        fread(original_size_arr, sizeof(unsigned char), 4, archive);
        unsigned int original_size = get_size(original_size_arr);
        unsigned char compressed_size_arr[4];
        fread(compressed_size_arr, sizeof(unsigned char), 4, archive);
        unsigned int compressed_size = get_size(compressed_size_arr);
        char tail;
        fread(&tail, sizeof(char), 1, archive);
        int pos = 0;
        unsigned char curr_byte[2] = {0};
        char byte_length = 0;
        while (pos < compressed_size - 1) {
            // printf("ASSAS\n");
            unsigned char to_decode = fgetc(archive);
            ++pos;
            int mult = 128;
            for (int i = 0; i < 8; ++i) {
                
                
                if (byte_length < 8) {
                    if ((to_decode & mult) != 0) {
                        curr_byte[0] |= (1 << (7 - byte_length));
                    }
                    else {
                        curr_byte[0] &= ~(1 << (7 - byte_length));
                    }
                }
                else {
                    if ((to_decode & mult) != 0) {
                        curr_byte[1] |= (1 << (15 - byte_length));
                    }
                    else {
                        curr_byte[1] &= ~(1 << (15 - byte_length));
                    }
                }
                mult /= 2;
                ++byte_length;
                if (multimap[byte_length - 1].size != 0) {
                    // printf("%d %d %d\n", byte_length, curr_byte[0], curr_byte[1]);
                    Huffman_table *temp = binsearch(multimap[byte_length - 1].codes, 0, multimap[byte_length - 1].size - 1, curr_byte);
                    if (temp != NULL) {
                        // printf("KKKK\n");
                        fputc(temp->fields.symbol, extracted_file);
                        curr_byte[0] = curr_byte[1] = 0;
                        byte_length = 0;
                    }
                }
                
            }
        }
        int mult = 128;
        unsigned char last_byte = fgetc(archive);
        for (int i = 0; i < (8 - tail); ++i) {
            // printf("WHYYYYYY\n");
            if (byte_length < 8) {
                if ((last_byte & mult) != 0) {
                    curr_byte[0] |= (1 << (7 - byte_length));
                }
                else {
                    curr_byte[0] &= ~(1 << (7 - byte_length));
                }
            }
            else {
                if ((last_byte & mult) != 0) {
                    curr_byte[1] |= (1 << (15 - byte_length));
                }
                else {
                    curr_byte[1] &= ~(1 << (15 - byte_length));
                }
            }
            mult /= 2;
            ++byte_length;
            if (multimap[byte_length - 1].size != 0) {
                // printf("%d %d %d\n", byte_length, curr_byte[0], curr_byte[1]);
                Huffman_table *temp = binsearch(multimap[byte_length - 1].codes, 0, multimap[byte_length - 1].size - 1, curr_byte);
                if (temp != NULL) {
                    fputc(temp->fields.symbol, extracted_file);
                    curr_byte[0] = curr_byte[1] = 0;
                    byte_length = 0;
                }
            }
        }
        
        // unsigned char size_arr[4];
        // fread(size_arr, sizeof(unsigned char), 4, archive);
        // unsigned int data_size = get_size(size_arr);
        // char tail;
        // fread(&tail, sizeof(char), 1, archive);
        // char *file_buf = (char *)malloc(data_size * sizeof(char));
        // fread(file_buf, sizeof(char), data_size, archive);

        // fwrite(file_buf, sizeof(char), data_size, extracted_file); 
        fclose(extracted_file);
        prev_position = ftell(archive);
    }
    printf("All files were extracted from \"%s\" archive", archive_name); //AND SAVED TO <DIRNAME> DIRECTORY
    fclose(archive);
}


void list(char *archive_name) {
    FILE *archive = fopen(archive_name, "rb");
    if (archive == NULL) {
        printf("Unable to open provided archive! Try again!\n");
        exit(EXIT_FAILURE);
    }
    printf("The archive \"%s\" contains following files:\n", archive_name);
    unsigned char header_size[2];
    fread(header_size, sizeof(unsigned char), 2, archive);
    int offset = header_size[0];
    if (header_size[1] == 1) {
        offset = 256;
    }
    fseek(archive, offset * 4, SEEK_CUR);
    int symbol = 0;
    while (symbol != EOF) {
        while ((symbol = fgetc(archive)) != '\0' && symbol != EOF) {
            printf("%c", symbol);
        }
        if (symbol == EOF) {
            return;
        }
        printf("\n");
        fseek(archive, 4, SEEK_CUR);
        unsigned char size_arr[4];
        fread(size_arr, sizeof(unsigned char), 4, archive);
        unsigned int data_size = get_size(size_arr);
        fseek(archive, data_size + 1, SEEK_CUR);
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


Node *count_entries(int files_number, char *files_names[]) {
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
    quick_sort(hash_table, 0, 255);
    int count = 256;
    for (int i = 0; i < 256; ++i) {
        if (hash_table[i].number_of_entries == 0) {
            --count;
        }
        else {
            hash_table[i].root = (Node *)malloc(sizeof(Node));
            hash_table[i].root->symbol = hash_table[i].symbol;
            hash_table[i].root->frequency = hash_table[i].number_of_entries;
            hash_table[i].root->left = hash_table[i].root->right = NULL;
            hash_table[i].root->code.code_bytes[0] = hash_table[i].root->code.code_bytes[1] = 0;
            hash_table[i].root->code.length = 0;
        }
    }
    int start = 256 - count;
    Priority_queue queue;
    for (int i = 0; i < count; ++i) {
        queue.array[i] = hash_table[start + i].root;
    }
    queue.head = 0;
    queue.tail = count;
    queue.size = count;
    while (queue.size >= 2) {
        Node *least_1 = queue.array[queue.head];
        Node *least_2 = queue.array[queue.head + 1];
        ++(queue.head);
        Node *new_node = (Node *)malloc(sizeof(Node));
        new_node->left = least_1;
        new_node->right = least_2;
        new_node->frequency = least_1->frequency + least_2->frequency;
        new_node->symbol = -1;
        new_node->code.code_bytes[0] = new_node->code.code_bytes[1] = 0;
        new_node->code.length = 0;
        --(queue.size);
        queue.array[queue.head] = new_node;
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
    make_codes(queue.array[queue.head]);
    // tree_walk(queue.array[queue.head]);
    // printf("DONE\n");
    // printf("ROOT %c %d\n", queue.array[queue.head]->right->right->right->right->right->right->right->right->symbol, queue.array[queue.head]->right->right->right->right->right->right->right->right->frequency);
    return queue.array[queue.head];
 }


void make_codes(Node *tree) {
    if (tree->left != NULL) {
        tree->left->code.length = tree->code.length + 1;
        strcpy(tree->left->code.code_bytes, tree->code.code_bytes);
        if (tree->code.length <= 7) {
            tree->left->code.code_bytes[0] &= ~(1 << (7 - tree->code.length));
        }
        else {
            tree->left->code.code_bytes[1] &= ~(1 << (7 - (tree->code.length - 8)));
        }
        // strcpy(tree->left->code, tree->code);
        // strcat(tree->left->code, "0");
        make_codes(tree->left);
    }
    if (tree->right != NULL) {
        tree->right->code.length = tree->code.length + 1;
        strcpy(tree->right->code.code_bytes, tree->code.code_bytes);
        if (tree->code.length <= 7) {
            tree->right->code.code_bytes[0] |= (1 << (7 - tree->code.length));
        }
        else {
            tree->right->code.code_bytes[1] |= (1 << (7 - (tree->code.length - 8)));
        }
        // strcpy(tree->right->code, tree->code);
        // strcat(tree->right->code, "1");
        make_codes(tree->right);
    }
}


void tree_walk(Node *tree, Huffman_table *table) {
    if (tree != NULL) {
        if (tree->symbol != -1) {
            table[tree->symbol].fields.symbol = tree->symbol;
            table[tree->symbol].fields.code = tree->code;
            // printf("FREQ %d SYM %c CODE %d %d %d\n", tree->frequency, tree->symbol, tree->code.code_bytes[0], tree->code.code_bytes[1], tree->code.length);
        }
        if (tree->left != NULL) {
            tree_walk(tree->left, table);
        }
        if (tree->right != NULL) {
            tree_walk(tree->right, table);
        }
    }
}


void quick_sort(Huffman_tree *array, int start, int end) {
    Huffman_tree pivot = array[(start + end) / 2];
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
            SWAP(array[i], array[j]);
            ++i;
            --j;
        }
    }
    if (start < j) {
        quick_sort(array, start, j);
    }
    if (end > i) {
        quick_sort(array, i, end);
    }
}



void quick_sort_tables(Huffman_table *array, int start, int end) {
    Huffman_table pivot = array[(start + end) / 2];
    int i = start;
    int j = end;
    while (i <= j) {
        while (array[i].fields.code.code_bytes[0] < pivot.fields.code.code_bytes[0] ||
        array[i].fields.code.code_bytes[0] == pivot.fields.code.code_bytes[0] &&
        array[i].fields.code.code_bytes[1] < pivot.fields.code.code_bytes[1]) {
            ++i;
        }
        while (array[j].fields.code.code_bytes[0] > pivot.fields.code.code_bytes[0] ||
        array[j].fields.code.code_bytes[0] == pivot.fields.code.code_bytes[0] &&
        array[j].fields.code.code_bytes[1] > pivot.fields.code.code_bytes[1]) {
            --j;
        }
        if (i <= j) {
            Huffman_table tmp = array[i];
            array[i] = array[j];
            array[j] = tmp;
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


Huffman_table *binsearch(Huffman_table *array, int start, int end, unsigned char *to_seek) {
    int mid = (start + end) / 2;
    if (start >= end && (array[mid].fields.code.code_bytes[0] != to_seek[0] || array[mid].fields.code.code_bytes[1] != to_seek[1])) {
        // printf("SWAT");
        return NULL;
    }
    if (array[mid].fields.code.code_bytes[0] == to_seek[0] && array[mid].fields.code.code_bytes[1] == to_seek[1]) {
        return &(array[mid]);
    }
    if (array[mid].fields.code.code_bytes[0] < to_seek[0] || array[mid].fields.code.code_bytes[0] == to_seek[0] && array[mid].fields.code.code_bytes[1] < to_seek[1]) {
        // printf("%d %d\n", array[mid].fields.code.code_bytes[0], array[mid].fields.code.code_bytes[1]);
        return binsearch(array, mid + 1, end, to_seek);
    }
    if (array[mid].fields.code.code_bytes[0] > to_seek[0] || array[mid].fields.code.code_bytes[0] == to_seek[0] && array[mid].fields.code.code_bytes[1] > to_seek[1]) {
        // printf("%d %d\n", array[mid].fields.code.code_bytes[0], array[mid].fields.code.code_bytes[1]);
        return binsearch(array, start, mid - 1, to_seek);
    }
    return NULL;
}