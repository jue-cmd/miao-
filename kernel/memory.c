#include "header/memory.h"
#include "header/stdint.h"
#include "header/strings.h"
#include "header/bitmap.h"
#include "header/debug.h"
//
#define PG_SIZE 4096

// 因为不出问题的话  0xc009f000 是内核主线程栈顶，0xc009e000 是内核主线程的 pcb。
// 一个页框大小的位图可表示 128MB 内存，位图位置安排在地址 0xc009a000，
/**
 * 也就是
 * 0xc009a000 --- 0xc009d000 位图
 * 0xc009e000 --- 0xc009f000 内核主线程的 pcb
 * 0xc009f000                内核主线程栈顶
 */
#define MEM_BITMAP_BASE 0xc009a000
// 因为内核地址空间从3G开始,再加上内核页表大小为1M,所以内核堆的起始地址为3G+1M=0xc0100000
#define K_HEAP_START 0xc0100000

struct pool
{
     struct bitmap pool_bitmap;
     uint32_t phy_addr_start;
     uint32_t pool_size;
};

struct pool kernel_pool, user_pool; // 内核内存池和用户内存池
struct virtual_addr kernel_vaddr;   // 此结构用来给内核分配虚拟地址

static void mem_pool_init(uint32_t all_mem)
{
     ASSERT(all_mem !=0);
     puts("mem_pool_init start\n");
     // 因为内核占有1G的内存空间,所以有256个页表一个页表可以映射1024个页每个页大小为4K.
     uint32_t page_table_size = PG_SIZE * 256;
     uint32_t used_mem = page_table_size + 0x100000; // 0x100000是内核栈大小
     uint32_t free_mem = all_mem - used_mem;
     uint16_t all_free_pages = free_mem / PG_SIZE;
     uint16_t kernel_free_pages = all_free_pages / 2;
     uint16_t user_free_pages = all_free_pages - kernel_free_pages;
     // 不处理余数,这样记录的小于等于实际值,防止内存泄漏(无需进行边界检查)
     // 内核位图的长度
     uint32_t kbm_length = kernel_free_pages / 8;
     // 用户位图的长度
     uint32_t ubm_length = user_free_pages / 8;

     uint32_t kp_start = used_mem;
     uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;
     kernel_pool.phy_addr_start = kp_start;
     user_pool.phy_addr_start = up_start;

     kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
     user_pool.pool_size = user_free_pages * PG_SIZE;

     kernel_pool.pool_bitmap.len = kbm_length;
     user_pool.pool_bitmap.len = ubm_length;

     kernel_pool.pool_bitmap.bits = (uint8_t *)MEM_BITMAP_BASE;
     user_pool.pool_bitmap.bits = (uint8_t *)(MEM_BITMAP_BASE + kbm_length);

     puts("kernel_pool_bitmap_start:");
     print_num32_hex((int)kernel_pool.pool_bitmap.bits);
     puts(" kernel_pool_phy_addr_start:");
     print_num32_hex(kernel_pool.phy_addr_start);
     puts("\n");
     puts("user_pool_bitmap_start:");
     print_num32_hex((int)user_pool.pool_bitmap.bits);
     puts(" user_pool_phy_addr_start:");
     print_num32_hex(user_pool.phy_addr_start);
     puts("\n");

     /* 将位图置 0*/
     bitmap_init(&kernel_pool.pool_bitmap);
     bitmap_init(&user_pool.pool_bitmap);

     /* 下面初始化内核虚拟地址的位图，按实际物理内存大小生成数组。*/
     kernel_vaddr.vaddr_bitmap.len = kbm_length;
     // 用于维护内核堆的虚拟地址，所以要和内核内存池大小一致
     /* 位图的数组指向一块未使用的内存，
    目前定位在内核内存池和用户内存池之外*/
     kernel_vaddr.vaddr_bitmap.bits = (void *)(MEM_BITMAP_BASE + kbm_length + ubm_length);
     kernel_vaddr.vaddr_start = K_HEAP_START;
     bitmap_init(&kernel_vaddr.vaddr_bitmap);

     puts("mem_pool_init done\n");
}

void mem_init()
{
     puts("mem_init start\n");
     //0xb00为loader中计算出的内存大小的地址
     uint32_t mem_bytes_total = (*(uint32_t *)0xb00);
     puts("total_mem: ");
     print_num32_hex(mem_bytes_total);
     puts("\n");
     mem_pool_init(mem_bytes_total);
     puts("mem_init done");
}