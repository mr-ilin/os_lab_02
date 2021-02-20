#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>

#define ERR_MALLOC 3
#define ERR_REALLOC 4
#define ERR_READ 5
#define ERR_WRITE 6
#define ERR_OPEN 7
#define ERR_CLOSE 8
#define ERR_PIPE 9
#define ERR_FORK 10
#define ERR_WAIT 11

#define ERR_DIV_0 20
#define ERR_INV_INT 21
#define ERR_NO_STATUS 22


// Межпроцессорное взаимодействие
// Многопоточное программирование
/* 
 * Пользователь вводит команды вида: «число число число<endline>». Далее эти числа передаются от родительского процесса в дочерний. 
 * Дочерний процесс производит деление первого числа, на последующие, а результат выводит в файл. 
 * Если происходит деление на 0, то тогда дочерний и родительский процесс завершают свою работу. 
 * Проверка деления на 0 должна осуществляться на стороне дочернего процесса. Числа имеют тип int.
*/

int get_int_length(int num) { // Считает длину числа
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

    //printf(">>> %s\n", s);

    while (s < p) {
        //printf("\t ch: %c <-> %c\n", *s, *p);
        char tmp = *s;
        *s = *p;
        *p = tmp;
        ++s;
        --p;
    }
}

void read_strings(int fd, char*** ptr_buffer, size_t* res_size) { // Считывает строки через пробел до конца ввода
    char c; // Текущий считываемый символ
    int str_idx = 0; // Индекс текущего слова
    size_t buffer_size = 1; // Кол-во строк
    char** buffer = (char**)malloc(sizeof(char*) * buffer_size);
    if (!buffer) {
        perror("Malloc error");
        exit(ERR_MALLOC);
    }
    int* lengths = (int*)malloc(sizeof(int) * buffer_size);
    if (!lengths) {
        perror("Malloc error");
        exit(ERR_MALLOC);
    }

    lengths[0] = 0;
    int str_size = 1; // Длина текущей строки
    buffer[0] = (char*)malloc(sizeof(char) * str_size);
    if (!buffer[0]) {
        perror("Malloc error");
        exit(ERR_MALLOC);
    }
    size_t read_bytes = 0;
    while ((read_bytes = read(fd, &c, sizeof(char))) > 0) {
        if ((read_bytes != sizeof(char)) && (read_bytes != 0)) {
            perror("Read error");
            exit(ERR_READ);
        }

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

            buffer = (char**)realloc(buffer, sizeof(char*) * buffer_size);
            if (!buffer) {
                perror("Realloc error");
                exit(ERR_REALLOC);
            }

            buffer[buffer_size - 1] = (char*)malloc(sizeof(char) * str_size);
            if (!buffer[buffer_size - 1]) {
                perror("Malloc error");
                exit(ERR_MALLOC);
            }
            for (size_t i = 0; i < str_size; ++i) {
                buffer[buffer_size - 1][i] = 0;
            }

            lengths = (int*)realloc(lengths, sizeof(int) * buffer_size);
            if (!lengths) {
                perror("Realloc error");
                exit(ERR_REALLOC);
            }
            lengths[buffer_size - 1] = 0;
        }

        // Увеличиваем длину строки
        if (lengths[str_idx] == str_size) {
            //printf("Increasing str len\n");
            str_size += 16;
            buffer[str_idx] = (char*)realloc(buffer[str_idx], sizeof(char) * str_size);
            if (!buffer[str_idx]) {
                perror("Realloc error");
                exit(ERR_REALLOC);
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
        buffer[i] = (char*)realloc(buffer[i], sizeof(char) * lengths[i]);
        if (!buffer[i]) {
            perror("Realloc error");
            exit(ERR_REALLOC);
        }
        buffer[i][lengths[i]] = '\0';
    }
    *res_size = buffer_size;
    *ptr_buffer = buffer;

    /*printf("Result from read_strings func:\n");
    for (int i = 0; i < *res_size; ++i) {
        printf("%d) \"%s\"\n", i, buffer[i]);
    }*/
}

void read_string(int fd, char** str, size_t* size) { // Считывает одну строку
    char c; // Текущий считываемый символ
    size_t str_len = 0;
    size_t str_size = 1; // Длина текущей строки
    char* buffer = (char*)malloc(sizeof(char) * str_size);
    if (!buffer) {
        perror("Malloc error");
        exit(ERR_MALLOC);
    }

    size_t read_bytes = 0;
    while ((read_bytes = read(fd, &c, sizeof(char))) > 0) {
        if ((read_bytes != sizeof(char)) && (read_bytes != 0)) {
            perror("Read error");
            exit(ERR_READ);
        }

        if (c == '\n') { // Конец ввода слов
            break;
        }

        // Увеличиваем длину строки
        if (str_len == str_size) {
            //printf("Increasing str len\n");
            str_size += 16;
            buffer = (char*)realloc(buffer, sizeof(char) * str_size);
            if (!buffer) {
                perror("Realloc error");
                exit(ERR_REALLOC);
            }
        }
        
        buffer[str_len] = c;
        ++str_len;
    }

    buffer = (char*)realloc(buffer, sizeof(char) * str_len);
    if (!buffer) {
        perror("Realloc error");
        exit(ERR_REALLOC);
    }
    buffer[str_len] = '\0';

    *size = str_len;
    *str = buffer;
}

void str_array_to_int(char** strs, int nums[], size_t n) { // Преобразует массив строк в массив int'ов
    for (size_t i = 0; i < n; ++i) {
        ///nums[i] = atoi(strs[i]);
        char* p = strs[i];
        bool is_negative = false;
        int result = 0;
        while(*p != '\0') {
            if (*p == '-' || *p == '+') {
                if (p != strs[i]) {
                    perror("Invalid int");
                    exit(ERR_INV_INT);
                }
                if (*p == '-') {
                    is_negative = true;
                }
            } else if ('0' <= *p && *p <= '9') {
                result = result * 10 + (*p - '0');
            } else {
                perror("Invalid int");
                exit(ERR_INV_INT);
            }
            ++p;
        }
        if (is_negative) {
            result *= -1;
        }
        nums[i] = result;
    }
}

void write_to_fd(int fd, const void *buf, size_t nbytes) {
    if (write(fd, buf, nbytes) != nbytes) {
        perror("Write error");
        exit(ERR_WRITE);
    }
}

void read_from_fd(int fd, void *buf, size_t nbytes) {
    if (read(fd, buf, nbytes) != nbytes) {
        perror("Read error");
        exit(ERR_READ);
    }
}

int main() {
    int fd[2]; // массив файловых дескрипторов
    // создаем pipe
    if (pipe(fd) != 0) {
        perror("Piping error");
        exit(ERR_PIPE);
    }
    int status;

    int id = fork();

    if (id == -1) {
        perror("fork() error\n");
        exit(ERR_FORK);
    } else if (id == 0) {
        // Ребенок
        size_t path_size;
        size_t size;
        int divident;
        int divisor = 1;

        // Читаем размер строки и строку
        read_from_fd(fd[0], &path_size, sizeof(size_t));
        char* path = (char*)malloc(sizeof(char) * path_size);
        if (!path) {
            exit(ERR_REALLOC);
        }
        read_from_fd(fd[0], path, sizeof(char) * path_size);

        // Читаем числа
        read_from_fd(fd[0], &size, sizeof(size_t));
        read_from_fd(fd[0], &divident, sizeof(int));

        for (int i = 0; i < size - 1; ++i) {
            int num;
            read_from_fd(fd[0], &num, sizeof(int));
            divisor *= num;
        }
        close(fd[0]);
        close(fd[1]);

        if (divisor == 0) {
            exit(ERR_DIV_0);
        }

        int res = divident / divisor;
        int length = get_int_length(res);
        char s[length];
        int_to_string(s, res);

        int fp = open(path, O_CREAT | O_WRONLY); // создаем, если не сущ., только для записи
        if (fp == -1) { // Если не получилось открыть файл
            free(path);
            close(fp);
            exit(ERR_OPEN);
        }
        write_to_fd(fp, s, sizeof(char) * length);

        close(fp);
        free(path);
        exit(0);
    } else {
        // Родитель, id = child_id
        char** strs = NULL;
        size_t size = 0;
        char* path;
        size_t path_size = 0;

        printf("[%d] Reading...\n", getpid()); fflush(stdout);
        read_string(0, &path, &path_size); // Читает имя выходного файла
        read_strings(0, &strs, &size); // Читаем числа до \n в массив char* и переводим в массив int'ов

        int nums[size];
        str_array_to_int(strs, nums, size);

        printf("[%d] Red all values.\n", getpid()); fflush(stdout);
        for (size_t i = 0; i < size; ++i) {
            printf("%zu) %d\n", i, nums[i]);
        }

        printf("[%d] Writing...\n", getpid()); fflush(stdout);
        write_to_fd(fd[1], &path_size, sizeof(size_t));
        write_to_fd(fd[1], path, sizeof(char) * path_size);
        write_to_fd(fd[1], &size, sizeof(size_t));
        for (size_t i = 0; i < size; ++i) {
            write_to_fd(fd[1], nums + i, sizeof(int));
        }
        close(fd[0]);
        close(fd[1]);
        printf("[%d] Wrote all values.\n", getpid());

        free(strs);

        if (waitpid(id, &status, 0) == -1) {
            perror("wait() error\n");
            exit(ERR_WAIT);
        }

        if (WIFEXITED(status)) { // Если ребенок завершился со статусом завершения
            int exit_code = WEXITSTATUS(status);
            switch (exit_code) {
            case ERR_DIV_0:
                perror("error: you cant divide by zer0\n");
                exit(exit_code);
                break;
            case ERR_OPEN:
                perror("error: can't open file\n");
                exit(exit_code);
                break;
            case ERR_CLOSE:
                perror("error: can't close file\n");
                exit(exit_code);
                break;
            case ERR_WRITE:
                perror("error: can't write to file\n");
                exit(exit_code);
                break;
            case ERR_READ:
                perror("error: can't read from file\n");
                exit(exit_code);
                break;
            case ERR_REALLOC:
                perror("error: realloc error\n");
                exit(exit_code);
                break;
            
            default:
                printf("[%d] Child ended with status: %d\n", getpid(), status); fflush(stdout);
                break;
            }
        } else {
            perror("error: child exit with no status\n");
            exit(ERR_NO_STATUS);
        }
    }
    return 0;
}
