/*----------------------------------------------------------------------------*
 * fichier : noyau_test_prio.c                                                *
 * programme de test illustrant l'héritage de priorité avec Mutex             *
 *----------------------------------------------------------------------------*/

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
