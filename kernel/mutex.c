/*----------------------------------------------------------------------------*
 * fichier : mutex.c                                                          *
 * implémentation du mutex avec héritage de priorité (priority inheritance)   *
 *----------------------------------------------------------------------------*/

#include "mutex.h"
#include "noyau_prio.h"

// Tableau statique stockant l'ensemble des mutex gérés par le système
static MUTEX _mutex[MAX_MUTEX];

/* * Initialise le tableau des mutex au démarrage du système.
 * Marque tous les mutex comme non créés (fifo_taille = -1).
 */
void m_init(void) {
    for (int i = 0; i < MAX_MUTEX; i++) {
        _mutex[i].owner = -1;
        _mutex[i].count = 0;
        fifo_init(&(_mutex[i].file));
        _mutex[i].file.fifo_taille = -1; // -1 indique que le slot est libre/non créé
    }
}

/* * Crée un nouveau mutex.
 * Parcourt le tableau pour trouver un emplacement libre, l'initialise et
 * retourne son identifiant. Retourne -1 si le nombre maximum est atteint.
 */
int m_create(void) {
    int id = -1;

    _lock_(); // Section critique : protection des structures du noyau
    for (int i = 0; i < MAX_MUTEX; i++) {
        if (_mutex[i].file.fifo_taille == -1) {
            _mutex[i].owner = -1;      // Aucun propriétaire initial
            _mutex[i].count = 0;       // Compteur de récursivité à 0
            fifo_init(&(_mutex[i].file)); // Initialise la file d'attente (taille = 0)
            id = i;
            break;
        }
    }
    _unlock_();

    return id;
}

/* * Tente d'acquérir le mutex spécifié.
 * Gère l'acquisition simple, la récursivité et l'héritage de priorité en cas de blocage.
 */
void m_acquire(int id_mutex) {
    int current_task = noyau_get_tc();

    _lock_(); // Section critique

    if (_mutex[id_mutex].owner == -1) {
        // Cas 1 : Le mutex est libre. La tâche courante en devient le propriétaire.
        _mutex[id_mutex].owner = current_task;
        _mutex[id_mutex].count = 1;

    } else if (_mutex[id_mutex].owner == current_task) {
        // Cas 2 : Le mutex est déjà détenu par la tâche courante (Mutex récursif).
        // On incrémente simplement le compteur.
        _mutex[id_mutex].count++;

    } else {
        // Cas 3 : Le mutex est détenu par une autre tâche.
        // La tâche courante doit se bloquer. Application de l'héritage de priorité.
        int owner = _mutex[id_mutex].owner;
        int my_prio = noyau_get_t_prio(current_task);
        int owner_prio = noyau_get_t_prio(owner);

        // Si la tâche courante a une priorité plus forte (valeur numérique plus faible)
        // que la tâche propriétaire, on booste la priorité du propriétaire.
        if (my_prio < owner_prio) {
            noyau_set_t_prio(owner, my_prio);
        }

        // Ajout de la tâche courante dans la file d'attente du mutex
        fifo_ajoute(&(_mutex[id_mutex].file), current_task);

        // Suspension de la tâche courante jusqu'à ce que le mutex soit libéré
        dort();
    }

    _unlock_();
}

/* * Libère le mutex spécifié.
 * Gère la décrémentation récursive, la restauration de priorité et le réveil
 * de la tâche en attente la plus prioritaire.
 */
void m_release(int id_mutex) {
    int current_task = noyau_get_tc();

    _lock_(); // Section critique

    // On s'assure que seule la tâche propriétaire peut relâcher le mutex
    if (_mutex[id_mutex].owner == current_task) {
        _mutex[id_mutex].count--;

        // Si le compteur atteint 0, le mutex est véritablement libéré
        if (_mutex[id_mutex].count == 0) {

            // 1. Restauration de la priorité de base de la tâche qui lâche le mutex
            // Cela annule l'effet d'un éventuel héritage de priorité ayant eu lieu dans m_acquire
            int base_prio = noyau_get_t_base_prio(current_task);
            if (noyau_get_t_prio(current_task) != base_prio) {
                noyau_set_t_prio(current_task, base_prio);
            }

            // 2. Élection de la prochaine tâche propriétaire parmi celles en attente
            int nb_waiting = _mutex[id_mutex].file.fifo_taille;

            if (nb_waiting > 0) {
                int best_task = -1;
                int best_prio = 255; // Initialisation à une priorité volontairement basse

                // Parcours complet de la FIFO pour trouver la tâche la plus prioritaire.
                // Le fonctionnement extrait chaque élément de la FIFO et le réinsère
                // à la fin s'il n'est pas l'élu final.
                for (int i = 0; i < nb_waiting; i++) {
                    uint8_t temp_task;
                    fifo_retire(&(_mutex[id_mutex].file), &temp_task);

                    int task_prio = noyau_get_t_prio(temp_task);

                    // Si on trouve une tâche avec une meilleure priorité (valeur plus petite)
                    if (task_prio < best_prio) {
                        // S'il y avait déjà un candidat retenu, il perd sa place
                        // et retourne dans la file d'attente
                        if (best_task != -1) {
                            fifo_ajoute(&(_mutex[id_mutex].file), best_task);
                        }

                        // Enregistrement du nouveau meilleur candidat
                        best_task = temp_task;
                        best_prio = task_prio;
                    } else {
                        // Ce n'est pas le meilleur candidat, on le replace immédiatement dans la FIFO
                        fifo_ajoute(&(_mutex[id_mutex].file), temp_task);
                    }
                }

                // À l'issue de la boucle, best_task contient la tâche la plus prioritaire.
                // Les autres tâches ont été réinsérées dans la FIFO.
                _mutex[id_mutex].owner = best_task;
                _mutex[id_mutex].count = 1;

                // On réveille l'heureux élu
                reveille(best_task);

            } else {
                // Aucune tâche n'était en attente, le mutex redevient totalement libre
                _mutex[id_mutex].owner = -1;
            }
        }
    }

    _unlock_();
}

/* * Détruit le mutex spécifié et libère son emplacement pour de futures créations.
 * La destruction n'est effective que si le mutex n'est actuellement détenu par personne.
 */
void m_destroy(int id_mutex) {
    _lock_(); // Section critique

    if (_mutex[id_mutex].owner == -1) {
        // Remise de la taille à -1 pour indiquer que l'emplacement est disponible
        _mutex[id_mutex].file.fifo_taille = -1;
    }

    _unlock_();
}
