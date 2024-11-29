#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define PAGE_SIZE 90

char global_string[PAGE_SIZE * 10] = {0};  


int get_total_pages(int file_size) {
    return (file_size + PAGE_SIZE - 1) / PAGE_SIZE;
}


char* get_text_before_substring(const char *str, const char *substring) {
    char *found = strstr(str, substring);
    if (found == NULL) {
        return NULL;
    }
    size_t length_before_substring = found - str;
    char *result = (char *)malloc(length_before_substring + 1);
    if (result == NULL) {
        perror("Alocare esuată");
        return NULL;
    }
    strncpy(result, str, length_before_substring);
    result[length_before_substring] = '\0';
    return result;
}

char* read_page(void *mapped_data, int page_num, off_t file_size) {
    long offset = page_num * PAGE_SIZE;  
    char buffer[PAGE_SIZE];
    if (offset >= file_size) {
        return NULL;  
    }

    char *page_data = (char *)mapped_data + offset;
    page_data[PAGE_SIZE-1]=0;

    return page_data;
}

void append_to_global_string(const char* text, int text_len) {
    strncat(global_string, text, text_len);  
}

int main() {
    struct stat file_stat;
    int fd = open("angajati", O_RDONLY);
    if (fd == -1) {
        perror("Nu s-a putut deschide fișierul");
        return 1;
    }

    if (fstat(fd, &file_stat) == -1) {
        perror("Eroare la obținerea informațiilor despre fișier");
        close(fd);
        return 1;
    }

    void *mapped_data = mmap(NULL, PAGE_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (mapped_data == MAP_FAILED) {
        perror("Eroare la maparea fișierului în memorie");
        close(fd);
        return 1;
    }

    int total_pages = get_total_pages(file_stat.st_size);
    printf("Fișierul are %d pagini.\n", total_pages);

    int page_num = 0;
    char *page_data = read_page(mapped_data, page_num, file_stat.st_size);
    printf("%s", page_data);
    
    



    if (munmap(mapped_data, file_stat.st_size) == -1) {
        perror("Eroare la eliberarea memoriei mapate");
    }
    close(fd);

    return 0;
}
