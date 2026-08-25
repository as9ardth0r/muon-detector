import math

import pytest

from muon_sim.barometric import BarometricFit, corrected_rate, fit_barometric_coefficient


def test_corrected_rate_at_reference_pressure_is_unchanged():
    fit = BarometricFit(beta_percent_per_hpa=0.75, reference_rate_hz=2.0, reference_pressure_hpa=1013.0)
    assert corrected_rate(2.0, 1013.0, fit) == pytest.approx(2.0)


def test_corrected_rate_higher_pressure_measurement_is_scaled_up():
    # Une mesure prise à pression plus élevée a été davantage atténuée
    # par l'atmosphère (plus de matière au-dessus du détecteur) : la
    # ramener à la pression de référence doit donc l'augmenter — pas la
    # diminuer. C'est l'inverse qui semblait intuitif à première vue et
    # qui a été la source du bug de signe corrigé dans corrected_rate().
    fit = BarometricFit(beta_percent_per_hpa=0.75, reference_rate_hz=2.0, reference_pressure_hpa=1013.0)
    corrected_high_p = corrected_rate(2.0, 1023.0, fit)
    corrected_low_p = corrected_rate(2.0, 1003.0, fit)
    assert corrected_high_p > 2.0 > corrected_low_p


def test_fit_recovers_known_coefficient_from_synthetic_data():
    """Test de cohérence fort : génère des données synthétiques avec un
    β connu via le modèle direct (taux qui DÉCROÎT avec la pression —
    convention physique standard, plus de pression = plus d'absorption
    = taux mesuré plus faible), puis vérifie que l'ajustement retrouve
    bien ce β — valide l'algorithme de régression lui-même."""
    true_beta = 0.72  # %/hPa, dans la fourchette généralement citée pour ce type d'effet
    reference_rate = 3.0
    reference_pressure = 1013.25

    pressures = [990.0, 1000.0, 1010.0, 1013.25, 1020.0, 1030.0]
    rates = [
        reference_rate * math.exp(-(true_beta / 100.0) * (p - reference_pressure))
        for p in pressures
    ]

    fit = fit_barometric_coefficient(rates, pressures)
    assert fit.beta_percent_per_hpa == pytest.approx(true_beta, abs=1e-6)

    # fit.reference_rate_hz est le taux évalué à la pression MOYENNE des
    # données d'entrée (comportement standard d'une régression linéaire
    # en log — la droite ajustée passe par (moyenne(P), moyenne(ln taux))),
    # pas à reference_pressure choisi arbitrairement dans la génération
    # synthétique. On compare donc au modèle direct évalué à cette même
    # pression moyenne, pas à la constante `reference_rate`.
    expected_rate_at_mean_p = reference_rate * math.exp(
        -(true_beta / 100.0) * (fit.reference_pressure_hpa - reference_pressure)
    )
    assert fit.reference_rate_hz == pytest.approx(expected_rate_at_mean_p, rel=1e-3)


def test_fit_rejects_mismatched_lengths():
    with pytest.raises(ValueError):
        fit_barometric_coefficient([1.0, 2.0], [1000.0])


def test_fit_rejects_single_point():
    with pytest.raises(ValueError):
        fit_barometric_coefficient([1.0], [1000.0])


def test_fit_rejects_identical_pressures():
    with pytest.raises(ValueError):
        fit_barometric_coefficient([1.0, 2.0, 1.5], [1000.0, 1000.0, 1000.0])
