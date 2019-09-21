#include <stdlib.h>
#include <stdio.h>
#include "hw_malloc.h"
#include <unistd.h>
#include <sys/mman.h>
#define CHUNK_HEADER_SIZE	24
#define MMAP_THRESHOLD	32768
#define KB 1024

//define the type of chunk
typedef size_t chunk_ptr_t;
typedef size_t chunk_info_t;
typedef struct ck {
    struct ck *prev;
    struct ck *next;
    chunk_info_t size_and_flag;
} chunk_header;
//global variable
void *Mstart;//to record the start of memory
void *Mtemp;
void *Mapstart;
int Count;//to record the number of memory been use
chunk_header *C_head;
chunk_header *Bin_head;
chunk_header *Bin[11];
chunk_header *Allo;
chunk_header *Mmap;
chunk_header *Alo_to_bin;
//function
chunk_info_t Set_chunk_info(size_t pre_size,size_t cur_size,size_t aflag,size_t mflag);//this is to set the chunkinfo
size_t Get_chunk_info(int which,chunk_info_t info);//this is to get the info from chunkinfo
void Test_fit_size(size_t bytes);
void PrintbinX(size_t binnum);
void PrintMmap();
//function about list
void list_add(chunk_header *head,chunk_header *insert,size_t i_cursize,size_t i_aflag,size_t mflag);//add to the list and sort by the asdending address
void list_mmap_add(chunk_header *head,chunk_header *insert,size_t i_cursize,size_t i_aflag,size_t mflag);
void list_bintake(chunk_header *head);//to remove the first chunk in the list
size_t list_search_rm(chunk_header *head,chunk_header *findthis);//to check if this chunk is in the list // if find =1 notfind =0
size_t list_merge_take(chunk_header *head,chunk_header *elsebin,size_t thissize);//to check if there is a free chunk in the same bin(with same size)next to each other or not,if there is one,merge and take out the chunk and return "1",if not then return 0

size_t Power2(size_t power)
{
    size_t i;
    size_t value = 1;
    for( i = 0 ; i < power ; i++)
        value = value * 2;
    return value;
}
size_t Testpower(size_t number)
{
    size_t power = 0;

    while(Power2(power) < number)
        power += 1;

    if(power < 5)
        power = 5;

    return power;
}

