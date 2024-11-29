#include "paginare.h"
#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE 4096

int get_total_pages(FILE *file)
{
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    return (file_size + PAGE_SIZE - 1) / PAGE_SIZE;
}

char *read_page(FILE *file, int page_num)
{
    char *page_data = (char *)malloc(PAGE_SIZE + 1);
    if (page_data == NULL)
    {
        perror("Eroare la alocarea memoriei pentru pagină");
        exit(1);
    }

    long offset = page_num * PAGE_SIZE;
    fseek(file, offset, SEEK_SET);

    size_t bytes_read = 0;
    int ch;

    while (bytes_read < PAGE_SIZE && (ch = fgetc(file)) != EOF)
    {
        page_data[bytes_read++] = ch;
        if (ch == '\n')
        { 
            break;
        }
    }

    for (size_t i = bytes_read; i < PAGE_SIZE; i++)
    {
        page_data[i] = '\0';
    }

    page_data[PAGE_SIZE] = '\0'; 
    return page_data;
}
