/*----------------------------------------------------------------------------*
 * fichier : noyau_test_prio.c                                                *
 * programme de test illustrant l'héritage de priorité avec Mutex             *
 *----------------------------------------------------------------------------*/
/*
#include <stdint.h>
#include <stdlib.h>

#include "hwsupport/uart.h"
#include "kernel/noyau_prio.h"
#include "kernel/delay.h"
#include "kernel/mutex.h"
#include "io/serialio.h"
#include "io/terminal.h"

// Variable globale pour notre mutex partagé
int my_mutex;

// Fonction utilitaire pour simuler un calcul long sans rendre la main au noyau
void busy_wait(int iterations) {
    for(volatile int i = 0; i < iterations; ++i);
}

// -------------------------------------------------------------------
// TACHE LOW (Priorité 6 - Basse)
// -------------------------------------------------------------------
TACHE tacheLow(void *arg) {
    delay(5); // S'exécute en premier
    printf("\n[LOW] Demarrage. Prio actuelle : %d", noyau_get_t_prio(noyau_get_tc()));

    printf("\n[LOW] Demande le mutex...");
    m_acquire(my_mutex);
    printf("\n[LOW] Mutex acquis ! Je travaille...");

    // Travail critique qui va être interrompu par Med, puis par High
    for (int i = 0; i < 4; i++) {
        printf("\n[LOW] ...travail critique (Ma prio est %d)", noyau_get_t_prio(noyau_get_tc()));
        busy_wait(50000000);
    }

    printf("\n[LOW] Relache le mutex.");
    m_release(my_mutex);

    printf("\n[LOW] Fin. Prio de retour : %d", noyau_get_t_prio(noyau_get_tc()));
    while(1) { delay(100); }
}

// -------------------------------------------------------------------
// TACHE MED (Priorité 3 - Moyenne)
// -------------------------------------------------------------------
TACHE tacheMed(void *arg) {
    delay(10); // S'exécute après que Low ait pris le mutex
    printf("\n[MED] Demarrage. Prio : %d. Je monopolise le CPU !", noyau_get_t_prio(noyau_get_tc()));

    // Travail non critique très long qui causerait l'inversion sans héritage
    for (int i = 0; i < 5; i++) {
        printf("\n[MED] ...travail standard");
        busy_wait(100000000);
    }

    printf("\n[MED] Fin du travail standard.");
    while(1) { delay(100); }
}

// -------------------------------------------------------------------
// TACHE HIGH (Priorité 1 - Haute)
// -------------------------------------------------------------------
TACHE tacheHigh(void *arg) {
    delay(15); // S'exécute en dernier
    printf("\n[HIGH] Demarrage. Prio : %d", noyau_get_t_prio(noyau_get_tc()));

    printf("\n[HIGH] Demande le mutex (bloque et boost LOW)...");
    m_acquire(my_mutex);
    printf("\n[HIGH] Mutex acquis ! Je travaille enfin...");

    busy_wait(50000000);

    printf("\n[HIGH] Relache le mutex.");
    m_release(my_mutex);

    printf("\n[HIGH] Fin.");
    while(1) { delay(100); }
}

// -------------------------------------------------------------------
// TACHE DE FOND
// -------------------------------------------------------------------
TACHE tachedefond(void *arg) {
    SET_CURSOR_POSITION(3,1);
    puts("------> EXEC tache de fond");

    // 1. Initialisation de l'API Mutex
    m_init();

    // 2. Création du Mutex de test
    my_mutex = m_create();
    printf("Mutex cree avec ID : %d\n", my_mutex);

    // 3. Création des tâches : 1 (plus forte), 3 (moyenne), 6 (plus faible)
    active(cree(tacheHigh, 1, NULL));
    active(cree(tacheMed,  3, NULL));
    active(cree(tacheLow,  6, NULL));

    // Maintient la tâche de fond en vie
    while (!usart_read()) {
        delay(1000);
    }
}

int main() {
    usart_init(115200);
    CLEAR_SCREEN(1);
    puts("Test inversion de priorite et mutex");
    start(tachedefond);
    return(0);
}
*/

