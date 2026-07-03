#include "listx.h"
#include "pcb.h"
#include "const.h"

#define HIDDEN static

// Array di PCB disponibili
HIDDEN pcb_t pcb_table[MAXPROC];

// Lista dei PCB liberi o inutilizzati
HIDDEN LIST_HEAD(pcb_free);

void initPcbs(void){
   for (int i = 0; i < MAXPROC; i++){
       list_add(&pcb_table[i].p_next, &pcb_free);
   }
}

void freePcb(pcb_t *p){
    if (p != NULL){
        list_add_tail(&p->p_next, &pcb_free);
    }
}

pcb_t *allocPcb(void){
    if(list_empty(&pcb_free)) return NULL;
    pcb_t *deleted;
    deleted = container_of(pcb_free.next, pcb_t, p_next);
    list_del(pcb_free.next);
    deleted->p_parent = NULL;
    deleted->p_semkey = NULL;
    deleted->priority = 0;
    INIT_LIST_HEAD(&deleted->p_child);
    INIT_LIST_HEAD(&deleted->p_sib);
    return deleted;
}

void mkEmptyProcQ(struct list_head *head){
    INIT_LIST_HEAD(head);
}

int emptyProcQ(struct list_head *head){
    return (list_empty(head));
}

void insertProcQ(struct list_head *q, pcb_t *p){ 
    if (q == NULL || p == NULL) return;
    struct list_head* iterator, *old;
    pcb_t *target;
    old = q;
    iterator = q->next;
    while (iterator != q) {
        target = container_of(iterator, pcb_t, p_next);
        if (target->priority < p->priority) break;
        old = iterator;
        iterator = iterator->next;
    }
    if (iterator == q) list_add_tail(&(p->p_next), q);
    else list_add(&(p->p_next), old);
}

pcb_t *headProcQ(struct list_head *head){
    pcb_t *target;
    target = container_of(head->next, pcb_t, p_next);
    if(emptyProcQ(head)) return NULL;
    return target;
}

pcb_t *removeProcQ(struct list_head *head){
    pcb_t *target; 
    target = headProcQ(head);
    list_del(head->next);
    return target;
}

pcb_t *outProcQ(struct list_head *head, pcb_t *p){
    if(emptyProcQ(head)){
        return NULL;
    } else {
        if(p == NULL){
            return NULL;
        } else {
            pcb_t *target;
            struct list_head *tmp;
            tmp = head->next; 
            target = container_of(tmp, pcb_t, p_next);
            while(target != p && tmp != head){
                tmp = tmp->next ;
                target = container_of(tmp, pcb_t, p_next);
            }
            if(target == p){
                list_del(&(target->p_next));
                return target;
            } else {
                return NULL;
            }
        }
    }
}

int emptyChild(pcb_t *this){
    if (this != NULL){
        if (this->p_child.next == NULL){
            return TRUE;
        } else {
            return list_empty(&this->p_child);
        }
    }
    return ERROR;
}
 
void insertChild(pcb_t *prnt, pcb_t *p){
    if (prnt != NULL && p != NULL){
        if (emptyChild(prnt)) INIT_LIST_HEAD(&prnt->p_child);
        list_add_tail(&p->p_sib, &prnt->p_child);
        p->p_parent = prnt;
    }
}

struct pcb_t *outChild(struct pcb_t *p) {
    if ( p == NULL || p->p_parent == NULL ) return NULL; 
    list_del( &(p->p_sib) );   
    p->p_parent = NULL;
    return p;
}

pcb_t* removeChild(pcb_t *p){ 
    if(p == NULL) return NULL;
    if(emptyChild(p)) return NULL;
    pcb_t *target = container_of(p->p_child.next, pcb_t, p_sib);
    list_del(&target->p_sib);
    target->p_parent = NULL;
    return target;
}