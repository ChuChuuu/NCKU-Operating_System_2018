#include "lib/hw_malloc.h"
#include "hw4_mm_test.h"
#include <stdio.h>
#include <string.h>

void *Add_start;
void alloc(size_t bytes);
void free(void *ADDR);
int main(int argc, char *argv[])
{
    int i;
    Add_start = get_start_sbrk();

    size_t want_size = 0;
    size_t want_which_bin = 0;
    void *want_address;
    char input[100]="";
    char command[10]="";
    char bin_mmap[20]="";
//	char temp[10] = "";
    while(fgets(input,100,stdin)) {
//        fgets(input,100,stdin);
        sscanf(input,"%s",&command);
        if(strncmp(command,"alloc",5) == 0) {
            sscanf(input,"%*s %d",&want_size);
//            printf("want_size:%d\n",want_size);
            alloc(want_size);
        } else if(strncmp(command,"free",4) == 0) {
            sscanf(input,"%*s %p",&want_address);
//            printf("want_address:%p\n",want_address);
            free(want_address);

        } else if(strncmp(command,"print",5) == 0) {
            sscanf(input,"%*s %s",&bin_mmap);
//            printf("bin_mmap:%s\n",bin_mmap);
            if(strncmp(bin_mmap,"bin",3) == 0) {
                sscanf(bin_mmap,"%*[^[][%d",&want_which_bin);
//                printf("want_which_bin:%d\n",want_which_bin);
                PrintbinX(want_which_bin);
            } else if(strncmp(bin_mmap,"mmap_alloc_list",15) == 0) {
                PrintMmap();
            }
        }


    }


//    printf("---------------------------\n");
//    for(i = 0 ; i < 11 ; i ++)
//        PrintbinX(i);


    return 0;
}

void alloc(size_t bytes)
{
    void *true_add;
    true_add = hw_malloc(bytes);
    if(true_add == NULL)
        return;
    if(bytes <= 32*1024-24)
        printf("%.12p\n",(void*)true_add - Add_start);
    else
        printf("%.12p\n",true_add);

}

void free(void* ADDR)
{
    void *true_add;
    int success_fail;
    true_add = Add_start + (size_t)ADDR;
    if( (size_t)ADDR <= 64*1024)
        success_fail = hw_free(true_add);
    else
        success_fail = hw_free(ADDR);

    if(success_fail == 0)
        printf("fail\n");
    else if(success_fail == 1)
        printf("success\n");

    return;


}
