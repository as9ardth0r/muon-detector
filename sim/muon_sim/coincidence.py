"""Détection de coïncidence entre deux voies de détection : un
événement compte comme un vrai muon traversant les deux scintillateurs
(donc probablement un vrai rayon cosmique, pas du bruit électronique ou
une désintégration radioactive locale) si les deux voies déclenchent
dans une fenêtre temporelle courte. Miroir exact de
firmware/Core/Src/coincidence.c — voir
tests/test_coincidence_c_matches_python.py.
"""
from __future__ import annotations

from dataclasses import dataclass


@dataclass
class CoincidenceResult:
    n_coincidences: int
    coincident_times_a: list[float]
    coincident_times_b: list[float]


def find_coincidences(times_a: list[float], times_b: list[float],
                       window_s: float) -> CoincidenceResult:
    """Balayage à deux pointeurs sur deux listes triées de temps
    d'arrivée : complexité O(n+m), pas O(n×m) — important pour un flux
    continu de données sur plusieurs heures d'acquisition.

    Un événement de la voie A est apparié au plus une fois avec un
    événement de la voie B (le plus proche dans la fenêtre), pour éviter
    qu'un même événement B ne compte plusieurs coïncidences avec des
    événements A rapprochés."""
    i, j = 0, 0
    matched_a, matched_b = [], []

    while i < len(times_a) and j < len(times_b):
        dt = times_a[i] - times_b[j]

        if abs(dt) <= window_s:
            matched_a.append(times_a[i])
            matched_b.append(times_b[j])
            i += 1
            j += 1
        elif dt < 0:
            i += 1
        else:
            j += 1

    return CoincidenceResult(
        n_coincidences=len(matched_a),
        coincident_times_a=matched_a,
        coincident_times_b=matched_b,
    )


def poisson_uncertainty(n_counts: int) -> float:
    """Incertitude statistique standard sur un comptage de Poisson :
    √N. Base de toute barre d'erreur en physique des rayons cosmiques."""
    return n_counts ** 0.5


def rate_with_uncertainty(n_counts: int, duration_s: float) -> tuple[float, float]:
    """Taux de comptage (Hz) et son incertitude à 1σ, propagée depuis
    l'incertitude de Poisson sur N."""
    if duration_s <= 0:
        raise ValueError("duration_s doit être positif")
    rate = n_counts / duration_s
    rate_uncertainty = poisson_uncertainty(n_counts) / duration_s
    return rate, rate_uncertainty
