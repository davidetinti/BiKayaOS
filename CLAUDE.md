# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

BiKayaOS is an educational OS kernel developed for the University of Bologna OS course. It targets two emulated architectures — **ARM** (via [uARM](https://github.com/mellotanica/uARM)) and **MIPS** (via [uMPS](https://github.com/tjonjic/umps)) — and is structured as four progressive implementation phases.

## Build Commands

Each phase (`phase0/`, `phase1/`, `phase1.5/`, `phase2/`) has its own independent Makefile. Run all commands from inside the target phase directory.

```bash
# Build for MIPS (mipsel-linux-gnu toolchain)
make umps

# Build for MIPS64 (mipsisa64r6el-linux-gnuabi64 toolchain)
make umps1

# Build for ARM (arm-none-eabi toolchain)
make uarm

# Clean build artifacts
make clean
```

Build outputs go to `build/umps/` or `build/uarm/`. The MIPS build produces `kernel.core.umps` (via `umps2-elf2umps`); ARM produces a raw ELF `kernel`.

**Required toolchains:**
- MIPS: `mipsel-linux-gnu-gcc`, `mipsel-linux-gnu-ld`, `umps2-elf2umps`
- ARM: `arm-none-eabi-gcc`, `arm-none-eabi-ld`

## Running Tests

There is no automated test runner. Each phase ships a test file that is compiled into the kernel binary itself and runs at boot in the emulator:

| Phase | Test file |
|-------|-----------|
| phase1 | `src/p1test_bikaya_v0.c` |
| phase1.5 | `src/p1.5test_bikaya_v0.c` |
| phase2 | `src/p2test_bikaya.c` |

To run tests, build the phase and load the resulting kernel into the corresponding emulator (uMPS or uARM).

## Architecture

### Phase Progression

- **phase0** — Bare-metal terminal and printer I/O only; no process management.
- **phase1** — PCB and ASL data structures: process queues, process trees, semaphore blocking queues. No scheduler or interrupts.
- **phase1.5** — Adds interrupt handler stubs, a scheduler stub, and syscall dispatcher scaffolding on top of phase1 data structures.
- **phase2** — Full kernel: preemptive scheduler with priority aging, 8 syscalls, device interrupt handling, TLB/trap exception handlers, and CPU time accounting.

### Key Data Structures

Defined in `include/types_bikaya.h`:

- **`pcb_t`** — Process Control Block. Uses Linux-style `list_head` for the run queue (`p_next`) and tree relationships (`p_child`, `p_sib`). Holds the saved CPU `state_t`, `priority`/`original_priority`, a semaphore key pointer `p_semkey`, three time counters (`time[3]`: start, user, kernel), and six alternative handler state pointers (`proc_area[6]`: new/old × sysbk/tlb/trap).

- **`semd_t`** — Semaphore descriptor. Holds a key pointer `s_key` and a `list_head s_procQ` of blocked PCBs.

- **`listx.h`** — Linux kernel-style intrusive circular doubly-linked list. All queues use `container_of` / `list_entry` to recover the containing struct. This pattern is used everywhere — understand it before modifying queue logic.

### Architecture Abstraction

All architecture-specific differences are isolated in `include/utils.h` and `include/const_bikaya.h` behind `#ifdef TARGET_UMPS` / `#ifdef TARGET_UARM` guards. Key abstractions:

- **Register names**: `ARG1`–`ARG4` (syscall arguments), `RET_VAL` (return register) — map to `reg_a0`/`a1` etc. per architecture.
- **Exception areas**: `INT_OLDAREA`, `SYSBK_NEWAREA`, etc. — fixed memory addresses where the hardware saves/restores CPU state on exception.
- **Timer access**: `getTODLO()`, `setTIMER()` — emulator-specific device-mapped I/O.
- **Device base addresses**: `FIRST_ADDR_DISK`, `FIRST_ADDR_TAPE`, `FIRST_ADDR_PRINTER`, `FIRST_ADDR_TERMINAL` — differ between ARM and MIPS.

Never add architecture-specific code outside these guarded sections.

### Global State (phase1.5 and phase2)

Declared in `include/utils.h`:

- `pcb_t *current` — the currently running process.
- `int dev_sem[MAXDEV]` — semaphore values for each device (disk×8, tape×8, network×8, printer×8, terminal-recv×8, terminal-send×8 = 48 total).
- `int dev_response[MAXDEV]` — device status codes saved by interrupt handlers, read by `WAITIO`.
- `unsigned last_user_switch`, `last_kernel_switch` — timestamps for CPU time accounting.
- `unsigned int timer_on` — whether the preemption timer is active.

### Syscall Numbers (phase2)

| # | Name | Description |
|---|------|-------------|
| 1 | `GETCPUTIME` | Return user/kernel/wall-clock time for current process |
| 2 | `CREATEPROCESS` | Fork a child PCB |
| 3 | `TERMINATEPROCESS` | Kill a process and its entire subtree |
| 4 | `VERHOGEN` | Semaphore V (signal) |
| 5 | `PASSEREN` | Semaphore P (wait) |
| 6 | `WAITIO` | Block on device I/O completion |
| 7 | `SPECPASSUP` | Register alternative exception handlers for SYSBK/TLB/TRAP |
| 8 | `GETPID` | Return PID of current process and its parent |

### Exception Vector Initialization

`initNewArea(memaddr new_area, memaddr handler)` in `utils.c` writes a handler address into one of the eight fixed exception vector slots. Called during `main()` to wire up the four handlers: INT, TLB, PGMTRAP, SYSBK.

### Scheduler (phase2)

- Round-robin with preemption via hardware timer (`TIME_SLICE = 3000 * TIME_SCALE` microseconds).
- **Priority aging**: every time slice, all non-running ready processes have their `priority` incremented, preventing starvation. On dispatch, `priority` is reset to `original_priority`.
- Process queue is a `list_head` sorted by priority on insertion.

## Coding Conventions

- **Types**: `snake_case_t` suffix (e.g. `pcb_t`, `semd_t`).
- **Functions**: `camelCase` for public API (e.g. `insertProcQ`, `removeBlocked`), `snake_case` for internal helpers.
- **Macros/constants**: `UPPER_CASE`.
- **`HIDDEN`** is a `#define` alias for `static` — used for file-scope functions.
- **No dynamic allocation** — all PCBs and semaphore descriptors come from statically allocated pools (`pcb_table[MAXPROC]`, `semd_table[MAXSEMD]`). `MAXPROC` = 20.
- Comments are in Italian (original course language). New comments may be in English.
- Compiler flags include `-Wall -O0`; keep code warning-clean.
