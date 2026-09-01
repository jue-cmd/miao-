#include "header/memory.h"
#include "header/stdint.h"
#include "header/strings.h"
#include "header/bitmap.h"
#include "header/debug.h"
#include "header/globoal.h"
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
#define K_HEAP_START 0xc0100000

#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22) // 获取页目录项的索引
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12) // 获取页表项的索引
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
     ASSERT(all_mem != 0);
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

/*
在fp中的虚拟内存池中申请pg_cnt个页,如果成功返回虚拟页的地址,失败返回NULL
*/
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt)
{
     int vaddr_start = 0;
     int bit_idx_start = -1;
     uint32_t cnt = 0;
     if (pf == PF_KERNEL)
     {
          bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
          if (bit_idx_start == -1)
          {
               return NULL;
          }
          while (cnt < pg_cnt)
          {
               bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt, 1);
               cnt++;
          }
          vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
     }
     else
     {
          // TODO: 用户内存池
     }
     return (void *)vaddr_start;
}

/**
 * 获取vaddr对应的pte指针
 */
uint32_t *get_pte_ptr_from_vaddr(uint32_t vaddr)
{
     uint32_t *pte =
         (uint32_t *)(0xffc00000 +
                      ((vaddr & 0xffc00000) >> 10) +
                      PTE_IDX(vaddr) * 4);
     return pte;
}
uint32_t *get_pde_ptr_from_vaddr(uint32_t vaddr)
{
     uint32_t *pde = (uint32_t *)((0xfffff000) + PDE_IDX(vaddr) * 4);
     return pde;
}

static void *palloc(struct pool *m_pool)
{
     int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
     if (bit_idx == -1)
     {
          return NULL;
     }
     bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
     uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
     return (void *)page_phyaddr;
}

static void page_table_add(void *_vaddr, void *_page_phyaddr)
{
     uint32_t vaddr = (uint32_t)_vaddr;
     uint32_t page_phyaddr = (uint32_t)_page_phyaddr;
     uint32_t *pde = get_pde_ptr_from_vaddr(vaddr);
     uint32_t *pte = get_pte_ptr_from_vaddr(vaddr);

     /* PDE 已存在：页表可用，只需安装 PTE（PTE 必须尚不存在） */
     if (IS_PRESENT(*pde))
     {
          if (IS_PRESENT(*pte))
          {
               puts("page_table_add: PTE already present\n");
               puts("  vaddr=");
               print_num32_hex(vaddr);
               puts(" phy=");
               print_num32_hex(page_phyaddr);
               puts("\n  pde*=");
               print_num32_hex((uint32_t)pde);
               puts(" *pde=");
               print_num32_hex(*pde);
               puts("\n  pte*=");
               print_num32_hex((uint32_t)pte);
               puts(" *pte=");
               print_num32_hex(*pte);
               puts("\n");
          }
          ASSERT(!IS_PRESENT(*pte));
          *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
     }
     else
     {
          /* PDE 不存在：先分配页表，清零后再安装 PTE */
          uint32_t pde_phyaddr = (uint32_t)palloc(&kernel_pool);
          *pde = (pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
          memset((void *)((uint32_t)pte & 0xfffff000), 0, PG_SIZE);
          ASSERT(!IS_PRESENT(*pte));
          *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
     }
}

void *malloc_page(enum pool_flags pf, uint32_t pg_cnt)
{
     ASSERT(pg_cnt > 0 && pg_cnt <= 3840);
     void *vaddr_start = vaddr_get(pf, pg_cnt);
     if (vaddr_start == NULL)
     {
          return NULL;
     }
     uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;
     struct pool *mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;
     while (cnt > 0)
     {
          void *page_phyaddr = palloc(mem_pool);
          if (page_phyaddr == NULL)
          {
               return NULL;
          }
          page_table_add((void *)vaddr, page_phyaddr);
          vaddr += PG_SIZE;
          cnt--;
     }
     return vaddr_start;
}

void *get_kernel_pages(uint32_t pg_cnt)
{
     void *vaddr = malloc_page(PF_KERNEL, pg_cnt);
     if (vaddr != NULL)
     {
          memset(vaddr, 0, pg_cnt * PG_SIZE);
     }
     return vaddr;
}

// TODO
uint32_t get_total_memory()
{
}

void mem_init()
{
     puts("mem_init start\n");
     uint32_t mem_bytes_total = (*(uint32_t *)0xb00);
     puts("total_mem: ");
     print_num32_hex(mem_bytes_total);
     puts("\n");
     mem_pool_init(mem_bytes_total);
     puts("mem_init done");
}