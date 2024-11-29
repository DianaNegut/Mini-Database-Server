
#ifndef FILE_PAGING_H
#define FILE_PAGING_H

#include <stdio.h>
#define PAGE_SIZE 4096  


int get_total_pages(FILE *file);
char* read_page(FILE *file, int page_num);

#endif // FILE_PAGING_H
