# Matériel — nomenclature détaillée

Conception dérivée du **CosmicWatch Desktop Muon Detector** (MIT/NCBJ, projet
open source bien documenté depuis 2018), adaptée en STM32 (continuité avec
les projets nanodrone et ballon-sonde) plutôt que l'Arduino Nano d'origine.
Deux voies pour la coïncidence — voir `docs/coincidence.md` pour pourquoi
c'est important. Références vérifiées contre la documentation publique
CosmicWatch, pas inventées.

## Nomenclature (BOM) — par voie de détection (×2 pour la coïncidence)

| # | Composant | Référence précise | Caractéristiques clés | Rôle |
|---|---|---|---|---|
| 1 | Scintillateur plastique | **5×5×1 cm**, polystyrène dopé 1% PPO + 0,03% POPOP (ou équivalent Bicron BC408) | Émission bleue, pic ~420 nm | Convertit le passage d'un muon en lumière de scintillation |
| 2 | SiPM | **onsemi C-Series SMT, réf. MICROFC-SMA-30035** (ou distributeur RS 185-9609) | Zone active 6×6 mm, boîtier 7×7 mm | Détecte les photons de scintillation |
| 3 | Gel de couplage optique | Silicone optique (silicone grease), indice adapté | Interface scintillateur/SiPM | Minimise les pertes par réflexion |
| 4 | Feuille réfléchissante + ruban isolant | Aluminium + ruban électrique noir | Enveloppe le scintillateur | Piège la lumière, bloque la lumière ambiante |
| 5 | Convertisseur DC-DC (bias SiPM) | **MAX5026** (ou équivalent booster) | Génère ~29-30 V depuis 3,3-5 V | Polarisation inverse du SiPM (~24,5 V seuil + ~5 V surtension) |
| 6 | Amplificateur | AOP rapide, montage non-inverseur, gain ×6 environ | Bande passante suffisante pour un pulse ~0,5 µs | Amplifie le pulse SiPM (10-100 mV en sortie brute) |
| 7 | Comparateur | Comparateur rapide (ex. LM339 ou équivalent) | Seuil réglable (potentiomètre) | Convertit le pulse analogique en front logique pour le MCU |
| 8 | Microcontrôleur | **STM32F405RGT6** (LQFP64) — même choix que le nanodrone | Cortex-M4F 168 MHz, TIM2 32 bits pour l'horodatage | Capture d'entrée 2 voies, coïncidence, comptage |
| 9 | Capteur pression | **Bosch BME280** — même capteur que le ballon-sonde | I²C, correction barométrique du taux (voir `sim/muon_sim/barometric.py`) | Contexte atmosphérique |
| 10 | Boîtier | Aluminium, léger, ~7×7×4 cm par voie | Blocage lumière ambiante | Protection mécanique/optique |
| 11 | Alimentation | USB 5V (ou pile), régulateur 3.3V pour le MCU | | Alim système |

**Coût par voie** : le CosmicWatch original revendique un coût total sous
100 $ pour une voie complète (scintillateur + SiPM + électronique) — ordre
de grandeur à confirmer selon fournisseurs actuels, pas une garantie de prix.

## Pourquoi deux voies (coïncidence)

Une seule voie compte tout ce qui déclenche le comparateur : vrais muons,
mais aussi bruit électronique et radioactivité ambiante locale (les deux ne
sont pas corrélés entre deux détecteurs séparés). En exigeant que les DEUX
voies déclenchent dans une fenêtre de quelques dizaines à quelques centaines
de microsecondes, le taux de coïncidences fortuites devient très faible
(voir `sim/muon_sim/coincidence.py`, testé), alors que les vrais muons — qui
traversent les deux scintillateurs en quasi-simultané — sont presque tous
retrouvés. C'est la seule vraie garantie que ce qu'on compte est un rayon
cosmique, pas du bruit.

Une seule voie reste utilisable en mode "comptage simple" (sans coïncidence)
si tu veux commencer avec un seul scintillateur — moins de matériel, moins
de certitude sur la nature de chaque déclenchement.

## Plan de brochage (STM32F405RGT6)

| Fonction | Broche(s) | Notes |
|---|---|---|
| TIM2_CH1 (comparateur voie A) | PA0 | Capture d'entrée, front montant, résolution 1 µs |
| TIM2_CH2 (comparateur voie B) | PA1 | Idem |
| I2C1 (BME280) | PB6 (SCL), PB7 (SDA) | Réutilise le pilote du nanodrone (même génération de périphérique I2Cv1) |
| USART3 (rapport, TX seul) | PB10 | 9600 bauds, comptages + pression en texte |

## Ce qui reste ouvert

- **Circuit analogique** (amplificateur, comparateur, convertisseur DC-DC)
  : composants réels cités, mais aucun schéma ni valeurs de composants
  précises (résistances, capacités) fournis ici — voir la documentation
  CosmicWatch originale (schéma complet publié) pour un point de départ
  éprouvé plutôt que de re-router ce circuit analogique à l'aveugle.
- **Coefficient barométrique** (`sim/muon_sim/barometric.py`) : le module
  d'ajustement est prêt et testé, mais la valeur réelle de β doit être
  mesurée sur ce détecteur précis, pas supposée — voir le module.
- **Étalonnage du seuil de comparateur** : à régler expérimentalement une
  fois le circuit assemblé (compromis bruit/efficacité), pas une valeur
  fixe à coder en dur.
