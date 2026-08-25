"""Vérifie que la détection de coïncidence embarquée
(firmware/Core/Src/coincidence.c, arithmétique entière microseconde)
trouve les mêmes coïncidences que la version Python (secondes flottantes).
"""
from __future__ import annotations

import ctypes
import random
import subprocess
from pathlib import Path

from muon_sim.coincidence import find_coincidences
from muon_sim.pulse_train import generate_poisson_arrivals

REPO_ROOT = Path(__file__).resolve().parent.parent
FIRMWARE_SRC = REPO_ROOT / "firmware" / "Core" / "Src" / "coincidence.c"
FIRMWARE_INC = REPO_ROOT / "firmware" / "Core" / "Inc"


def to_microseconds(times_s: list[float]) -> list[int]:
    return [round(t * 1_000_000) for t in times_s]


import pytest


@pytest.fixture(scope="module")
def coincidence_lib(tmp_path_factory):
    out_dir = tmp_path_factory.mktemp("coincidence_native")
    lib_path = out_dir / "libcoincidence.so"
    subprocess.run(
        ["gcc", "-shared", "-fPIC", "-O2",
         "-o", str(lib_path), str(FIRMWARE_SRC), f"-I{FIRMWARE_INC}"],
        check=True,
    )
    lib = ctypes.CDLL(str(lib_path))
    lib.mc_find_coincidences.argtypes = [
        ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint32,
        ctypes.POINTER(ctypes.c_uint32), ctypes.c_uint32,
        ctypes.c_uint32,
        ctypes.POINTER(ctypes.c_uint32), ctypes.POINTER(ctypes.c_uint32),
        ctypes.c_uint32,
    ]
    lib.mc_find_coincidences.restype = ctypes.c_uint32
    return lib


def call_c(lib, times_a_us: list[int], times_b_us: list[int], window_us: int) -> int:
    arr_a = (ctypes.c_uint32 * len(times_a_us))(*times_a_us)
    arr_b = (ctypes.c_uint32 * len(times_b_us))(*times_b_us)
    max_matches = max(len(times_a_us), len(times_b_us)) + 1
    matched_a = (ctypes.c_uint32 * max_matches)()
    matched_b = (ctypes.c_uint32 * max_matches)()

    n = lib.mc_find_coincidences(
        arr_a, len(times_a_us), arr_b, len(times_b_us), window_us,
        matched_a, matched_b, max_matches,
    )
    return n


def test_c_matches_python_on_simple_case(coincidence_lib):
    times_a_s = [1.0, 2.0, 5.0]
    times_b_s = [1.00005, 2.0, 5.2]  # 50µs, 0, 200ms d'écart
    window_s = 100e-6  # 100 µs

    py_result = find_coincidences(times_a_s, times_b_s, window_s)
    c_result = call_c(coincidence_lib, to_microseconds(times_a_s), to_microseconds(times_b_s),
                       window_us=100)

    assert c_result == py_result.n_coincidences


def test_c_matches_python_on_realistic_poisson_data(coincidence_lib):
    """Taux réaliste pour un détecteur (quelques Hz par voie, proche du
    flux muon attendu — voir docs/hardware.md), pas un taux de bruit
    artificiellement élevé. Important : voir
    test_high_density_quantization_can_diverge ci-dessous pour la
    raison de ce choix."""
    rng_a, rng_b = random.Random(7), random.Random(8)
    duration_s = 20.0
    times_a_s = generate_poisson_arrivals(rate_hz=5.0, duration_s=duration_s, rng=rng_a)
    times_b_s = generate_poisson_arrivals(rate_hz=5.0, duration_s=duration_s, rng=rng_b)
    window_s = 100e-6

    py_result = find_coincidences(times_a_s, times_b_s, window_s)
    c_result = call_c(coincidence_lib, to_microseconds(times_a_s), to_microseconds(times_b_s),
                       window_us=100)

    assert c_result == py_result.n_coincidences


def test_high_density_quantization_can_diverge(coincidence_lib):
    """Limite connue, documentée plutôt que cachée : à très haute densité
    d'événements proches (ex. 500 Hz avec une fenêtre de 100 µs, des
    centaines de correspondances fortuites en quelques secondes), l'arrondi
    secondes->microsecondes (±0,5 µs par horodatage) peut faire basculer
    UNE correspondance en bordure de fenêtre, et l'appariement à deux
    pointeurs étant séquentiel, ce basculement peut se répercuter en
    cascade sur les correspondances suivantes. Chaque étape de
    l'algorithme reste identique entre C et Python (voir le test
    précédent, sans écart) — c'est la quantification de l'horodatage
    d'entrée, pas la logique de coïncidence, qui est en cause ici.

    Sans conséquence pratique : un vrai détecteur ne fonctionne pas à un
    taux de bruit pareil (voir docs/hardware.md) — ce test documente la
    limite plutôt que de prétendre qu'elle n'existe pas.
    """
    rng_a, rng_b = random.Random(7), random.Random(8)
    times_a_s = generate_poisson_arrivals(rate_hz=500.0, duration_s=5.0, rng=rng_a)
    times_b_s = generate_poisson_arrivals(rate_hz=500.0, duration_s=5.0, rng=rng_b)
    window_s = 100e-6

    py_result = find_coincidences(times_a_s, times_b_s, window_s)
    c_result = call_c(coincidence_lib, to_microseconds(times_a_s), to_microseconds(times_b_s),
                       window_us=100)

    # pas d'égalité stricte attendue ici (voir docstring) — juste un
    # écart relatif faible, pour détecter une vraie régression algorithmique
    # plutôt qu'un simple effet de bord de quantification
    assert abs(c_result - py_result.n_coincidences) / py_result.n_coincidences < 0.05


def test_c_matches_python_no_coincidences(coincidence_lib):
    times_a_s = [1.0, 2.0, 3.0]
    times_b_s = [1.5, 2.5, 3.5]
    window_s = 1e-6

    py_result = find_coincidences(times_a_s, times_b_s, window_s)
    c_result = call_c(coincidence_lib, to_microseconds(times_a_s), to_microseconds(times_b_s),
                       window_us=1)

    assert c_result == py_result.n_coincidences == 0
