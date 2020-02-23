#include "asl.h"
#include "const.h"
#include "termprint.h"


#define HIDDEN static

// Array di SEMD disponibili
HIDDEN semd_t semd_table[MAXPROC];

// Lista dei SEMD liberi o inutilizzati
HIDDEN LIST_HEAD(semd_free);

// Lista dei SEMD attivi
HIDDEN LIST_HEAD(semd_busy);

// Ritorna il SEMD contenente target
HIDDEN semd_t* semdOf(struct list_head *target){
    return container_of(target, semd_t, s_next);
}

// Ritorna la chiave associata al SEMD contenente target
HIDDEN int* keyOf(struct list_head *target){
    return semdOf(target)->s_key;
}

// Restituisce il puntatore al SEMD nella semd_busy la cui
// chiave è pari a key. Se non esiste un elemento nella
// semd_busy con chiave uguale a key, viene restituito NULL.
semd_t* getSemd(int *key){
    struct list_head *tmp = semd_busy.next;
    semd_t *target;
    while (tmp != &semd_busy){
        target = semdOf(tmp);
        if (target->s_key == key) return target;
        tmp = tmp->next;
    }
    return NULL;
}

// Inizializza la lista di semafori liberi
void initASL(){
    for (int i = 0; i < MAXPROC; i++){
        semd_table[i].s_key = NULL;
        semd_table[i].s_next.next = NULL;
        semd_table[i].s_next.prev = NULL;
        INIT_LIST_HEAD( &semd_table[i].s_procQ ); //inizializza la sentinella della lista di PCB di SEMD_T
        list_add_tail(&semd_table[i].s_next, &semd_free);
    }
}

// Viene inserito il PCB puntato da p nella coda dei processi bloccati
// associata al SEMD con chiave key.
// Se il semaforo corrispondente non è presente nella semd_busy, alloca
// un nuovo SEMD dalla lista di quelli liberi (semd_free) e lo inserisce
// nella semd_busy, settando i campi in maniera opportuna.
// Se non è possibile allocare un nuovo SEMD perché la lista di quelli
// liberi è vuota, restituisce TRUE.
// In tutti gli altri casi, restituisce FALSE.
int insertBlocked(int *key, struct pcb_t *p){
    if(p == NULL || key == NULL) return ERROR;
    struct list_head *tmp_free, *tmp_busy; 
    struct semd_t* target;
    target = getSemd(key);
    if(target == NULL){
        if(list_empty(&(semd_free))) return TRUE;
        tmp_busy = semd_busy.next;
        while(tmp_busy != &semd_busy && (keyOf(tmp_busy) < key)) tmp_busy = tmp_busy->next;
        tmp_free = semd_free.next;
        list_del(tmp_free);
        target = container_of( tmp_free , struct semd_t , s_next );
        target->s_key = key;
        __list_add(tmp_free, tmp_busy->prev, tmp_busy);
    }
    p->p_semkey = key;
    list_add_tail(&(p->p_next), &(target->s_procQ));
    return FALSE;
}

//rimuove il semaforo q dalla lista di quelli attivi se non ha processi bloccati
HIDDEN inline void rem_idle_sem( struct semd_t* q){
	if( list_empty( &(q->s_procQ) )){
		list_del( &(q->s_next) );
		list_add( &(q->s_next), &semd_free );
	}
}

//Rimuove il primo processo dalla lista dei processi bloccati del semaforo puntato da semAdd  
struct pcb_t *removeBlocked(int *key){
    struct semd_t* semd_target;
    struct pcb_t* pcb_target ;
    semd_target = getSemd(key);
    if( semd_target != NULL ){
        pcb_target = removeProcQ(&(semd_target->s_procQ));
        rem_idle_sem(semd_target);
	return pcb_target ;	
    }
    return NULL;
}

//Rimuove il processo p dalla lista di processi bloccati del semaforo su cui è bloccato
//(se bloccato su un semaforo)
struct pcb_t *outBlocked(struct pcb_t *p){
    if( p == NULL ) return NULL;
    struct semd_t *sem ;
    if( p->p_semkey == NULL ) return NULL;
    sem = getSemd( p->p_semkey );
    if( sem == NULL ) return NULL ;
    p = outProcQ(  &(sem->s_procQ) , p ); 
    if(p!= NULL) rem_idle_sem( getSemd(p->p_semkey) );
    return p;
}

void outChildBlocked(struct pcb_t *p) {
    struct list_head *aux;

    if (p == NULL)
        return;

    aux = p->p_child.next;
    while (aux != &(p->p_child) && aux != NULL) {
        outChildBlocked(container_of(aux, struct pcb_t, p_sib));
        outBlocked(container_of(aux, struct pcb_t, p_sib));
        aux = aux->next;
    }
    outBlocked(p);
}
 

//Restituisce il primo processo bloccato sul semaforo puntato da semAdd senza rimuoverlo
struct pcb_t *headBlocked(int *semAdd){
    struct semd_t* q;
    q = getSemd( semAdd );
    if( q == NULL ) return NULL;
    return headProcQ( &q->s_procQ );
}
