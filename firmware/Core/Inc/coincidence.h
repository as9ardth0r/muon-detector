/**
 * coincidence.h — détection de coïncidence entre deux voies de
 * détection. Indépendant du matériel, miroir de
 * sim/muon_sim/coincidence.py — voir
 * tests/test_coincidence_c_matches_python.py.
 *
 * Version embarquée : les temps sont en microsecondes entières
 * (uint32_t), pas en secondes flottantes — c'est ce que fournit
 * naturellement un timer STM32 en input capture. La fenêtre de
 * coïncidence est donc aussi en microsecondes.
 */
#ifndef MUON_COINCIDENCE_H
#define MUON_COINCIDENCE_H

#include <stdint.h>
#include <stddef.h>

/* Balayage à deux pointeurs sur deux tableaux TRIÉS de temps
 * d'arrivée (µs). Écrit les indices appariés dans
 * matched_a_idx/matched_b_idx (taille max `max_matches`), retourne le
 * nombre de coïncidences trouvées (peut être < max_matches). */
uint32_t mc_find_coincidences(const uint32_t *times_a, uint32_t n_a,
                               const uint32_t *times_b, uint32_t n_b,
                               uint32_t window_us,
                               uint32_t *matched_a_idx, uint32_t *matched_b_idx,
                               uint32_t max_matches);

#endif /* MUON_COINCIDENCE_H */
