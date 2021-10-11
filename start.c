#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <sys/types.h>

#define BLOCK_SIZE 64

typedef enum {dm, fa} cache_map_t;
typedef enum {uc, sc} cache_org_t;
typedef enum {instruction, data} access_t;

typedef struct {
    uint32_t address;
    access_t accesstype;
} mem_access_t;

typedef struct {
    uint64_t accesses;
    uint64_t hits;
    uint64_t evicts;
    // You can declare additional statistics if
    // you like, however you are now allowed to
    // remove the accesses or hits
} cache_stat_t;

/* typedef struct { */
/*     u_int16_t head; */
/*     u_int16_t tail; */
/*     u_int8_t [cache_size]; */
/* } que; */

// DECLARE CACHES AND COUNTERS FOR THE STATS HERE


uint32_t cache_size; 
uint32_t memory_size = 32; 
uint32_t block_size = 64;
cache_map_t cache_mapping;
cache_org_t cache_org;

//egne greier
struct cache {
    uint32_t tag;
    int valid;
    int pos[BLOCK_SIZE];
    int lastPos;
};
struct fa_cache {
    uint32_t tag;
    u_int8_t valid;
    int pos[BLOCK_SIZE];
};
// USE THIS FOR YOUR CACHE STATISTICS
cache_stat_t cache_statistics;


/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access
 * 2) memory address
 */
mem_access_t read_transaction(FILE *ptr_file) {
    char buf[1000];
    char* token;
    char* string = buf;
    mem_access_t access;

    if (fgets(buf,1000, ptr_file)!=NULL) {

        /* Get the access type */
        token = strsep(&string, " \n");        
        if(cache_org ==sc){
            if (strcmp(token,"I") == 0) {
                access.accesstype = instruction;
            } else if (strcmp(token,"D") == 0) {
                access.accesstype = data;
            } else {
                printf("Unkown access type\n");
                exit(0);
            }
        }
        //if cache_org is unified just set every instruction to the same cache
        else {
            access.accesstype = instruction;
        }
        
        token = strsep(&string, " \n");
        access.address = (uint32_t)strtol(token, NULL, 16);

        return access;
    }

    /* If there are no more entries in the file,  
     * return an address 0 that will terminate the infinite loop in main
     */
    access.address = 0;
    return access;
}

