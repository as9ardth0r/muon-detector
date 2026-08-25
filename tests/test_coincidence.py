import random

import pytest

from muon_sim.coincidence import find_coincidences, poisson_uncertainty, rate_with_uncertainty
from muon_sim.pulse_train import generate_poisson_arrivals, add_timing_jitter


def test_exact_matching_timestamps_are_coincident():
    times_a = [1.0, 2.0, 3.0]
    times_b = [1.0, 2.0, 3.0]
    result = find_coincidences(times_a, times_b, window_s=1e-6)
    assert result.n_coincidences == 3


def test_timestamps_outside_window_are_not_coincident():
    times_a = [1.0]
    times_b = [1.0 + 1e-3]  # 1 ms d'écart
    result = find_coincidences(times_a, times_b, window_s=100e-9)  # fenêtre 100 ns
    assert result.n_coincidences == 0


def test_timestamps_just_inside_window_match():
    times_a = [1.0]
    times_b = [1.0 + 50e-9]
    result = find_coincidences(times_a, times_b, window_s=100e-9)
    assert result.n_coincidences == 1


def test_each_event_matched_at_most_once():
    """Un événement A proche de deux événements B ne doit compter
    qu'une seule coïncidence, pas deux — sinon le taux de coïncidence
    serait artificiellement gonflé."""
    times_a = [1.0]
    times_b = [1.0 - 10e-9, 1.0 + 10e-9]
    result = find_coincidences(times_a, times_b, window_s=50e-9)
    assert result.n_coincidences == 1


def test_uncorrelated_high_rate_noise_gives_low_coincidence_rate():
    """Le test le plus important : deux flux de bruit indépendants à
    haut débit ne doivent produire qu'un taux de coïncidence fortuite
    faible — c'est ce qui justifie physiquement d'utiliser la
    coïncidence pour réjeter le bruit électronique/radioactivité locale
    (non corrélée entre les deux voies) par rapport aux vrais muons
    cosmiques (qui traversent les deux scintillateurs quasi
    simultanément)."""
    rng_a, rng_b = random.Random(1), random.Random(2)
    duration_s = 10.0
    rate_hz = 1000.0  # bruit à haut débit, très supérieur au taux muon réel
    window_s = 100e-9  # 100 ns, typique d'un montage CosmicWatch-like

    times_a = generate_poisson_arrivals(rate_hz, duration_s, rng_a)
    times_b = generate_poisson_arrivals(rate_hz, duration_s, rng_b)
    result = find_coincidences(times_a, times_b, window_s)

    # taux de coïncidence fortuite attendu (formule standard) :
    # R_accidentelle ≈ 2 × window × rate_a × rate_b × duration
    expected_accidental = 2 * window_s * rate_hz * rate_hz * duration_s
    # tolérance large : c'est une estimation statistique, pas une valeur exacte
    assert result.n_coincidences < expected_accidental * 3 + 5


def test_correlated_events_are_all_caught_despite_jitter():
    """À l'inverse : des événements réellement corrélés (même train
    d'arrivée sur les deux voies, avec un peu de gigue de mesure)
    doivent presque tous être retrouvés en coïncidence."""
    rng = random.Random(42)
    true_arrivals = generate_poisson_arrivals(rate_hz=5.0, duration_s=20.0, rng=rng)

    times_a = add_timing_jitter(true_arrivals, jitter_std_s=5e-9, rng=random.Random(10))
    times_b = add_timing_jitter(true_arrivals, jitter_std_s=5e-9, rng=random.Random(20))

    result = find_coincidences(times_a, times_b, window_s=100e-9)
    # avec une gigue de 5 ns et une fenêtre de 100 ns, la quasi-totalité
    # des événements corrélés doit être retrouvée
    assert result.n_coincidences >= len(true_arrivals) * 0.95


def test_poisson_uncertainty():
    assert poisson_uncertainty(100) == pytest.approx(10.0)
    assert poisson_uncertainty(0) == 0.0


def test_rate_with_uncertainty():
    rate, unc = rate_with_uncertainty(n_counts=400, duration_s=100.0)
    assert rate == pytest.approx(4.0)
    assert unc == pytest.approx(0.2)  # sqrt(400)/100


def test_rate_with_uncertainty_rejects_zero_duration():
    with pytest.raises(ValueError):
        rate_with_uncertainty(10, 0.0)
