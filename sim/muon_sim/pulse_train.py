"""Simulation de trains d'impulsions : les muons cosmiques arrivent
selon un processus de Poisson (événements indépendants, taux constant
en moyenne) — hypothèse standard en physique des rayons cosmiques pour
un détecteur de petite surface sur des échelles de temps courtes.
"""
from __future__ import annotations

import random


def generate_poisson_arrivals(rate_hz: float, duration_s: float,
                               rng: random.Random | None = None) -> list[float]:
    """Génère des temps d'arrivée (secondes, croissants) selon un
    processus de Poisson de taux `rate_hz`, sur une durée `duration_s`.
    Les intervalles entre événements suivent une loi exponentielle —
    propriété caractéristique d'un processus de Poisson."""
    rng = rng or random.Random()
    arrivals = []
    t = 0.0
    while t < duration_s:
        interval = rng.expovariate(rate_hz)
        t += interval
        if t < duration_s:
            arrivals.append(t)
    return arrivals


def add_timing_jitter(arrivals: list[float], jitter_std_s: float,
                       rng: random.Random | None = None) -> list[float]:
    """Ajoute un bruit de mesure gaussien à des temps d'arrivée —
    modélise l'incertitude de mesure d'un détecteur réel (temps de
    montée du SiPM, résolution du timer)."""
    rng = rng or random.Random()
    return sorted(t + rng.gauss(0.0, jitter_std_s) for t in arrivals)
