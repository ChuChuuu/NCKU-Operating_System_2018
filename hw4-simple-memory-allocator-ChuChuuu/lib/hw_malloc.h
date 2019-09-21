#ifndef HW_MALLOC_H
#define HW_MALLOC_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern void *hw_malloc(size_t bytes);
extern void *get_start_sbrk(void);
extern int hw_free(void *mem);
extern void PrintbinX(size_t binnum);
extern void PrintMmap();
extern size_t Power2(size_t power);
extern size_t Testpower(size_t number);

#endif