/*
#include <stdint.h>
#include <stdlib.h>
#include "hwsupport/uart.h"
#include "kernel/noyau_prio.h"
#include "kernel/delay.h"
#include "kernel/mutex.h"
#include "io/serialio.h"
#include "io/terminal.h"

int my_mutex;

void busy_wait(int iterations) {
    for(volatile int i = 0; i < iterations; ++i);
}

TACHE tacheLow(void *arg) {
    delay(5); // LOW démarre au tic 5
    printf("\n[LOW-6] Prend le mutex.");
    m_acquire(my_mutex);

    // Au lieu de faire calculer le CPU bêtement, on dit à l'OS de nous rendormir.
    // LOW va dormir jusqu'au tic 25.
    // Pendant ce temps, MED (tic 10) et HIGH (tic 15) vont se réveiller,
    // voir que le mutex est pris, et s'entasser dans la FIFO !
    delay(20);

    printf("\n[LOW-6] Relache le mutex.");
    m_release(my_mutex); // Ici, MED et HIGH seront BIEN dans la file !
    while(1) { delay(100); }
}

TACHE tacheMed(void *arg) {
    delay(10); // Arrive en PREMIER dans la file d'attente du Mutex
    printf("\n[MED-3] Je veux le mutex (j'arrive en 1er dans la file)");
    m_acquire(my_mutex);
    printf("\n[MED-3] Mutex acquis ! C'est anormal si HIGH attendait.");
    m_release(my_mutex);
    while(1) { delay(100); }
}

TACHE tacheHigh(void *arg) {
    delay(15); // Arrive en DEUXIEME dans la file d'attente du Mutex
    printf("\n[HIGH-1] Je veux le mutex (j'arrive en 2eme dans la file)");
    m_acquire(my_mutex);
    printf("\n[HIGH-1] Mutex acquis ! L'ordre de priorite est respecte.");
    m_release(my_mutex);
    while(1) { delay(100); }
}

TACHE tachedefond(void *arg) {
    m_init();
    my_mutex = m_create();
    active(cree(tacheHigh, 1, NULL));
    active(cree(tacheMed,  3, NULL));
    active(cree(tacheLow,  6, NULL));
    while (!usart_read()) { delay(1000); }
}

int main() {
    usart_init(115200);
    start(tachedefond);
    return 0;
}

*/

/*----------------------------------------------------------------------------*
 * fichier : noyau_test_prio.c                                                *
 * Test COMPLET du cahier des charges (Recursif, Héritage, File, Tourniquets) *
 *----------------------------------------------------------------------------*/

#include <stdint.h>
#include <stdlib.h>
#include "hwsupport/uart.h"
#include "kernel/noyau_prio.h"
#include "kernel/delay.h"
#include "kernel/mutex.h"
#include "io/serialio.h"
#include "io/terminal.h"

// Variables globales
int mutex_rec;
int mutex_dyn;
int id_tache_A; // Nécessaire pour que C puisse changer la priorité de A

// Fonction pour ralentir l'affichage dans les boucles infinies
void busy_wait(int iterations) {
    for(volatile int i = 0; i < iterations; ++i);
}

/* ========================================================================= *
 * PHASE 1 : TEST DU MUTEX RÉCURSIF
 * ========================================================================= */
TACHE tacheRecursif(void *arg) {
    delay(5); // Démarre au tic 5
    printf("\n\n--- PHASE 1 : MUTEX RECURSIF ---");
    printf("\n[REC] Je prends le mutex une 1ere fois.");
    m_acquire(mutex_rec);

    printf("\n[REC] Je prends le MEME mutex une 2eme fois (recursif).");
    m_acquire(mutex_rec);

    printf("\n[REC] Pas de blocage ! Je relache la 1ere fois.");
    m_release(mutex_rec);

    printf("\n[REC] Je relache la 2eme fois.");
    m_release(mutex_rec);

    while(1) { delay(1000); } // S'endort définitivement
}

