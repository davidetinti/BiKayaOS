#include "utils.h" // per altre funzioni
#include "scheduler.h" // per scheduler
#include "interrupt.h" // per inthandler
#include "syscall.h" // per sys_handler

int main(){

    // Inizializza le New Area
    initNewArea((memaddr) SYSBK_NEWAREA, (memaddr) sysHandler);
    initNewArea((memaddr) PGMTRAP_NEWAREA, (memaddr) trapHandler);
    initNewArea((memaddr) TLB_NEWAREA, (memaddr) tlbHandler);
    initNewArea((memaddr) INT_NEWAREA, (memaddr) int_handler);

    // Inizializza la pcb free
    initPcbs();
    
    // Alloca 3 pcb
    first_t = allocPcb();
    second_t = allocPcb();
    third_t = allocPcb();

    // Istanzia i 3 processi di test nei pcb
    createProcess((memaddr) test1, first_t, FIRST_PRIORITY);
    createProcess((memaddr) test2, second_t, SECOND_PRIORITY);
    createProcess((memaddr) test3, third_t, THIRD_PRIORITY);

    current_process = NULL;

    // Avvia gestione processi
    scheduler();
    return 0;
}