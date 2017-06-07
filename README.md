# Y86-OS
csapp course project, semester 2, Fudan Univ.
## Brief introduction
YOS is a miniature of an computer system, aggregating concepts of CPU, cores, caches, virtual memories, processes, file systems, elfs and exception handlers, referring to CSAPP 2e. The key purpose of the project is to simulate a computer, running on an entire Y86-instruction set, with a shell and concurrent processes running on every CPU core(implemented by threads in C++)
## Feature list
1. Core: In Y86-OS, the minimum unit of a transaction on a core is a process(not a thread, as it is in reality). The kernel(can be regarded as an OS) determines process arrangement when there are multiple processes. The kernel always tries to make best use of all cores. The job on every core, is implemented with threads(required in the project).
2. CPU: In Y86-OS, a cpu may contain at most 4 cores(free to increase). Different cores on a single CPU shares the same L1-Cache, while different CPUs have their seperate L1-Cache. It's the only(but really annoying) difference.
3. Cache: For simplicity, Y86-OS includes only L1-Cache, while it have to synchronize caches on differnce CPUs so as to act with accordance with context switches. Refer to MSEI protocol to learn who to deal with the problem.
4. Physical Memory. With C++ Map<ADDR,VALUE> implementation. In YOS, the physcial memory is unique.
5. VM: In Y86-OS, a VM is equivalent with a process both in concepts and in implementation. It includes FD-set, page tables, its pid, and area structs(with can be called "kernel-space"). Everything in the kernel space is simulated directedly with C++, i.e. the page tables, don't exists in Y86-physical memory. However, everything in the user space, including .data, .text, .rodata, stack, are not directly managed by c++ and exists in Y86-Physical meomory. As for command-arguments, it do exist in Y86-Physical meomory, but it's behavior might be a little different. VM take charge of VM address translation, and segment protection(i.e. throw an exception when the Y86-program is going to write a .data(read-only) section). It also handlers circumstances of copy-on-writes.
6. Files: In order to use files in Y86-OS, you should first import a file(ascii or binary, limited to 10kb), and the Vnode-table, Open-File table in the kernel will manage the file-cursor position and other information of the file. Automatically, stdin and stdout is always opened by a process running on Y86.
7. ELF: A standard of y86-codes. A y86 code should include elf_header, data, rodata, text, bss, and section header table sections to run in YOS. It's very convenient to convert a normal y86-code to a y86-elf manually.
8. Syscalls: Y86-OS simluate 7 syscalls: exit, fork, open, read, write, waitpid, execve. The syscall-numbers and the meanings of each syscalls are equal to the ones in Linux. When you write Y86 codes, remind that the f_icode of SYSCALL is e0, and it needs Val_C(which, actually, makes no use). The syscall number should be stored in %eax, and other arguments in %ebx, %ecx, %edx in order. When a syscall happens, the f_stat of the kernel is set to SSYS(newly defined), and the kernel check the system call whem it pass through W-stage. The return value stores in %eax.
9. Signals: Only one signal: SIGSTOP. Implementation: The stat of the CPU is set to SSPD and it prepares to suspend the process running on itself. SSPD is very similar to SSYS, except that it passes no arguments.
8. Others: process arrangement, process suspending and terminating.

## example codes
1. init_full.ys: a complete shell y86-script in y86-elf format. Initially loaded when the simulator starts. As for program "abc", Use "{abc|arg1|arg2...}" to run "abc" forground or {abc|arg1|arg2..& to run it background.
2. print.ys: a simple script in y86-elf format. It prints a "hello" to stdout and terminates with syscall READ.
3. sum.ys: a fibonacci calculator in y86-elf format. Takes and integer argument and prints the ascii result(simplely add 0x30 to the integer result) on stdout.

