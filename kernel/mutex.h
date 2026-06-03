/*----------------------------------------------------------------------------*
 * fichier : mutex.h                                                          *
 * mécanisme de synchronisation par mutex avec gestion de priorité            *
 *----------------------------------------------------------------------------*/

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <stdint.h>
#include "fifo.h"

// Nombre maximum de mutex gérés par le système
#define MAX_MUTEX 16

/* Structure définissant un mutex */
typedef struct {
    int owner;          // Identité de la tâche qui détient le mutex (-1 si libre)
    int count;          // Compteur pour permettre les mutex récursifs
    FIFO file;          // File d'attente des tâches bloquées
} MUTEX;

/* Prototypes */
void m_init(void);
int  m_create(void);
void m_acquire(int id_mutex);
void m_release(int id_mutex);
void m_destroy(int id_mutex);

#endif // __MUTEX_H__
