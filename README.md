# SimpleOS 学习笔记

学习用的简易操作系统。本文记录近期几处**启动 / 链接 / 页表**相关改动的原因与做法，方便对照代码理解。

---

## 怎么构建与运行

```bash
cmake -B build -S .
cmake --build build
cmake --build build --target run      # QEMU
cmake --build build --target run-gdb  # QEMU -s -S，等 GDB
```

构建产物：

| 产物 | 作用 | 写入磁盘位置 |
|------|------|----------------|
| `mbr.bin` | 引导扇区 | LBA 0 |
| `loader.bin` | 进入保护模式、建页表、加载 ELF | LBA 2 |
| `kernel.bin` | 内核 ELF | LBA 9 起（最多 200 扇区） |

磁盘镜像默认是项目根目录的 `c.img`。

相关文件：`CMakeLists.txt`、`cmake/write_disk_image.cmake`。

---

## 改动一：Loader 加载 ELF 后必须清 BSS

### 现象

`mem_init` 等依赖全局变量的路径可能异常 / panic。BSS 里有 `idt_table`、`kernel_pool`、`user_pool` 等，若未清零则是脏数据。

### 原因

ELF `PT_LOAD` 段有两个大小：

- `p_filesz`：文件里实际有的字节（`.text` / `.data` …）
- `p_memsz`：装入内存后应占的字节（还包含 `.bss`）

`.bss` 是 `NOBITS`，**不在文件里**，规范要求加载器把 `[p_vaddr + p_filesz, p_vaddr + p_memsz)` **置 0**。

原先 `kernel_init` 只做了 `mem_cpy(p_filesz)`，没有清 BSS。

可用 `readelf -l build/kernel.bin` 核对，例如 data 段常见：

```text
LOAD  ...  FileSiz 0x000ab  MemSiz 0x00334  RW
```

差值就是 BSS。

### 修复（`mbr/loader.asm` → `kernel_init`）

每个非 `PT_NULL` 段：

1. 按 `p_filesz` 拷贝到 `p_vaddr`
2. 若 `p_memsz > p_filesz`，对剩余区域 `mem_set` 清零

注意：`loop` 用的是 `ecx`（`e_phnum`），清 BSS 前要先 `push ecx` / 用完再 `pop`，否则循环计数会被毁掉。

### 学习要点

- C 里未初始化的全局 / 静态变量落在 `.bss`，依赖「启动时为 0」
- 自己写加载器时，**拷贝文件内容 ≠ 完成加载**，还要处理 `memsz - filesz`

---

## 改动二：`page_table_add` 断言与分支写反

### 现象

```text
mem_pool_init done
mem_init done
KERNEL PANIC!!!
file: memory.c  line: 0157  function: page_table_add
condition: IS_PRESENT(*pte)
```

说明 `mem_init` 已过，崩在第一次给内核堆装页时（常见虚拟地址从 `0xc0100000` 起）。

### 原因

错误写法大致是：

1. `ASSERT(IS_PRESENT(*pte))` —— 新映射时 PTE **本来就不该 Present**
2. 不管 PDE 是否已有，一律 `palloc` 新页表并写 PDE —— 会破坏已有映射（例如 `0xc0000000` 那张表）

正确逻辑应对齐「操作系统真象还原」一类教材：

| 情况 | 该做什么 |
|------|----------|
| PDE 已 Present | 页表已在；断言 **PTE 不存在**；只写 PTE |
| PDE 不存在 | `palloc` 页表 → 写 PDE → `memset` 整张页表 → 再写 PTE |

内核堆 `0xc0100000` 落在 PDE\[768\]（loader 已在 `0xc00` 挂好第一张页表）上，因此会走「PDE 已存在」分支；PTE 索引为：

```text
PTE_IDX(0xc0100000) = ((0xc0100000 & 0x003ff000) >> 12) = 256
```

第一张页表只预先映射了物理 0～1MB（PTE 0～255），**256 号项应为空**，所以断言必须是 `!IS_PRESENT(*pte)`。

### 修复（`kernel/memory.c`）

见当前 `page_table_add`：先看 `*pde`，再分别处理两种分支；两处都断言 **PTE 尚未 Present**，再写入：

```c
*pte = page_phyaddr | PG_US_U | PG_RW_W | PG_P_1;
```

### 学习要点

- `ASSERT(条件)` 失败表示**条件为假**；panic 打印的是失败的那个表达式
- 装页 = 改页表；**新建映射时 PTE 必须从「不存在」变为「存在」**
- PDE / PTE 指针通过「页目录自映射」（末项指向页目录自身）用虚拟地址访问，对应 `get_pde_ptr_from_vaddr` / `get_pte_ptr_from_vaddr`

---

## 改动三：Loader 建页表的两处疏漏

### 3.1 只清了页目录，没清第一张页表

原先只把 `PAGE_DIR_TABLE_POS` 起 4KB 清零，然后只填写 PTE\[0..255\]。  
PTE\[256..1023\] 可能是内存脏数据；若偶然 Present=1，后面 `ASSERT(!IS_PRESENT(*pte))` 会误伤。

现改为清 **页目录 + 第一张页表** 共 8KB，再写 256 个低端映射。

### 3.2 `create_kernel_pde` 写错地方

内核高半区需要 PDE\[769..1022\] 指向后续页表。正确写法应：

- 写回 **页目录** `PAGE_DIR_TABLE_POS`
- 从物理 `PAGE_DIR_TABLE_POS + 0x2000`（第二张页表）起挂表
- PDE 要带上 `PG_US_U | PG_RW_W | PG_P`

错误写法曾用当时的 `ebx`（第一张页表基址）去写 `esi=769`，等于改坏第一张页表的高项，且 `eax` 也没加属性 / 没从第二张表算起。

### 学习要点

- PDE\[768\]（偏移 `0xc00`）与 PDE\[0\] 可指向同一张页表 → 低 1MB 与 `0xc0000000` 起 1MB 双映射
- PDE 最后一项指向页目录自身 → 才能用 `0xffc00000` / `0xfffff000` 公式访问任意 PTE/PDE
- 预挂的页表物理页，也应保证内容为 0，或在第一次使用前清零（`page_table_add` 在「新建 PDE」路径里会 `memset`）

---

## 建议的阅读顺序

1. `include/boot.inc` — 加载地址、入口、`PAGE_DIR_TABLE_POS`
2. `mbr/loader.asm` — `setup_page` → `kernel_init`（ELF + BSS）→ `jmp KERNEL_ENTRY_POINT`
3. `kernel/memory.c` — `mem_pool_init` → `malloc_page` → `page_table_add`
4. `readelf -lS build/kernel.bin` — 对照段与 `.bss`

---

## 尚未完成（后续可学）

- `0xb00` 处的总内存：`mem_init` 会读 `*(uint32_t *)0xb00`，但 loader 里目前没有完整的 E820 / ARDS 检测并写入该地址；内存数不对时，`ASSERT(all_mem != 0)` 或池大小会出问题
- `get_total_memory()` 仍是空实现
- `kernel/list.c` 仍有问题（错误头文件、`list_append` 递归等），默认 CMake 不编它（`-DOS_BUILD_LIST=ON` 可打开）

---

## 目录速览

```text
mbr/           MBR + loader
kernel/        内核 C / 汇编
include/       boot.inc、elf.inc 等
cmake/         写磁盘镜像脚本
CMakeLists.txt 一键编 boot / kernel / image / run
```