/* ========================================================================= *
 * PHASE 2 : HÉRITAGE DE PRIORITÉ ET ORDRE DE LA FILE D'ATTENTE
 * ========================================================================= */
TACHE tacheLow(void *arg) {
    delay(20); // Démarre au tic 20
    printf("\n\n--- PHASE 2 : HERITAGE ET FILE D'ATTENTE ---");
    printf("\n[LOW-6] Prend le mutex.");
    m_acquire(mutex_dyn);

    // LOW s'endort avec le mutex jusqu'au tic 60.
    // Pendant ce temps, MED et HIGH vont arriver et se bloquer.
    delay(40);

    printf("\n[LOW-6] Je me reveille (Ma prio actuelle est %d). Je relache le mutex.", noyau_get_t_prio(noyau_get_tc()));
    m_release(mutex_dyn);
    while(1) { delay(1000); }
}

TACHE tacheMed(void *arg) {
    delay(30);
    printf("\n[MED-3] Arrive en 1er dans la file du mutex.");
    m_acquire(mutex_dyn);
    printf("\n[MED-3] Mutex acquis !");
    m_release(mutex_dyn);
    while(1) { delay(1000); }
}

TACHE tacheHigh(void *arg) {
    delay(40);
    printf("\n[HIGH-1] Arrive en 2eme dans la file du mutex.");
    m_acquire(mutex_dyn);
    printf("\n[HIGH-1] Mutex acquis en PREMIER grace au tri de la file !");
    m_release(mutex_dyn);
    while(1) { delay(1000); }
}

/* ========================================================================= *
 * PHASE 3 : TEST MANUEL DES TOURNIQUETS (Boucles Infinies)
 * ========================================================================= */
TACHE tacheA(void *arg) {
    delay(80);
    printf("\n\n--- PHASE 3 : TEST DES TOURNIQUETS (A, B, C) ---");
    printf("\n[A-6] Demarre. Boucle infinie...");
    while(1) {
        busy_wait(20000000);
        printf("A");
    }
}

TACHE tacheB(void *arg) {
    delay(90);
    printf("\n[B-3] Preempte A. Boucle infinie...");
    while(1) {
        busy_wait(20000000);
        printf("B");
    }
}

TACHE tacheC(void *arg) {
    delay(100);
    printf("\n[C-1] Preempte B. Booste A a 1. Boucle infinie...");

    // On booste la priorité de A au niveau de C (niveau 1)
    noyau_set_t_prio(id_tache_A, 1);

    while(1) {
        busy_wait(20000000);
        printf("C");
    }
}

/* ========================================================================= *
 * TACHE DE FOND ET MAIN
 * ========================================================================= */
TACHE tachedefond(void *arg) {
    SET_CURSOR_POSITION(3,1);
    puts("------> DEMARRAGE DES TESTS\n");

    m_init();
    mutex_rec = m_create();
    mutex_dyn = m_create();

    // Lancement Phase 1
    active(cree(tacheRecursif, 5, NULL));

    // Lancement Phase 2
    active(cree(tacheHigh, 1, NULL));
    active(cree(tacheMed,  3, NULL));
    active(cree(tacheLow,  6, NULL));

    // Lancement Phase 3 (On sauvegarde l'ID de A)
    id_tache_A = cree(tacheA, 6, NULL);
    active(id_tache_A);
    active(cree(tacheB, 3, NULL));
    active(cree(tacheC, 1, NULL));

    while (!usart_read()) {
        delay(1000); // Ne fait rien, laisse l'OS tourner
    }
}

int main() {
    usart_init(115200);
    CLEAR_SCREEN(1);
    start(tachedefond);
    return(0);
}
