#include "coincidence.h"

uint32_t mc_find_coincidences(const uint32_t *times_a, uint32_t n_a,
                               const uint32_t *times_b, uint32_t n_b,
                               uint32_t window_us,
                               uint32_t *matched_a_idx, uint32_t *matched_b_idx,
                               uint32_t max_matches) {
    uint32_t i = 0, j = 0, n_matches = 0;

    while (i < n_a && j < n_b) {
        uint32_t ta = times_a[i];
        uint32_t tb = times_b[j];

        /* différence absolue sans risque de sous-dépassement (unsigned) :
         * on compare d'abord, puis on soustrait dans le bon sens */
        uint32_t abs_dt;
        int a_is_later;
        if (ta >= tb) {
            abs_dt = ta - tb;
            a_is_later = 1;
        } else {
            abs_dt = tb - ta;
            a_is_later = 0;
        }

        if (abs_dt <= window_us) {
            if (n_matches < max_matches) {
                matched_a_idx[n_matches] = i;
                matched_b_idx[n_matches] = j;
            }
            n_matches++;
            i++;
            j++;
        } else if (a_is_later) {
            /* a est plus tardif que b : b doit avancer pour tenter de
             * rattraper la fenêtre de coïncidence */
            j++;
        } else {
            /* a est plus précoce que b : c'est a qui doit avancer */
            i++;
        }
    }

    return n_matches;
}
