#include "utils.h"
#include "syscall.h"

// Coda dei processi attivi
LIST_HEAD(ready_queue);

struct list_head* getQueue(){
    return &ready_queue;
}

void breakpoint(){
}

void idle(){
    while(TRUE){
    }
}

void initNewArea(memaddr new_area, memaddr handler){
    state_t* new_state = (state_t*) new_area;
    STST(new_state);
    #ifdef TARGET_UMPS
    new_state->status = new_state->status & ~(STATUS_VMc | STATUS_VMp | STATUS_VMo); /* Virtual memory OFF */
    new_state->status = new_state->status & ~(STATUS_KUc | STATUS_KUp | STATUS_KUo); /* Kernel mode ON */
    new_state->status = new_state->status & ~STATUS_TE; /* Timer OFF */
    new_state->status = new_state->status & ~(STATUS_IEc | STATUS_IEp | STATUS_IEo); /* Interrupt mascherati */
    new_state->reg_sp = RAMTOP; /* Stack pointer a RAM_TOP */
    new_state->pc_epc = handler; /* Imposta pc all'handler */
    #elif defined(TARGET_UARM)
    new_state->CP15_Control = CP15_DISABLE_VM(new_state->CP15_Control); /* Virtual memory OFF */
    new_state->cpsr = STATUS_ALL_INT_DISABLE(new_state->cpsr) | STATUS_SYS_MODE; /* Kernel mode ON, timer e interrupt OFF */
    new_state->sp = RAMTOP; /* Stack pointer a RAM_TOP */
    new_state->pc = handler; /* Imposta pc all'handler */
    #endif
}

void initProcess(memaddr entry_point, unsigned priority){
    state_t p_s;
    STST(&p_s);
    // Imposta lo status del processo
    #ifdef TARGET_UMPS
    p_s.status = p_s.status & ~(STATUS_VMc | STATUS_VMp | STATUS_VMo); /* Virtual memory OFF */
    p_s.status = p_s.status & ~(STATUS_KUc | STATUS_KUp | STATUS_KUo); /* Kernel mode ON */
    p_s.status = p_s.status | STATUS_TE; /* Timer ON */
    p_s.status = p_s.status | STATUS_IEc | STATUS_IEp | STATUS_IEo; /* Interrupt abilitati */
    p_s.status = p_s.status | STATUS_IM_MASK; /* Attiva tutti gli interrupt */
    p_s.reg_sp = RAMTOP - FRAMESIZE; /* TODO: Imposta RAM adeguata */
    p_s.pc_epc = entry_point; /* Imposta pc all'entry point */
    #elif defined(TARGET_UARM)
    p_s.CP15_Control = CP15_DISABLE_VM(p_s.CP15_Control); /* Virtual memory OFF */
    p_s.cpsr = STATUS_ALL_INT_ENABLE(p_s.cpsr) | STATUS_SYS_MODE; /* Interrupt, timer e kernel mode ON */
    p_s.sp = RAMTOP - FRAMESIZE; /* TODO: Imposta RAM adeguata */
    p_s.pc = entry_point; /* Imposta pc all'entry point */
    #endif
    createProcess(&p_s, priority, 0);
}

void copyState(state_t* src, state_t* dest){
    #ifdef TARGET_UMPS
    for(int i = 0; i < 31; i++) dest->gpr[i] = src->gpr[i];
    dest->pc_epc = src->pc_epc;
    dest->status = src->status;
    dest->entry_hi = src->entry_hi;
    dest->cause = src->cause;
    dest->hi = src->hi;
    dest->lo = src->lo;
    #elif defined(TARGET_UARM)
    dest->a1 = src->a1;
    dest->a2 = src->a2;
    dest->a3 = src->a3;
    dest->a4 = src->a4;
    dest->v1 = src->v1;
    dest->v2 = src->v2;
    dest->v3 = src->v3;
    dest->v4 = src->v4;
    dest->v5 = src->v5;
    dest->v6 = src->v6;
    dest->sl = src->sl;
    dest->fp = src->fp;
    dest->ip = src->ip;
    dest->sp = src->sp;
    dest->lr = src->lr;
    dest->pc = src->pc;
    dest->cpsr = src->cpsr;
    dest->CP15_Control = src->CP15_Control;
    dest->CP15_EntryHi = src->CP15_EntryHi;
    dest->CP15_Cause = src->CP15_Cause;
    dest->TOD_Hi = src->TOD_Hi;
    dest->TOD_Low = src->TOD_Low;
    #endif
}

unsigned deviceIndex(unsigned *reg, int subdevice){
    unsigned dev_line,dev_pos,address;
    address = (unsigned)reg;
    if (address >= FIRST_ADDR_TERMINAL){
        address -= FIRST_ADDR_TERMINAL;
        if (subdevice) dev_line = INT_TERMINAL;
        else dev_line = INT_TERMINAL + 1;
    } else if (address >= FIRST_ADDR_PRINTER){
        address -= FIRST_ADDR_PRINTER;
        dev_line = INT_PRINTER;
    } else if (address >= FIRST_ADDR_UNUSED){
        address -= FIRST_ADDR_UNUSED;
        dev_line = INT_UNUSED;
    } else if (address >= FIRST_ADDR_TAPE){
        address -= FIRST_ADDR_TAPE;
        dev_line = INT_TAPE;
    } else if (address >= FIRST_ADDR_DISK){
        address -= FIRST_ADDR_DISK;
        dev_line = INT_DISK;
    } else {
        PANIC();
    }
    dev_pos = address / 16;
    return (dev_line - 3) * 8 + dev_pos;
}

void *memset(void *s, int c, size_t n){
    unsigned char* p = s;
    while(n--)
        *p++ = (unsigned char)c;
    return s;
}