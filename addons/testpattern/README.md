# Testmuster

Erzeugt ein Testmuster als P4-PBM-Datei, um Größe, Ränder und Versatz eines Etiketts zu prüfen.

Das Muster enthält:

- **Rahmen** an allen Kanten – fehlt eine Seite, wird dort abgeschnitten.
- **mm-Skala** an allen Rändern (lange Striche alle 10 mm, mittlere alle 5 mm) – Rand oder Versatz lassen sich direkt ablesen.
- **Diagonalen und Mittelkreuz** – zeigen Verzerrung, Stauchung oder Verschiebung.
- **Schwarzes Quadrat oben links** – zur Erkennung der Ausrichtung.

## Voraussetzungen

- Python 3 (keine weiteren Pakete)

## Verwendung

```bash
python3 testpattern.py [output.pbm] [--size WxH] [--dpi DPI]
```

Ohne Angaben wird `testpattern.pbm` mit 672x378 Pixel erzeugt (57x32 mm, Typ 11354, bei 300 dpi).

**Beispiele:**

```bash
# Testmuster für 57x32 mm Etiketten (11354)
python3 testpattern.py test_57x32.pbm

# Testmuster mit anderer Größe
python3 testpattern.py test.pbm --size 336x1050
```

Die mitgelieferte Datei `test_57x32.pbm` wurde mit den Standardwerten erzeugt.
