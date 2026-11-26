# Projet_UART : station de surveillance ESP32-S3

Projet d'apprentissage en systemes embarques (ESP-IDF) : lecture de plusieurs
capteurs reels et affichage des mesures avec les logs serie (niveaux, tags,
horodatage).

## Materiel

- Carte **ESP32-S3 N16R8** (16 Mo de flash, 8 Mo de PSRAM)
- Capteur d'humidite du sol (sortie analogique)
- Detecteur de mouvement PIR HC-SR501
- Module suiveur de ligne infrarouge (MH-B, sortie numerique)
- Capteur de temperature et d'humidite de l'air DHT11

## Cablage

| Capteur | Broche du module | Broche de la carte |
|---|---|---|
| Humidite du sol HW-390 | AO | GPIO4 (ADC1) |
| PIR HC-SR501 | OUT | GPIO5 |
| Suiveur de ligne | OUT | GPIO6 |
| DHT11 | DATA | GPIO7 |

Tous les capteurs sont alimentes en 3V3 (VCC sur 3V3, GND sur GND).

Broches a eviter sur l'ESP32-S3 N16R8 : GPIO26 a 37 (flash et PSRAM),
GPIO19 et 20 (USB), GPIO43 et 44 (console UART0), GPIO0, 3, 45 et 46 (demarrage).

## Compiler et flasher

Prerequis : ESP-IDF installe et active (projet ecrit avec la version 6.1).

```
idf.py set-target esp32s3
idf.py build
idf.py -p COMx flash monitor
```

Remplace `COMx` par le port de ta carte. La bibliotheque DHT
(`esp-idf-lib/dht`) est declaree dans `main/idf_component.yml` et se telecharge
automatiquement au premier build (connexion internet necessaire).

Pour quitter le moniteur : `Ctrl+]`.

## Ce que fait le programme

Une boucle de 500 ms lit les capteurs et ecrit des logs :

| Tag | Role |
|---|---|
| `sol` | Humidite du sol toutes les 2 s : correcte, sec, ou valeur hors plage |
| `pir` | Mouvement detecte / plus de mouvement (seulement les changements) |
| `ligne` | Etat du suiveur de ligne (seulement les changements) |
| `dht` | Temperature et humidite de l'air toutes les 2 s, erreurs de lecture |
| `etat` | Etat brut du PIR et du suiveur a chaque tour (mise au point) |

Niveaux utilises : `I` (information), `W` (avertissement), `E` (erreur).

## Reglages

- `SOL_SEC` (dans `main.c`) : seuil de sol sec, a calibrer en relevant la valeur
  de la sonde a l'air puis dans l'eau.
- Suiveur de ligne : regler le potentiometre du module pour que le blanc
  allume la LED du module et que le noir mat ne l'allume pas (distance de
  detection : environ 1 cm).
- PIR : attendre environ une minute apres la mise sous tension avant de tester.

## Limites connues

- Un fil debranche sur le capteur d'humidite du sol n'est detecte que si la
  valeur lue sort de la plage valide : l'ADC ne distingue pas toujours un fil
  debranche d'une vraie mesure.
- Le DHT11 est peu precis (environ 2 degres Celsius et 5 % d'humidite).
- Le PIR peut osciller brievement entre 0 et 1 quand le mouvement s'arrete.

## Suite prevue

- Console de commandes sur l'UART (composant `console` d'ESP-IDF)
- Session de debogage avec GDB (USB-JTAG integre de l'ESP32-S3)