void *hw_malloc(size_t bytes)
{
    size_t now_alo_size = 0;
    size_t insert_which_bin = 0;
    size_t take_which_bin = 0;
    //use heap
    if(bytes <= MMAP_THRESHOLD - CHUNK_HEADER_SIZE) {
        //first use heap
        if(Count == 0) {

            Mtemp = Mstart;
//x            printf("MS:%p\n",Mstart);
            C_head = Mtemp;
            C_head->prev = Mstart;//?????????????????????
            C_head->next = Mtemp + CHUNK_HEADER_SIZE + bytes;
            C_head->size_and_flag = Set_chunk_info(0,32*KB,1,0);
//			size_t g = Get_chunk_info(1,C_head->size_and_flag);
//			printf("info:%lx\n,size:%d\n",C_head->size_and_flag,sizeof(C_head->size_and_flag));
            Bin_head = Mtemp + 32*KB;
            list_add(Bin[10],Bin_head,32*KB,0,0);

            now_alo_size = Get_chunk_info(2,C_head->size_and_flag);
            while( ( (now_alo_size / 2) >= ( bytes + CHUNK_HEADER_SIZE) )&&( (now_alo_size / 2) >= Power2(5) ) ) {
//x                printf("alo/2:%d > %d\n",now_alo_size/2,bytes+CHUNK_HEADER_SIZE);
                C_head->size_and_flag = Set_chunk_info(0,(now_alo_size/2),1,0);
                insert_which_bin = Testpower(now_alo_size/2) - 5;
                Bin_head = Mtemp + (now_alo_size/2) ;
                list_add(Bin[insert_which_bin],Bin_head,(now_alo_size/2),0,0);
                now_alo_size = now_alo_size / 2;
            }

            list_add(Allo,C_head,now_alo_size,1,0);
            Count+=1;
        }
        //not first
        else if(Count > 0) {
            take_which_bin = Testpower(CHUNK_HEADER_SIZE+bytes) - 5 ;
            //to check if the free chunk in that bin is exist
            //if not exsit, then choose the bigger one
            //if there is no bigger one(bin>=16) then means no free chunk
            //then return NULL
            while(Bin[take_which_bin]->next == Bin[take_which_bin]) {
                take_which_bin += 1;
//return null
                if(take_which_bin >= 11) {
                    printf("no free memory\n");
                    return NULL;
                }
            }
            C_head = Bin[take_which_bin]->next;
            list_bintake(Bin[take_which_bin]);
            now_alo_size = Get_chunk_info(2,C_head->size_and_flag);
            while( ((now_alo_size/2) >= CHUNK_HEADER_SIZE+bytes) && ((now_alo_size/2) >= Power2(5)) ) {
//x                printf("nalo/2:%d > %d\n",now_alo_size/2,CHUNK_HEADER_SIZE+bytes);
                C_head->size_and_flag = Set_chunk_info(0,(now_alo_size/2),1,0);
                insert_which_bin = Testpower(now_alo_size/2) - 5;
                Bin_head = (void *)C_head + (now_alo_size/2);
                list_add(Bin[insert_which_bin],Bin_head,(now_alo_size/2),0,0);
                now_alo_size = now_alo_size/2;
            }
            list_add(Allo,C_head,now_alo_size,1,0);

        }
    }
    // the memory that user want is lager then mmap_threshold
    else if( bytes > MMAP_THRESHOLD - CHUNK_HEADER_SIZE) {
        C_head = (void *)mmap(0,bytes+CHUNK_HEADER_SIZE,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
        list_mmap_add(Mmap,C_head,bytes+CHUNK_HEADER_SIZE,1,1);

//x        printf("tmep_map:%p\nsize:%d\n",Mmap->next,Get_chunk_info(2,Mmap->next->size_and_flag));
    }


    return (void*)C_head+CHUNK_HEADER_SIZE;
}

int hw_free(void *mem)
{
    void *head_mem;
    size_t insert_which_bin = 0;
    size_t free_chunk_size = 0;
    size_t success_merge = 0;
    int testmun = 0;
    head_mem = mem - CHUNK_HEADER_SIZE;//bcz the user input mem is the data head not the header head
//x    printf("hwmem:%p\n",head_mem);
    int success = 0;//not found = 0,heap_found = 1,mmap_found = 2
    success = list_search_rm(Allo,(chunk_header*)head_mem);
//x    printf("success1:%d",success);
    if(success == 0) {
        //if fail then check if the target free chunk is in mmaplist
        chunk_header *temp_header = head_mem;
        success = list_search_rm(Mmap,(chunk_header*)head_mem);
//x        printf("success2:%d\n",success);
        if(success == 1) {
            free_chunk_size = Get_chunk_info(2,temp_header->size_and_flag);
//x            printf("free_chunk_size:%d\n",free_chunk_size);
            testmun = munmap(head_mem,free_chunk_size);
        }
//x        printf("testmun:%d\n",testmun);
        return success;
    }
    //if target free chunk is in the heap
    else if(success == 1) {
        Alo_to_bin = head_mem;
        free_chunk_size = Get_chunk_info(2,Alo_to_bin->size_and_flag);
        //set the chunk info of free chunk to be free
        Alo_to_bin->size_and_flag = Set_chunk_info(0,free_chunk_size,0,0);
        insert_which_bin = Testpower(free_chunk_size) -5;
        while(insert_which_bin <= 10) {
//x            printf("Alo_to_bin:%p\n",Alo_to_bin);
//x            printf("which_bin:%d",insert_which_bin);
            //bcz if the size is 2^15,then no need to merge,just add in to the bin[10]
            if(insert_which_bin < 10)
                success_merge = list_merge_take(Bin[insert_which_bin],Alo_to_bin,free_chunk_size);
//x            printf("success_merge:%d\n",success_merge);
            if(success_merge == 0 || insert_which_bin == 10) {
                list_add(Bin[insert_which_bin],Alo_to_bin,free_chunk_size,0,0);
                break;
            } else if(success_merge == 1) {
                free_chunk_size = free_chunk_size*2;
                insert_which_bin += 1;//bcz if merge success means that need to check if the double size can be merge or not
            }

        }


    }
    //if target free chunk is not in the heap is mmap
    else if(success == 3) {
    }

    return success;
}

void *get_start_sbrk(void)
{
    int i;
    Mstart = sbrk(64*KB);
    Mapstart = Mstart + 64*KB;
//x    printf("Mend:%p\n",sbrk(0));
    for( i = 0 ; i < 11 ; i++) {
        Bin[i] = (chunk_header*)malloc(sizeof(chunk_header));
        Bin[i]->size_and_flag = 0;
        Bin[i]->prev = Bin[i];
        Bin[i]->next = Bin[i];
    }

    Allo = (chunk_header*)malloc(sizeof(chunk_header));
    Allo->size_and_flag = 0;
    Allo->prev = Allo;
    Allo->next = Allo;

    Mmap = (chunk_header*)malloc(sizeof(chunk_header));
    Mmap->size_and_flag = 0;
    Mmap->prev = Mmap;
    Mmap->next = Mmap;


//x    printf("%p\n",Mstart);
    return Mstart;
}


chunk_info_t Set_chunk_info(size_t pre_size,size_t cur_size,size_t aflag,size_t mflag)
{
    chunk_info_t re_value = 0;
    re_value = re_value | (chunk_info_t)pre_size;
    re_value = re_value << 31;
    re_value = re_value | (chunk_info_t)cur_size;
    re_value = re_value << 1;
    re_value = re_value | (chunk_info_t)aflag;
    re_value = re_value << 1;
    re_value = re_value | (chunk_info_t)mflag;

    return re_value;
}

size_t Get_chunk_info(int which,chunk_info_t info)
{
    size_t pre_size = 0;
    size_t cur_size = 0;
    size_t aflag = 0;
    size_t mflag = 0;
    mflag = info % 2;
    aflag = (info % 4)/2;
    cur_size = (info / 4) % 2147483648;
    pre_size = info >> 33;
//	printf("%d %d %d %d\n",pre_size,cur_size,aflag,mflag);

    if(which ==1)
        return pre_size;
    else if(which == 2)
        return cur_size;
    else if(which == 3)
        return aflag;
    else if(which == 4)
        return mflag;

    return 0;

}

void PrintbinX(size_t binnum)
{
    chunk_header *temp_cur;
    temp_cur = Bin[binnum]->next;
    if(temp_cur == Bin[binnum]) {
        printf("there is no free chunk in this Bin[%d].\n",binnum);
        return;
    }
    while(temp_cur != Bin[binnum]) {
//x        printf("temp:%.8p\n",temp_cur);
//x        printf("Mstart:%8p\n",Mstart);
        printf("0x%012x--------%d\n",(void*)temp_cur-Mstart,Get_chunk_info(2,temp_cur->size_and_flag));
        temp_cur = temp_cur->next;
    }

    return;

}
void PrintMmap()
{
    chunk_header *temp_cur;
    temp_cur = Mmap->next;
    if(temp_cur == Mmap) {
        printf("there is no allocated chunk in Mmap-list.\n");
        return;
    }
    while(temp_cur != Mmap) {
        printf("%.12p--------%d\n",temp_cur,Get_chunk_info(2,temp_cur->size_and_flag));
//        fflush(stdout);
        temp_cur = temp_cur->next;
    }

    return;

}
void list_add(chunk_header *head,chunk_header *insert,size_t i_cursize,size_t i_aflag,size_t i_mflag)
{
    size_t temp_aflag,temp_mflag,temp_cursize;
    chunk_header *temp_pre;
    chunk_header *temp_cur;
    temp_pre = head;
    temp_cur = head->next;
    while(temp_cur != head && temp_cur < insert) {
        temp_pre = temp_cur;
        temp_cur = temp_cur->next;
    }
    insert->next = temp_cur;
    insert->prev = temp_pre;
    temp_pre->next = insert;
    temp_cur->prev = insert;

    if(temp_cur != head) { //innsert not at last
        temp_mflag = Get_chunk_info(4,temp_cur->size_and_flag);
        temp_aflag = Get_chunk_info(3,temp_cur->size_and_flag);
        temp_cursize = Get_chunk_info(2,temp_cur->size_and_flag);

        temp_cur->size_and_flag = Set_chunk_info(Get_chunk_info(2,insert->size_and_flag),temp_cursize,temp_aflag,temp_mflag);
    }
    insert->size_and_flag = Set_chunk_info(Get_chunk_info(2,temp_pre->size_and_flag),i_cursize,i_aflag,i_mflag);
}

void list_mmap_add(chunk_header *head,chunk_header *insert,size_t i_cursize,size_t i_aflag,size_t i_mflag)
{
    size_t temp_aflag,temp_mflag,temp_cursize;
    chunk_header *temp_pre;
    chunk_header *temp_cur;
    temp_pre = head;
    temp_cur = head->next;
    while(temp_cur != head && Get_chunk_info(2,temp_cur->size_and_flag) < i_cursize) {
        temp_pre = temp_cur;
        temp_cur = temp_cur->next;
    }
    insert->next = temp_cur;
    insert->prev = temp_pre;
    temp_pre->next = insert;
    temp_cur->prev = insert;

    if(temp_cur != head) { //innsert not at last
        temp_mflag = Get_chunk_info(4,temp_cur->size_and_flag);
        temp_aflag = Get_chunk_info(3,temp_cur->size_and_flag);
        temp_cursize = Get_chunk_info(2,temp_cur->size_and_flag);

        temp_cur->size_and_flag = Set_chunk_info(Get_chunk_info(2,insert->size_and_flag),temp_cursize,temp_aflag,temp_mflag);
    }
    insert->size_and_flag = Set_chunk_info(Get_chunk_info(2,temp_pre->size_and_flag),i_cursize,i_aflag,i_mflag);
}

void list_bintake(chunk_header *head)
{
    chunk_header *temp_cur;
    chunk_header *temp_pre;
    temp_pre = head;
    temp_cur = head->next;
    if( temp_cur == head )
        return;
    temp_cur = temp_cur->next;
    temp_pre->next = temp_cur;
    temp_cur->prev = temp_pre;

}

size_t list_search_rm(chunk_header *head,chunk_header *findthis)
{
    size_t find_or_not = 0;//find = 1 notfind = 0
    chunk_header *temp_cur;
    temp_cur = head->next;

    while(temp_cur != head) {
//		printf("temp_cur:%p\n",temp_cur);
//		printf("findthis:%p\n",findthis);
        if(temp_cur == findthis) {
            temp_cur->prev->next = temp_cur->next;
            temp_cur->next->prev = temp_cur->prev;
            find_or_not = 1;
            break;
        } else
            find_or_not = 0;
        temp_cur = temp_cur->next;
    }
    return find_or_not;
}

size_t list_merge_take(chunk_header *head,chunk_header *elsebin,size_t thissize)
{
    size_t success_merge = 0;//1 is success,0 is fail
    chunk_header *temp_cur;
    chunk_header *temp_pre;
    temp_pre = head;
    temp_cur = head->next;
    if(temp_cur == head) {
        success_merge = 0;
        return success_merge;
    }

    while(temp_cur != head) {
        //if new free chunk that same size is next to curchunk and at lower add
        if( (void*)temp_cur - thissize == (void*)elsebin) {
//x            printf("in low |cur:%p elsebin:%p\n",temp_cur,elsebin);
            success_merge = 1 ;//success
            //no need to  change the header just modify the size
            temp_cur = temp_cur->next;
            temp_cur->prev = temp_pre;
            temp_pre->next = temp_cur;
            temp_cur->size_and_flag = Set_chunk_info(Get_chunk_info(2,temp_pre->size_and_flag),Get_chunk_info(2,temp_cur->size_and_flag),0,0);//reset info of next chunk
            elsebin->size_and_flag = Set_chunk_info(0,thissize*2,0,0);//merge to double size
            Alo_to_bin = elsebin;
            break;
        }
        //if new free chunk that same size is next to curchunk and at higer add;
        else if( (void*)temp_cur + thissize == (void*)elsebin) {
//x            printf("in high |cur:%p elsebin:%p\n",temp_cur,elsebin);
            success_merge = 1;//success
            elsebin = temp_cur;//change the header //and middle
//x            printf("newelsebin:%p\n",elsebin);
            temp_cur = temp_cur->next;//bcz dont want the middle and connect next and last
            temp_cur->prev = temp_pre;
            temp_pre->next = temp_cur;
            elsebin->size_and_flag = Set_chunk_info(0,thissize*2,0,0);//merge to double size
            temp_cur->size_and_flag = Set_chunk_info(Get_chunk_info(2,temp_pre->size_and_flag),Get_chunk_info(2,temp_cur->size_and_flag),0,0);//to reset the info of next free chunk;
            Alo_to_bin = elsebin;
            break;
        }
        temp_pre = temp_cur;
        temp_cur = temp_cur->next;
    }

    return success_merge;
}
