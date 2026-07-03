#include "interrupt.h"
#include "utils.h"
#include "scheduler.h"
#include "syscall.h"

int getDeviceNr(unsigned bitmap){
    int i;
    for (i = 0; i < 8; i++){
        if (bitmap == 1) return i;
        else bitmap = bitmap >> 1;
    }
    return -1;
}

void intHandler(){

    // TIME CONTROLLER
    current->time[USERTIME] = current->time[USERTIME] + (getTODLO() - last_user_switch);
    last_kernel_switch = getTODLO();

    unsigned cause;
    state_t* old_state = (state_t*) INT_OLDAREA;

    #ifdef TARGET_UMPS
    cause = old_state->cause;
    #elif defined(TARGET_UARM)
    old_state->pc = old_state->pc - 4;
    cause = old_state->CP15_Cause;
    
    #endif
    copyState(old_state, &current->p_s);

    if(CAUSE_IP_GET(cause, TIMER_USED))
        timer_on = 0;
    else if(CAUSE_IP_GET(cause, INT_DISK)){
        dtpHandler(INT_DISK);
    } else if(CAUSE_IP_GET(cause, INT_TAPE)){
        dtpHandler(INT_TAPE);
    } else if(CAUSE_IP_GET(cause, INT_UNUSED)){
        dtpHandler(INT_UNUSED);
    } else if(CAUSE_IP_GET(cause, INT_PRINTER)){
        dtpHandler(INT_PRINTER);
    } else if(CAUSE_IP_GET(cause, INT_TERMINAL)){
        terminalHandler();
    } else {
        PANIC(); /* sollevato interrupt non riconosciuto */
    }
    
    if (current != NULL) current->time[KERNELTIME] = current->time[KERNELTIME] + (getTODLO() - last_kernel_switch);
    scheduler();
}

void dtpHandler(int type){
    int i, status, device_nr;
    dtpreg_t* dev;
    memaddr* interrupt_bitmap = (memaddr*) CDEV_BITMAP_ADDR(type);

    if ((device_nr = getDeviceNr(*interrupt_bitmap)) < 0) PANIC();
    else dev = (dtpreg_t*) DEV_REG_ADDR(type, device_nr);
    i = DEV_PER_INT * (type - 3) + device_nr;
    status = dev->status;
    dev->command = CMD_ACK;
    if(dev_sem[i] < 0){
        pcb_t *free = headBlocked(&dev_sem[i]);
        verhogen(&dev_sem[i]);
        if (free != NULL) free->p_s.RET_VAL = status;
        dev_response[i] = 0;
    } else {
        dev_response[i] = status;
    }
}

void terminalHandler(){
    int i = -1, status, device_nr;
    termreg_t* term;
    memaddr* interrupt_bitmap = (memaddr*) CDEV_BITMAP_ADDR(INT_TERMINAL);

    if ((device_nr = getDeviceNr(*interrupt_bitmap)) < 0) PANIC();
    else term = (termreg_t*) DEV_REG_ADDR(INT_TERMINAL, device_nr);

    if ((term->recv_status & TERM_STATUS_MASK) == ST_TRANS_RECV){
        i = DEV_PER_INT * (INT_TERMINAL - 3) + device_nr;
        status = term->recv_status;
        term->recv_command = CMD_ACK;
    }
    else if ((term->transm_status & TERM_STATUS_MASK) == ST_TRANS_RECV){
        i = DEV_PER_INT * (INT_TERMINAL - 3 + 1) + device_nr;
        status = term->transm_status;
        term->transm_command = CMD_ACK;
    } else {
        PANIC();
    }
    if (i < 0) return;
    if(dev_sem[i] < 0){
        pcb_t *free = headBlocked(&dev_sem[i]);
        verhogen(&dev_sem[i]);
        if (free != NULL) free->p_s.RET_VAL = status;
        dev_response[i] = 0;
    } else {
        dev_response[i] = status;
    }
}