/*----------------------------------------------------------------------------*
 * fichier : mutex.c                                                          *
 * implémentation du mutex avec héritage de priorité (priority inheritance)   *
 *----------------------------------------------------------------------------*/

#include "mutex.h"
#include "noyau_prio.h"

// Tableau stockant l'ensemble des mutex
static MUTEX _mutex[MAX_MUTEX];

void m_init(void) {
    for (int i = 0; i < MAX_MUTEX; i++) {
        _mutex[i].owner = -1;
        _mutex[i].count = 0;
        fifo_init(&(_mutex[i].file));
        _mutex[i].file.fifo_taille = -1; // -1 indique que le mutex n'est pas créé
    }
}

int m_create(void) {
    int id = -1;
    _lock_();
    for (int i = 0; i < MAX_MUTEX; i++) {
        if (_mutex[i].file.fifo_taille == -1) {
            _mutex[i].owner = -1;
            _mutex[i].count = 0;
            fifo_init(&(_mutex[i].file)); // Remet la taille à 0
            id = i;
            break;
        }
    }
    _unlock_();
    return id;
}

void m_acquire(int id_mutex) {
    int current_task = noyau_get_tc();
    _lock_();

    if (_mutex[id_mutex].owner == -1) {
        // Le mutex est libre
        _mutex[id_mutex].owner = current_task;
        _mutex[id_mutex].count = 1;
    } else if (_mutex[id_mutex].owner == current_task) {
        // Mutex récursif (déjà détenu par la tâche courante)
        _mutex[id_mutex].count++;
    } else {
        // Le mutex est détenu par une autre tâche -> Héritage de priorité
        int owner = _mutex[id_mutex].owner;
        int my_prio = noyau_get_t_prio(current_task);
        int owner_prio = noyau_get_t_prio(owner);

        // Attention: on part du principe que 0 est la priorité la plus forte
        if (my_prio < owner_prio) {
            noyau_set_t_prio(owner, my_prio); // La tâche propriétaire hérite de notre priorité
        }

        // On se bloque dans la file d'attente
        fifo_ajoute(&(_mutex[id_mutex].file), current_task);
        dort(); // Suspend la tâche courante
    }
    _unlock_();
}

void m_release(int id_mutex) {
    int current_task = noyau_get_tc();
    _lock_();

    if (_mutex[id_mutex].owner == current_task) {
        _mutex[id_mutex].count--;

        if (_mutex[id_mutex].count == 0) {
            // 1. Restauration de la priorité de base de la tâche qui lâche le mutex
            int base_prio = noyau_get_t_base_prio(current_task);
            if (noyau_get_t_prio(current_task) != base_prio) {
                noyau_set_t_prio(current_task, base_prio);
            }

            // 2. Recherche de la tâche la plus prioritaire dans la FIFO
            int nb_waiting = _mutex[id_mutex].file.fifo_taille;

            if (nb_waiting > 0) {
                int best_task = -1;
                int best_prio = 255; // Une priorité volontairement très mauvaise au départ

                // On vide la file un par un pour trouver le meilleur
                for (int i = 0; i < nb_waiting; i++) {
                    uint8_t temp_task;
                    fifo_retire(&(_mutex[id_mutex].file), &temp_task);

                    int task_prio = noyau_get_t_prio(temp_task);

                    if (task_prio < best_prio) { // (Rappel : 0 est la meilleure priorité)
                        // Si on avait un "meilleur" précédent, il a perdu, on le remet dans la file
                        if (best_task != -1) {
                            fifo_ajoute(&(_mutex[id_mutex].file), best_task);
                        }
                        // On sauvegarde le nouveau champion
                        best_task = temp_task;
                        best_prio = task_prio;
                    } else {
                        // Pas le meilleur, on le remet à la fin de la file immédiatement
                        fifo_ajoute(&(_mutex[id_mutex].file), temp_task);
                    }
                }

                // À la fin de cette boucle, best_task est le grand gagnant (HIGH)
                // et la FIFO contient toujours les autres (MED).
                _mutex[id_mutex].owner = best_task;
                _mutex[id_mutex].count = 1;
                reveille(best_task);

            } else {
                // Personne n'attendait le mutex
                _mutex[id_mutex].owner = -1;
            }
        }
    }
    _unlock_();
}
void m_destroy(int id_mutex) {
    _lock_();
    if (_mutex[id_mutex].owner == -1) {
        _mutex[id_mutex].file.fifo_taille = -1; // Marque comme détruit
    }
    _unlock_();
}
