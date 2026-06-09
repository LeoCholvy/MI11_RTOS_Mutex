/*----------------------------------------------------------------------------*
 * fichier : noyau_file_prio_dyn.c                                            *
 * ordonnanceur à priorités dynamiques géré par liste circulaire dans les TCB *
 *----------------------------------------------------------------------------*/

/* * ============================================================================
 * JUSTIFICATION DE LA MODIFICATION DE L'ORDONNANCEUR (V. STATIQUE -> V. DYNAMIQUE)
 * ============================================================================
 * L'ancienne implémentation (noyau_file_prio.c) gérait les tourniquets via
 * un tableau statique 2D (_file[MAX_PRIO][MAX_TACHES_FILE]). Le défaut majeur
 * de cette approche était l'association de l'identité de la tâche et sa
 * priorité : l'ID d'une tâche était codé en dur.
 * * Pour implémenter l'héritage de priorité (solution au problème Pathfinder),
 * il est impératif de pouvoir modifier temporairement la priorité d'une tâche
 * sans modifier son ID.
 * * Cette nouvelle implémentation découple l'ID de la priorité :
 * 1. Les tourniquets ne sont plus gérés par un tableau, mais par
 * un chaînage dynamique directement intégré dans le TCB de la tâche via
 * les champs 'prio_actuelle' et 'id_suivant'.
 * 2. Le tableau '_tetes' se contente de pointer vers la première tâche de la
 * liste circulaire pour chaque niveau de priorité.
 * Cela permet de retirer une tâche de son tourniquet actuel et de l'insérer
 * dans un autre pour modifier sa priorité à la volée.
 * ============================================================================
 */

#include <stdint.h>
#include "noyau_file_prio.h"
#include "noyau_prio.h"
#include "../io/serialio.h"

// Tableau stockant l'ID de la tâche en tête de file pour chaque niveau de priorité
static uint16_t _tetes[MAX_PRIO];

/*
 * Initialise l'ordonnanceur dynamique.
 * Marque toutes les têtes de file comme vides (MAX_TACHES_NOYAU).
 */
void file_init(void) {
    for (int i = 0; i < MAX_PRIO; i++) {
        _tetes[i] = MAX_TACHES_NOYAU; // Indique un tourniquet vide
    }
}

/*
 * Ajoute une tâche dans le tourniquet correspondant à sa priorité actuelle.
 * Gère le chaînage circulaire directement via les TCB.
 */
void file_ajoute(uint16_t id) {
    NOYAU_TCB *tcb = noyau_get_p_tcb(id);
    int prio = tcb->prio_actuelle;

    if (_tetes[prio] == MAX_TACHES_NOYAU) {
        // Cas 1 : La file est vide. La tâche devient la seule du tourniquet
        // et boucle sur elle-même.
        tcb->id_suivant = id;
        _tetes[prio] = id;
    } else {
        // Cas 2 : Ajout en tête du tourniquet existant.
        // Il faut parcourir la liste circulaire pour trouver le dernier élément
        // afin de le faire pointer vers cette nouvelle tête.
        uint16_t queue = _tetes[prio];
        NOYAU_TCB *tcb_queue = noyau_get_p_tcb(queue);

        // On cherche le dernier élément (celui qui pointe vers l'ancienne tête)
        while (tcb_queue->id_suivant != _tetes[prio]) {
            queue = tcb_queue->id_suivant;
            tcb_queue = noyau_get_p_tcb(queue);
        }

        // Insertion et mise à jour des pointeurs
        tcb->id_suivant = _tetes[prio]; // La nouvelle tâche pointe vers l'ancienne tête
        tcb_queue->id_suivant = id;     // Le dernier élément pointe vers la nouvelle tête
        _tetes[prio] = id;              // Mise à jour du pointeur de tête du noyau
    }
}

/*
 * Retire une tâche de son tourniquet (utilisé lors des changements de priorité
 * ou de l'endormissement d'une tâche).
 */
void file_retire(uint16_t id) {
    NOYAU_TCB *tcb = noyau_get_p_tcb(id);
    int prio = tcb->prio_actuelle;

    if (_tetes[prio] == MAX_TACHES_NOYAU) return; // Sécurité : file déjà vide
    if (tcb->id_suivant == MAX_TACHES_NOYAU) return; // Sécurité : tâche non chaînée

    if (tcb->id_suivant == id) {
        // Cas 1 : C'était la seule tâche dans ce tourniquet.
        // La file devient vide.
        _tetes[prio] = MAX_TACHES_NOYAU;
    } else {
        // Cas 2 : Il y a d'autres tâches. On doit trouver la tâche précédente
        // pour réparer le chaînage circulaire.
        uint16_t prev = _tetes[prio];
        NOYAU_TCB *tcb_prev = noyau_get_p_tcb(prev);

        while (tcb_prev->id_suivant != id) {
            prev = tcb_prev->id_suivant;
            tcb_prev = noyau_get_p_tcb(prev);
        }

        // La tâche précédente saute la tâche actuelle pour pointer sur la suivante
        tcb_prev->id_suivant = tcb->id_suivant;

        // Si la tâche qu'on retire était la tête du tourniquet,
        // la tête est décalée sur la tâche suivante.
        if (_tetes[prio] == id) {
            _tetes[prio] = tcb->id_suivant;
        }
    }

    // Nettoyage du pointeur de la tâche retirée
    tcb->id_suivant = MAX_TACHES_NOYAU;
}

/*
 * Élit la prochaine tâche à exécuter.
 * Parcourt les niveaux de priorité de la plus forte (0) à la plus faible.
 * Effectue la rotation du tourniquet (Round-Robin) pour le niveau de priorité choisi.
 */
uint16_t file_suivant(void) {
    // Parcours strict par priorité décroissante (0 = la plus élevée)
    for (int prio = 0; prio < MAX_PRIO; ++prio) {
        if (_tetes[prio] != MAX_TACHES_NOYAU) {
            // Une tâche prête a été trouvée pour ce niveau de priorité
            uint16_t id = _tetes[prio];
            NOYAU_TCB *tcb = noyau_get_p_tcb(id);

            // Rotation du tourniquet : l'élément suivant devient la nouvelle tête.
            // Cela garantit l'équité (Round-Robin) entre les tâches de même priorité.
            _tetes[prio] = tcb->id_suivant;

            return id;
        }
    }
    // Si toutes les files sont vides
    return MAX_TACHES_NOYAU;
}

/*
 * Affiche l'état des têtes de files (fonction de débogage).
 */
void file_affiche_queue(void) {
    for (int i=0; i < MAX_PRIO; i++){
         printf("_tetes[%d] = %d\n", i, _tetes[i]);
    }
}

/*
 * Fonction conservée uniquement pour respecter le prototypage de l'ancienne version.
 * L'affichage complexe du tableau 2D n'a plus de sens ici.
 */
void file_affiche(void) {

}
