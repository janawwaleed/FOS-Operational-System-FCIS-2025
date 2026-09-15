
# FOS — Fos Operating System 🖥️

A custom 32-bit operating system kernel built from scratch in C, simulated on the **Bochs x86 emulator**. FOS implements core OS concepts including memory management, process scheduling, virtual memory, and synchronization.

---


## ✨ Features

### 🧠 Memory Management
- **Kernel heap allocator** — dynamic memory allocation inside the kernel (`kheap.c`)
- **Dynamic allocator** — user-space heap management with custom malloc/free (`dynamic_allocator.c`, `uheap.c`)
- **Paging** — full virtual memory support with page tables and paging helpers
- **Shared memory** — inter-process shared memory manager
- **Page file manager** — disk-backed virtual memory (swap)
- **Working set manager** — page replacement policies (Clock, Optimal)

### ⚙️ Process Management
- **User environments** — process creation, loading, and management
- **Priority-based scheduler** — Round Robin with priority levels (`priority_manager.c`)
- **System calls** — user-to-kernel interface (`syscall.c`)
- **Fault handler** — page fault and exception handling

### 🔒 Synchronization
- **Semaphores** — kernel and user-space semaphore implementation
- **Spin locks** — busy-wait locking for critical sections
- **Concurrency utilities** — sleep locks and concurrency primitives

### 🧪 User Programs & Tests
- Memory allocation tests (malloc, free, custom fit)
- Page replacement tests (Clock, Optimal algorithms)
- Shared memory and protection tests
- Priority scheduling tests
- Semaphore and concurrency tests

---

## 🛠️ Tech Stack

| Layer | Details |
|---|---|
| Language | C + x86 Assembly |
| Emulator | Bochs x86 PC Emulator |
| Build System | GNU Make |
| IDE | Eclipse (CDT) |
| Architecture | x86 32-bit protected mode |

---

## 🗂️ Project Structure

```
FOS_PROJECT_2025_TEMPLATE/
├── kern/
│   ├── mem/        # Memory management (kheap, paging, shared memory, working set)
│   ├── proc/       # Process/environment management & scheduler
│   ├── trap/       # Interrupt & fault handling, syscalls
│   ├── disk/       # Page file (swap) manager
│   ├── cons/       # Console & printf
│   └── tests/      # Kernel-level test suites
├── lib/            # User-space libraries (heap, syscalls, semaphores, strings)
├── user/           # User programs and test cases
├── inc/            # Shared headers (MMU, types, syscall defs)
├── boot/           # Bootloader
├── conf/           # Build configuration
└── GNUmakefile     # Build system
```

---

## 🚀 Getting Started

### Prerequisites
- Bochs x86 emulator
- GCC cross-compiler (i386)
- GNU Make
- Eclipse CDT (optional, for IDE support)

### Build & Run
```bash
# Build the OS
make

# Run in Bochs emulator
make qemu
# or double-click bochscon.bat on Windows
```

---

## 📄 License

Built as an academic OS course project (2025). Based on the FOS template framework.
