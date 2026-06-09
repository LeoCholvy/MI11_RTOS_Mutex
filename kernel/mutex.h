/*----------------------------------------------------------------------------*
 * fichier : mutex.h                                                          *
 * mécanisme de synchronisation par mutex avec gestion de priorité            *
 *----------------------------------------------------------------------------*/

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <stdint.h>
#include "fifo.h"

/* Définition du nombre maximum de mutex pouvant être créés dans le système */
#define MAX_MUTEX 16

/* * Structure de données représentant un mutex.
 * Permet l'exclusion mutuelle, la récursivité et sert de base pour
 * l'implémentation de l'héritage de priorité.
 */
typedef struct {
    int owner;          // Identité de la tâche qui détient le mutex (-1 si le mutex est libre)
    int count;          // Compteur de prises (permet l'acquisition récursive par le même propriétaire)
    FIFO file;          // File d'attente (FIFO) des tâches bloquées en attente de ce mutex
} MUTEX;

/*----------------------------------------------------------------------------*
 * Prototypes des primitives de gestion des mutex                             *
 *----------------------------------------------------------------------------*/

/* * Initialise le tableau interne des mutex au démarrage du système.
 * Marque tous les mutex comme disponibles et non créés.
 */
void m_init(void);

/* * Crée un nouveau mutex et retourne son identifiant.
 * Retourne -1 si le nombre maximum de mutex (MAX_MUTEX) est atteint.
 */
int  m_create(void);

/* * Tente d'acquérir le mutex d'identifiant id_mutex.
 * Gère l'acquisition récursive et bloque la tâche courante si le mutex
 * est déjà détenu par une autre tâche, en appliquant l'héritage de priorité.
 */
void m_acquire(int id_mutex);

/* * Libère le mutex d'identifiant id_mutex.
 * Gère la décrémentation récursive, la restauration de la priorité de base
 * et le réveil de la tâche en attente la plus prioritaire.
 */
void m_release(int id_mutex);

/* * Détruit le mutex d'identifiant id_mutex.
 * Rend son emplacement disponible pour une future création (m_create).
 * La destruction est ignorée si le mutex est actuellement détenu.
 */
void m_destroy(int id_mutex);

#endif // __MUTEX_H__
