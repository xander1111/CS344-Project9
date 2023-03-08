#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MEM_SIZE 16384  // MUST equal PAGE_SIZE * PAGE_COUNT
#define PAGE_SIZE 256  // MUST equal 2^PAGE_SHIFT
#define PAGE_COUNT 64
#define PAGE_SHIFT 8  // Shift page number this much

#define PTP_OFFSET 64 // How far offset in page 0 is the page table pointer table

// Simulated RAM
unsigned char mem[MEM_SIZE];

//
// Convert a page,offset into an address
//
int get_address(int page, int offset)
{
    return (page << PAGE_SHIFT) | offset;
}

//
// Initialize RAM
//
void initialize_mem(void)
{
    memset(mem, 0, MEM_SIZE);

    int zpfree_addr = get_address(0, 0);
    mem[zpfree_addr] = 1;  // Mark zero page as allocated
}

//
// Get the page table page for a given process
//
unsigned char get_page_table(int proc_num)
{
    int ptp_addr = get_address(0, PTP_OFFSET + proc_num);
    return mem[ptp_addr];
}

//
// Returns the index of the first free page
//
int allocate_page()
{
    int free_page = -1;
    int map_entry = 0;
    do
    {
        if (mem[++map_entry] == 0) free_page = map_entry;
    } while (map_entry < PAGE_COUNT && free_page < 0);

    if (free_page == -1) return 0xff;
    else {
        mem[free_page] = 1;
        return free_page;
    }
}

//
// Deallocates the page at the given index
//
void deallocate_page(int page_number)
{
    mem[page_number] = 0;
}

//
// Allocate pages for a new process
//
// This includes the new process page table and page_count data pages.
//
void new_process(int proc_num, int page_count)
{
    int page_table_page = allocate_page();

    if (page_table_page == 0xff) {
        printf("OOM: proc %d: page table\n", proc_num);
        return;
    }

    mem[PTP_OFFSET + proc_num] = page_table_page;

    for (int i = 0; i < page_count; i++)
    {
        int new_page = allocate_page();
        if (new_page == 0xff) {
            printf("OOM: proc %d: data page\n", proc_num);
            return;
        }
        mem[get_address(page_table_page, i)] = new_page;
    }
}

//
// Kills the given process, deallocating every page it used
//
void kill_process(int proc_num)
{
    int page_table_page = mem[proc_num + PTP_OFFSET];
    mem[proc_num + PTP_OFFSET] = 0;

    for (int i = 0; i < 256; i++)
    {
        int page_i = get_address(page_table_page, i);
        if (mem[page_i] != 0) {
            deallocate_page(mem[page_i]);
        }
    }

    deallocate_page(page_table_page);
}

//
// Returns the corresponding physical address from a given process number and virtual address
//
int get_physical_addr(int proc_num, int v_addr)
{
    int page_table_page = mem[proc_num + PTP_OFFSET];
    int virt_page = v_addr >> PAGE_SHIFT;
    int physical_page = mem[get_address(page_table_page, virt_page)];

    int offset = v_addr & 255;

    return get_address(physical_page, offset);
}

//
// Stores a value at a given processes virtual address
//
void store_value(int proc_num, int v_addr, int value)
{
    int p_addr = get_physical_addr(proc_num, v_addr);

    mem[p_addr] = value;

    printf("Store proc %d: %d => %d, value=%d\n",
    proc_num, v_addr, p_addr, value);
}

//
// Prints the value stored to a given processes virtual address
//
void load_value(int proc_num, int virt_addr)
{
    int physical_addr = get_physical_addr(proc_num, virt_addr);
    int value = mem[physical_addr];


    printf("Load proc %d: %d => %d, value=%d\n",
    proc_num, virt_addr, physical_addr, value);
}

//
// Print the free page map
//
// Don't modify this
//
void print_page_free_map(void)
{
    printf("--- PAGE FREE MAP ---\n");

    for (int i = 0; i < 64; i++) {
        int addr = get_address(0, i);

        printf("%c", mem[addr] == 0? '.': '#');

        if ((i + 1) % 16 == 0)
            putchar('\n');
    }
}

//
// Print the address map from virtual pages to physical
//
// Don't modify this
//
void print_page_table(int proc_num)
{
    printf("--- PROCESS %d PAGE TABLE ---\n", proc_num);

    // Get the page table for this process
    int page_table = get_page_table(proc_num);

    // Loop through, printing out used pointers
    for (int i = 0; i < PAGE_COUNT; i++) {
        int addr = get_address(page_table, i);

        int page = mem[addr];

        if (page != 0) {
            printf("%02x -> %02x\n", i, page);
        }
    }
}

//
// Main -- process command line
//
int main(int argc, char *argv[])
{
    assert(PAGE_COUNT * PAGE_SIZE == MEM_SIZE);

    if (argc == 1) {
        fprintf(stderr, "usage: ptsim commands\n");
        return 1;
    }
    
    initialize_mem();

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "pfm") == 0) {
            print_page_free_map();
        }
        else if (strcmp(argv[i], "ppt") == 0) {
            int proc_num = atoi(argv[++i]);
            print_page_table(proc_num);
        }
        else if (strcmp(argv[i], "np") == 0) {
            int proc_num = atoi(argv[++i]);
            int page_count = atoi(argv[++i]);
            new_process(proc_num, page_count);
        }
        else if (strcmp(argv[i], "kp") == 0) {
            int proc_num = atoi(argv[++i]);
            kill_process(proc_num);
        }
        else if (strcmp(argv[i], "sb") == 0) {
            int proc_num = atoi(argv[++i]);
            int v_addr = atoi(argv[++i]);
            int value = atoi(argv[++i]);

            store_value(proc_num, v_addr, value);
        }
        else if (strcmp(argv[i], "lb") == 0) {
            int proc_num = atoi(argv[++i]);
            int v_addr = atoi(argv[++i]);

            load_value(proc_num, v_addr);
        }
    }
}
