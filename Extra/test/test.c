#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>

#include <stdbool.h>

int get_int_length(int num) {
    int length = 0;
    if (num < 0) {
        ++length;
    }
    do {
        num /= 10;
        ++length;
    } while (num != 0);
    return length;
}

void str_array_to_int(char* strs[], int nums[], int n) {
    for (int i = 0; i < n; ++i) {
        nums[i] = atoi(strs[i]);
    }
}

void int_to_string(char* s, int x) { // Преобразует число в строку
    bool is_negative = x < 0;
    if (is_negative) {
        x *= -1;
    }

    bool is_first = true;
    char* p;
    do {
        if (is_first) {
            p = s;
            is_first = false;
        } else {
            ++p;
        }
        *p = '0' + (x % 10);
        x /= 10;
    } while (x != 0);

    if (is_negative) {
        ++p;
        *p = '-';
    }

    printf(">>> %s\n", s);

    while (s < p) {
        printf("\t ch: %c <-> %c\n", *s, *p);
        char tmp = *s;
        *s = *p;
        *p = tmp;
        ++s;
        --p;
    }
}

void read_strings(int fd, char*** ptr_buffer, int* res_size) {
    char c; // Текущий считываемый символ
    int str_idx = 0; // Индекс текущего слова
    size_t buffer_size = 1; // Кол-во строк
    char** buffer = malloc(sizeof(char*) * buffer_size);
    int* lengths = malloc(sizeof(int) * buffer_size);

    lengths[0] = 0;
    int str_size = 1; // Длина текущей строки
    buffer[0] = malloc(sizeof(char) * str_size);
    if (!buffer[0]) {
        perror("Malloc error");
        exit(-2);
    }

    while (read(fd, &c, sizeof(char)) > 0) {
        if (c == '\n') { // Конец ввода слов
            break;
        }
        if (c == ' ') { // Следующее слово
            ++str_idx;
            str_size = 1;
            continue;
        }

        // Увеличиваем буфер строк и длин
        if (str_idx >= buffer_size) {
            //printf("Increasing buffer\n");

            ++buffer_size;
            str_size = 1;
            buffer = realloc(buffer, sizeof(char*) * buffer_size);
            buffer[buffer_size - 1] = malloc(sizeof(char) * str_size);
            for (size_t i = 0; i < str_size; ++i) {
                buffer[buffer_size - 1][i] = 0;
            }

            lengths = realloc(lengths, sizeof(int) * buffer_size);
            lengths[buffer_size - 1] = 0;
        }

        // Увеличиваем длину строки
        if (lengths[str_idx] == str_size) {
            //printf("Increasing str len\n");
            str_size += 16;
            buffer[str_idx] = realloc(buffer[str_idx], sizeof(char) * str_size);
            if (!buffer[str_idx]) {
                perror("Realloc error");
                exit(-2);
            }
        }
        
        buffer[str_idx][lengths[str_idx]] = c;
        ++lengths[str_idx];

        /*printf(">>> Read '%c'\n", c);
        printf("buffer[str_idx] = %s\n", buffer[str_idx]);
        printf("str_idx = %d\n", str_idx);
        printf("buffer_size = %zu\n", buffer_size);
        printf("str_size = %d\n", str_size);
        printf("length[str_idx] = %d\n",lengths[str_idx]);*/
    }

    for (int i = 0; i < buffer_size; ++i) {
        buffer[i] = realloc(buffer[i], sizeof(char) * lengths[i]);
        buffer[i][lengths[i]] = '\0';
    }
    *res_size = buffer_size;
    *ptr_buffer = buffer;

    /*printf("Result from read_strings func:\n");
    for (int i = 0; i < *res_size; ++i) {
        printf("%d) \"%s\"\n", i, buffer[i]);
    }*/
}

void read_string(int fd, char** str, size_t* size) {
    char c; // Текущий считываемый символ
    size_t str_len = 0;
    size_t str_size = 1; // Длина текущей строки
    char* buffer = malloc(sizeof(char) * str_size);

    if (!buffer) {
        perror("Malloc error");
        exit(-2);
    }

    while (read(fd, &c, sizeof(char)) > 0) {
        if (c == '\n') { // Конец ввода слов
            break;
        }

        // Увеличиваем длину строки
        if (str_len == str_size) {
            //printf("Increasing str len\n");
            str_size += 16;
            buffer = realloc(buffer, sizeof(char) * str_size);
            if (!buffer) {
                perror("Realloc error");
                exit(-2);
            }
        }
        
        buffer[str_len] = c;
        ++str_len;
    }

    buffer = realloc(buffer, sizeof(char) * str_len);
    buffer[str_len] = '\0';

    *size = str_len;
    *str = buffer;
}

int main() {
    /*char** nums;;
    int size = 0;
    read_strings(0, &nums, &size);
    printf("size: %d \n", size);
    for (int i = 0; i < size; ++i) {
        printf("%s ", nums[i]);
    }
    printf("\n");*/

    char* str;
    size_t size = 0;
    read_string(0, &str, &size);
    printf("%s\n", str);

    int fp = open("output.txt", O_CREAT | O_WRONLY);
    if (fp == -1) {
        printf("fp = %d", fp);
        exit(1);
    }
    write(fp, str, sizeof(char) * size);

    /*
    for (size_t i = 0; i < 10; i++)
    {
        int x;
        scanf("%d", &x);

        char str[get_int_length(x)];
        int_to_string(str, x);

        printf("string \"%s\" with size = %d\n", str, get_int_length(x));
    }
    */




    return 0;
}
