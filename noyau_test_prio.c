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

// Fon
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
 * PHASE 2 : TEST DE L'INVERSION ET DE L'HERITAGE DE PRIORITE
 * ========================================================================= */
TACHE tacheLow(void *arg) {
    delay(20);
    printf("\n\n--- PHASE 2 : HERITAGE DE PRIORITE ---");
    printf("\n[LOW-6] Prend le mutex.");
    m_acquire(mutex_dyn);

    // On augmente drastiquement la boucle pour s'assurer que LOW
    // se fasse rattraper par le réveil de MED et HIGH
    for (int i = 0; i < 20; i++) {
        busy_wait(20000000);
        // L'affichage de la priorité ici va te prouver l'héritage !
        printf("\n[LOW] (Prio %d) Travaille dans la zone critique...", noyau_get_t_prio(noyau_get_tc()));
    }

    printf("\n[LOW-6] Zone critique terminee. Je relache le mutex.");
    m_release(mutex_dyn);
    while(1) { delay(1000); }
}

TACHE tacheMed(void *arg) {
    delay(22); // <-- MED se réveille presque tout de suite après LOW
    printf("\n[MED-3] Arrive et tente de monopoliser le CPU !");

    while(1) {
        busy_wait(20000000);
        printf("\n[MED] Execute...");
    }
}

TACHE tacheHigh(void *arg) {
    delay(25); // <-- HIGH se réveille juste après, pendant que MED monopolise
    printf("\n[HIGH-1] Arrive et bloque sur le mutex detenu par LOW.");

    m_acquire(mutex_dyn);
    printf("\n[HIGH-1] Mutex acquis ! LOW a bien ete boostee pour me le rendre.");
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

    while (1);
}

int main() {
    usart_init(115200);
    CLEAR_SCREEN(1);
    start(tachedefond);

    while (1);
    return(0);
}