int isInteger(double number){
    double temp;

    temp = number - (int) number;

    if(temp > 0){
        return 0;
    }
    else{
        return 1;
    }
}
int power2(uint32_t exponent){
    return 1 << exponent;
}
void main(int argc, char** argv)
{
    double cache_size_bits_check;
    //number of bits used to acces cache in a dm cache
    uint32_t cache_size_bits;
    uint32_t cache_entries;
    uint32_t cache_pos_bits;
    // Reset statistics:
    memset(&cache_statistics, 0, sizeof(cache_stat_t));

    /* Read command-line parameters and initialize:
     * cache_size, cache_mapping and cache_org variables
     */

    if ( argc != 4 ) { /* argc should be 2 for correct execution */
        printf("Usage: ./cache_sim [cache size: 128-4096] [cache mapping: dm|fa] [cache organization: uc|sc]\n");
        exit(0);
    } else  {
        /* argv[0] is program name, parameters start with argv[1] */

        /* Set cache size */
        cache_size = atoi(argv[1]);
        double cache_size_bits_check = log2(cache_size);
        if (!isInteger(cache_size_bits_check)){
            printf("cache size has to be a power of 2\n");
            exit(0);
        }
        if(cache_size < 128 || cache_size > 4096){
            printf("cache size has to be in range 128-4096\n");
            exit(0);
        }
        //if cache organization is split in 2 split cache size between them
        /* Set Cache Mapping */
        if (strcmp(argv[2], "dm") == 0) {
            cache_mapping = dm;
        } else if (strcmp(argv[2], "fa") == 0) {
            cache_mapping = fa;
        } else {
            printf("Unknown cache mapping\n");
            exit(0);
        }

        /* Set Cache Organization */
        if (strcmp(argv[3], "uc") == 0) {
            cache_org = uc;
        } else if (strcmp(argv[3], "sc") == 0) {
            cache_org = sc;
        } else {
            printf("Unknown cache organization\n");
            exit(0);
        }
        if(cache_org == sc){
            printf("sc\n");
            cache_size = cache_size/2;
        }
    }


    /* Open the file mem_trace.txt to read memory accesses */
    FILE *ptr_file;
    ptr_file =fopen("mem_trace.txt","r");
    if (!ptr_file) {
        printf("Unable to open the trace file\n");
        exit(1);
    }

    //egne greier
    cache_entries = cache_size/block_size;
    cache_size_bits = log2(cache_entries);
    cache_pos_bits = log2(block_size);
    
    /* Loop until whole trace file has been read */
    mem_access_t access;
    if (cache_mapping == dm){
        struct cache instans[cache_entries];
        struct cache instans2[cache_entries * cache_org];
        // set the memory in instan to 0 
        memset(&instans, 0, sizeof(instans));
        /* memset(&instans2, 0, sizeof(instans2)); */
        printf("bloack ofsett = %d\n", cache_pos_bits);
        printf("tag ofsett = %d\n", (32 -cache_pos_bits) - cache_size_bits);
        printf("index ofsett = %d\n", cache_size_bits);
        printf("cache size = %d\n", cache_size);
        printf("bloack entries = %d\n", cache_entries);

        while(1) {
            access = read_transaction(ptr_file);
            //If no transactions left, break out of loop
            /* printf("run\n"); */
            if (access.address == 0)
                break;
            uint64_t tag = access.address >> (cache_pos_bits + cache_size_bits);
            uint64_t pos = access.address %block_size;
            uint64_t place = access.address%power2(cache_pos_bits+cache_size_bits) >> cache_pos_bits;
            printf("tag %d\n", tag);
            printf("place %d\n", place);
            printf("pos/block %d\n", pos);
            cache_statistics.accesses += 1;
            if (access.accesstype == instruction){
                /* puts("runrun222"); */
                if (instans[place].tag == tag && instans[place].valid == 1){
                    if (instans[place].pos[pos] == 1){
                        /* printf("this memory adress is used %x\n", access.address); */
                        cache_statistics.hits += 1;
                        continue;
                    }
                }
                else if(instans[place].valid == 1 && instans[place].pos[pos] == 1){
                    instans[place].tag = tag;
                    instans[place].valid = 1;
                    instans[place].pos[pos] = 1;
                    cache_statistics.evicts += 1;
                    continue;

                }
                instans[place].tag = tag;
                instans[place].valid = 1;
                instans[place].pos[pos] = 1;
                continue;
            }
            else{
                puts("runrun");
                if (instans2[place].tag == tag && instans2[place].valid == 1){
                    if (instans2[place].pos[pos] == 1){
                        /* printf("this memory adress is used %x\n", access.address); */
                        cache_statistics.hits += 1;
                        continue;
                    }
                }
                instans2[place].tag = tag;
                instans2[place].valid = 1;
                instans2[place].pos[pos] = 1;
            }
        }
    }
    else{
        u_int16_t head;
        u_int16_t tail;
        u_int8_t que[cache_size];
        struct cache instans[cache_entries];
        struct cache instans2[cache_entries * cache_org];
        memset(&instans, 0, sizeof(instans));
        memset(&instans2, 0, sizeof(instans2));

        head = 0;
        tail = 0;
        memset(&que, 0, sizeof(que));

        /* struct cache fa_cache_instans[cache_entries]; */
        /* memset(&fa_cache_instans, 0, sizeof(fa_cache_instans)); */
        /* printf("elser den?\n"); */
        while(1) {
            access = read_transaction(ptr_file);
            //If no transactions left, break out of loop
            if (access.address == 0)
                break;
            // adress - pos
            int tag = access.address >> cache_pos_bits;
            uint8_t pos = access.address % power2(cache_pos_bits);
            uint8_t hit = 0;
            cache_statistics.accesses += 1;
            if (access.accesstype == instruction){
                for (int i=0; i<cache_entries; i++){
                    if (instans[i].tag == tag && instans[i].valid == 1 && instans[i].pos[pos] == 1){
                        /* printf("this memory adress is used %x\n", access.address); */
                        cache_statistics.hits += 1;
                        que[head] = i;
                        head = (head +1)%cache_size;
                        hit = 1;
                        break;
                    }
                }
                if(hit){
                    continue;
                }
                hit = 0;
                for (int i=0; i<cache_entries; i++){
                    if (instans[i].valid == 0){
                        /* printf("this memory adress is not used and are now placed inn %d %x\n",i, access.address); */
                        que[head] = i;
                        head = (head +1)%cache_size;
                        instans[i].tag = tag;
                        instans[i].valid = 1;
                        instans[i].pos[pos] = 1;
                        hit = 1;
                        break;
                    }
                }
                if(hit){
                    continue;
                }
                instans[que[tail]].tag = tag;
                instans[que[tail]].valid = 1;
                instans[que[tail]].pos[pos] = 1;
                tail = (tail + 1) % cache_size;
            }
            else{
                for (int i=0; i<cache_entries; i++){
                    if (instans2[i].tag == tag && instans2[i].valid == 1 && instans2[i].pos[pos] == 1){
                        /* printf("this memory adress is used %x\n", access.address); */
                        cache_statistics.hits += 1;
                        que[head] = i;
                        head = (head +1)%cache_size;
                        hit = 1;
                        break;
                    }
                }
                if(hit){
                    continue;
                }
                hit = 0;
                for (int i=0; i<cache_entries; i++){
                    if (instans2[i].valid == 0){
                        /* printf("this memory adress is not used and are now placed inn %d %x\n",i, access.address); */
                        que[head] = i;
                        head = (head +1)%cache_size;
                        instans2[i].tag = tag;
                        instans2[i].valid = 1;
                        instans2[i].pos[pos] = 1;
                        hit = 1;
                        break;
                    }
                }
                if(hit){
                    continue;
                }
                instans2[que[tail]].tag = tag;
                instans2[que[tail]].valid = 1;
                instans2[que[tail]].pos[pos] = 1;
                tail = (tail + 1) % cache_size;

            }
        }
    }
    /* for (int i=0; i<32; i++){ */
    /*     printf("valid ? = %d\n", instans[i].valid); */
    /*     printf("tag ? = %d\n", instans[i].tag); */
    /*     /1* printf("pos ? = %d\n", instans[i].tag); *1/ */
    /*     printf("address ? = %d\n", instans[i].memory); */
    /* } */
    /* Print the statistics */
    // DO NOT CHANGE THE FOLLOWING LINES!
    printf("\nCache Statistics\n");
    printf("-----------------\n\n");
    printf("Accesses: %ld\n", cache_statistics.accesses);
    printf("Hits:     %ld\n", cache_statistics.hits);
    printf("Evicts:   %ld\n", cache_statistics.evicts);
    printf("Hit Rate: %.4f\n", (double) cache_statistics.hits / cache_statistics.accesses);
    // You can extend the memory statistic printing if you like!

    /* Close the trace file */
    fclose(ptr_file);
    printf("heihei\n");

}
