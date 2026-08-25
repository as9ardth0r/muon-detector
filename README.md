# muon-detector

### Détecteur de muons cosmiques à coïncidence — STM32F405, dérivé du CosmicWatch (MIT)

Deux voies scintillateur+SiPM en coïncidence pour distinguer les vrais rayons
cosmiques du bruit électronique, avec correction barométrique du taux
mesuré. Relie directement le projet MadGraph (physique des particules) au
travail embarqué des autres dépôts.

## Ce qui est réel et vérifié (19 tests)

| Brique | Vérifié comment |
|---|---|
| **Génération de trains d'impulsions** (Poisson) | Modèle standard pour un flux de rayons cosmiques |
| **Détection de coïncidence** | **Le test le plus important** : deux flux de bruit indépendants à haut débit ne produisent qu'un taux de coïncidence fortuite faible, alors que des événements réellement corrélés (avec gigue de mesure réaliste) sont presque tous retrouvés — la coïncidence rejette vraiment le bruit, pas juste "tourne sans erreur" |
| **Correction barométrique** | **Deux vraies erreurs de signe physique attrapées par les tests** et corrigées : la correction allait dans le mauvais sens (une mesure à pression plus élevée doit être corrigée à la hausse, pas à la baisse — plus de pression = plus d'absorption atmosphérique = taux mesuré plus faible). Validé par régénération de données synthétiques à coefficient connu et vérification que l'ajustement le retrouve exactement |
| **Portage C de la coïncidence** | Comparé numériquement au Python via `ctypes`, à un taux réaliste de détecteur. Limite de quantification à très haute densité d'événements documentée explicitement plutôt que cachée (voir `tests/test_coincidence_c_matches_python.py`) |
| **Firmware STM32F405** | Compile et **linke réellement** (24,5 Ko) — réutilise les pilotes I2C/horloge déjà validés du projet nanodrone (même famille de MCU) |

## Ce qui n'est PAS vérifié

- **Le circuit analogique** (amplificateur, comparateur, convertisseur DC-DC
  pour la polarisation du SiPM) : composants réels cités dans
  `docs/hardware.md`, mais aucun schéma ni valeurs de composants fournis —
  la documentation CosmicWatch originale (schéma complet publié) est le bon
  point de départ plutôt que de re-router ce circuit à l'aveugle ici.
- **Le coefficient barométrique réel** de ce détecteur : le module
  d'ajustement (`barometric.py`) est prêt et testé sur données synthétiques,
  mais la vraie valeur de β doit être mesurée sur le détecteur assemblé.
- **Aucune mesure sur un vrai détecteur** — pas de matériel disponible dans
  cet environnement de développement.

## Nomenclature

**[docs/hardware.md](docs/hardware.md)** — composants réels vérifiés contre
la documentation publique CosmicWatch (SiPM onsemi C-Series, scintillateur
polystyrène+PPO+POPOP 5×5×1cm, convertisseur DC-DC MAX5026), plan de
brochage, et pourquoi deux voies plutôt qu'une.

## Structure du dépôt

```
sim/muon_sim/
├── pulse_train.py       # génération de trains d'impulsions (Poisson)
├── coincidence.py         # détection de coïncidence + statistiques
└── barometric.py            # correction barométrique (ajustement + application)
firmware/
├── Core/Inc, Core/Src        # pulse_timer (TIM2 capture), coincidence.c, bme280.c
├── Drivers/                    # en-têtes CMSIS vendorisés (STM32F405)
├── startup/                     # linker script + démarrage
└── Makefile                      # compilation arm-none-eabi-gcc
tests/                             # 19 tests, dont validation croisée C/Python
docs/hardware.md                     # nomenclature + brochage
.github/workflows/build.yml            # CI : tests + compilation firmware
```

## Installation et usage

```bash
pip install -r sim/requirements.txt
pytest tests/ -v                    # 19 tests

cd firmware
make                                 # produit build/muon_detector.elf
```

## Licence

MIT pour le code original — voir `LICENSE`. Fichiers CMSIS vendorisés sous
Apache 2.0 — voir `THIRD_PARTY_LICENSES.md`.
