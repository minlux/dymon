# Internetmarke – Datamatrix-Extraktor

Extrahiert den Datamatrix-Code aus einer PDF-Internetmarke (Deutsche Post) und speichert ihn als P4-PBM-Datei.

## Voraussetzungen

- Python 3 mit [Pillow](https://pillow.readthedocs.io/)
- `pdfimages` aus dem Paket `poppler-utils`

```bash
sudo apt install poppler-utils
pip install pillow
```

## Verwendung

```bash
python3 extract_datamatrix.py <input.pdf> [output.pbm] [--extend WxH]
```

Wird kein Ausgabepfad angegeben, wird `marke.pbm` im aktuellen Verzeichnis erstellt.

**Beispiele:**

```bash
# PBM in der Originalgröße des Datamatrix-Codes
python3 extract_datamatrix.py Briefmarken.1Stk.10.04.2026_1006.pdf marke.pbm

# PBM mit 272x252 Pixel, Datamatrix zentriert
python3 extract_datamatrix.py Briefmarken.1Stk.10.04.2026_1006.pdf marke.pbm --extend 272x252
```

## Hinweis

Das Skript extrahiert blind das erste eingebettete Rasterbild aus der PDF – ohne den Inhalt zu analysieren. Das funktioniert, weil Internetmarken-PDFs der Deutschen Post genau ein Bild enthalten: den Datamatrix-Code. Enthält eine PDF mehrere Bilder, würde das erste genommen, was zum falschen Ergebnis führen kann.

## Ausgabe

- Format: PBM P4 (binäres Bitmap, 1 Bit pro Pixel)
- Größe: standardmäßig die Originalgröße des extrahierten Bildes; mit `--extend WxH` die angegebene Canvas-Größe
- Bei `--extend`: Datamatrix-Code ist im Bild zentriert, Hintergrund weiß
- Hintergrund: weiß (0), Code: schwarz (1) — gemäß P4-Konvention
