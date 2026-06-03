/*----------------------------------------------------------------------------*
 * fichier : noyau_file_prio_dyn.c                                            *
 * ordonnanceur à priorités dynamiques géré par liste circulaire dans les TCB *
 *----------------------------------------------------------------------------*/

#include <stdint.h>
#include "noyau_file_prio.h"
#include "noyau_prio.h"
#include "../io/serialio.h"

// Tableau des têtes de tourniquet pour chaque niveau de priorité
static uint16_t _tetes[MAX_PRIO];

void file_init(void) {
    for (int i = 0; i < MAX_PRIO; i++) {
        _tetes[i] = MAX_TACHES_NOYAU; // Indique un tourniquet vide
    }
}

void file_ajoute(uint16_t id) {
    NOYAU_TCB *tcb = noyau_get_p_tcb(id);
    int prio = tcb->prio_actuelle;

    if (_tetes[prio] == MAX_TACHES_NOYAU) {
        // File vide : devient la seule tâche (elle boucle sur elle-même)
        tcb->id_suivant = id;
        _tetes[prio] = id;
    } else {
        // Ajout en tête du tourniquet
        uint16_t queue = _tetes[prio];
        NOYAU_TCB *tcb_queue = noyau_get_p_tcb(queue);

        // On cherche le dernier élément pour fermer la liste circulaire
        while (tcb_queue->id_suivant != _tetes[prio]) {
            queue = tcb_queue->id_suivant;
            tcb_queue = noyau_get_p_tcb(queue);
        }

        tcb->id_suivant = _tetes[prio]; // Pointe vers l'ancienne tête
        tcb_queue->id_suivant = id;     // Le dernier pointe vers la nouvelle tête
        _tetes[prio] = id;              // Devient la nouvelle tête
    }
}

void file_retire(uint16_t id) {
    NOYAU_TCB *tcb = noyau_get_p_tcb(id);
    int prio = tcb->prio_actuelle;

    if (_tetes[prio] == MAX_TACHES_NOYAU) return; // Sécurité

    if (tcb->id_suivant == id) {
        // C'était la seule tâche
        _tetes[prio] = MAX_TACHES_NOYAU;
    } else {
        // On cherche la tâche précédente
        uint16_t prev = _tetes[prio];
        NOYAU_TCB *tcb_prev = noyau_get_p_tcb(prev);
        while (tcb_prev->id_suivant != id) {
            prev = tcb_prev->id_suivant;
            tcb_prev = noyau_get_p_tcb(prev);
        }

        tcb_prev->id_suivant = tcb->id_suivant; // On court-circuite la tâche à retirer

        if (_tetes[prio] == id) {
            _tetes[prio] = tcb->id_suivant; // Si c'était la tête, la suivante devient la tête
        }
    }
    tcb->id_suivant = MAX_TACHES_NOYAU; // Nettoyage
}

uint16_t file_suivant(void) {
    // Parcours des priorités (0 = la plus élevée)
    for (int prio = 0; prio < MAX_PRIO; ++prio) {
        if (_tetes[prio] != MAX_TACHES_NOYAU) {
            uint16_t id = _tetes[prio];
            NOYAU_TCB *tcb = noyau_get_p_tcb(id);
            _tetes[prio] = tcb->id_suivant; // La tâche suivante devient la nouvelle tête du tourniquet
            return id;
        }
    }
    return MAX_TACHES_NOYAU; // Rien à exécuter
}

void file_affiche_queue(void) {
    for (int i=0; i < MAX_PRIO; i++){
         printf("_tetes[%d] = %d\n", i, _tetes[i]);
    }
}

void file_affiche(void) {
    // Non requis strictement pour le fonctionnement, gardé vide pour respecter le prototype
}
