"""Correction barométrique du taux de muons : le flux de muons au sol
varie avec la pression atmosphérique (plus de pression = plus
d'atmosphère au-dessus du détecteur = plus d'absorption). C'est un
effet réel, mesurable, documenté depuis des décennies en physique des
rayons cosmiques.

Le coefficient barométrique β dépend du site, de l'altitude et de la
réponse spécifique du détecteur — ce n'est pas une constante universelle
à coder en dur. Ce module fournit le modèle et l'outil d'ajustement ;
la valeur de β doit être déterminée empiriquement à partir des propres
données du détecteur (voir README).
"""
from __future__ import annotations

import math
from dataclasses import dataclass


@dataclass
class BarometricFit:
    beta_percent_per_hpa: float
    reference_rate_hz: float
    reference_pressure_hpa: float


def corrected_rate(rate_hz: float, pressure_hpa: float, fit: BarometricFit) -> float:
    """Normalise un taux mesuré à la pression de référence, pour
    comparer des mesures prises à des moments/pressions différents sur
    un pied d'égalité — utile pour détecter une vraie variation du flux
    (ex. événement solaire) plutôt qu'un artefact météo."""
    delta_p = pressure_hpa - fit.reference_pressure_hpa
    correction = math.exp((fit.beta_percent_per_hpa / 100.0) * delta_p)
    return rate_hz * correction


def fit_barometric_coefficient(rates_hz: list[float], pressures_hpa: list[float]) -> BarometricFit:
    """Ajuste β à partir d'une série de mesures (taux, pression)
    appariées, par régression linéaire sur ln(taux) vs pression — la
    relation taux~exp(-β·P) devient linéaire une fois le log pris,
    d'où le choix de cette méthode plutôt qu'un ajustement non linéaire
    direct."""
    if len(rates_hz) != len(pressures_hpa):
        raise ValueError("rates_hz et pressures_hpa doivent avoir la même longueur")
    if len(rates_hz) < 2:
        raise ValueError("au moins 2 points de mesure sont nécessaires pour un ajustement")

    n = len(rates_hz)
    log_rates = [math.log(r) for r in rates_hz]
    mean_p = sum(pressures_hpa) / n
    mean_log_r = sum(log_rates) / n

    numerator = sum((p - mean_p) * (lr - mean_log_r) for p, lr in zip(pressures_hpa, log_rates))
    denominator = sum((p - mean_p) ** 2 for p in pressures_hpa)
    if denominator == 0:
        raise ValueError("toutes les pressions sont identiques : régression impossible")

    slope = numerator / denominator  # d(ln rate)/dP
    beta_percent_per_hpa = -slope * 100.0

    return BarometricFit(
        beta_percent_per_hpa=beta_percent_per_hpa,
        reference_rate_hz=math.exp(mean_log_r),
        reference_pressure_hpa=mean_p,
    )
