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
int bus_mutex;
int id_tache_A; // Nécessaire pour que C puisse changer la priorité de A

void busy_wait(int iterations) {
    for(volatile int i = 0; i < iterations; ++i);
}

/* ========================================================================= *
 * PHASE 1 : TEST DU MUTEX RÉCURSIF (Générique)
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

    while(1) { delay(1000); } // S'endort définitivement pour ne pas gêner la suite
}

/* ========================================================================= *
 * PHASE 2 : SCENARIO PATHFINDER (INVERSION ET HERITAGE DE PRIORITE)
 * ========================================================================= */
TACHE tacheMeteo(void *arg) {
    delay(20);
    printf("\n\n--- PHASE 2 : SCENARIO PATHFINDER (HERITAGE DE PRIORITE) ---");
    printf("\n[METEO-6] Prend le bus_mutex.");
    m_acquire(bus_mutex);

    // Simulation d'un temps d'exécution long sur le bus
    for (int i = 0; i < 20; i++) {
        busy_wait(20000000);
        printf("\n[METEO] (Prio actuelle: %d) Transmet les donnees sur le bus 1553...", noyau_get_t_prio(noyau_get_tc()));
    }

    printf("\n[METEO-6] Transmission terminee. Relache le bus_mutex.");
    m_release(bus_mutex);
    while(1) { delay(1000); }
}

TACHE tacheRadio(void *arg) {
    delay(22);
    printf("\n[RADIO-3] Arrive et tente de monopoliser le CPU (Inversion de priorite) !");

    // CORRECTION ICI : Une boucle finie pour éviter de déborder sur la Phase 3
    for(int i = 0; i < 15; i++) {
        busy_wait(20000000);
        printf("\n[RADIO] Execute les communications...");
    }
    printf("\n[RADIO-3] Fin des communications. S'endort.");
    while(1) { delay(1000); }
}

TACHE tacheDistribDonnees(void *arg) {
    delay(25);
    printf("\n[DISTRIB_DONNEES-1] Arrive et bloque sur le bus_mutex detenu par METEO.");

    m_acquire(bus_mutex);
    printf("\n[DISTRIB_DONNEES-1] Mutex acquis ! METEO a bien ete boostee pour me le rendre.");
    m_release(bus_mutex);

    while(1) { delay(1000); }
}

/* ========================================================================= *
 * PHASE 3 : TEST MANUEL DES TOURNIQUETS (Générique)
 * ========================================================================= */
TACHE tacheA(void *arg) {
    delay(80);
    printf("\n\n--- PHASE 3 : TEST DES TOURNIQUETS (A, B, C) ---");
    printf("\n[A-6] Demarre. Boucle infinie...\n");
    while(1) {
        busy_wait(20000000);
        printf("A");
    }
}

TACHE tacheB(void *arg) {
    delay(90);
    printf("\n[B-3] Preempte A. Boucle infinie...\n");
    while(1) {
        busy_wait(20000000);
        printf("B");
    }
}

TACHE tacheC(void *arg) {
    delay(100);
    printf("\n[C-1] Preempte B. Booste A a 1. Boucle infinie...\n");

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
    bus_mutex = m_create();

    // Lancement Phase 1
    active(cree(tacheRecursif, 5, NULL));

    // Lancement Phase 2 (Pathfinder)
    active(cree(tacheDistribDonnees, 1, NULL));
    active(cree(tacheRadio,          3, NULL));
    active(cree(tacheMeteo,          6, NULL));

    // Lancement Phase 3
    id_tache_A = cree(tacheA, 6, NULL);
    active(id_tache_A);
    active(cree(tacheB, 3, NULL));
    active(cree(tacheC, 1, NULL));

    while (1);
}

int main() {
    usart_init(115200);
    CLEAR_SCREEN(1);
    start(tachedefond);

    while (1);
    return(0);
}
