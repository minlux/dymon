#!/usr/bin/env python3
"""
Extrahiert den Datamatrix-Code aus einer Internetmarke-PDF der Deutschen Post
und speichert ihn als P4-PBM.

Verwendung: python3 extract_datamatrix.py <input.pdf> [output.pbm] [--extend WxH]
"""

import sys
import argparse
import subprocess
import tempfile
import os
from pathlib import Path
from PIL import Image


def extract_datamatrix_from_pdf(pdf_path: str) -> Image.Image:
    """Extrahiert das erste Bild aus der PDF (der Datamatrix-Code)."""
    with tempfile.TemporaryDirectory() as tmpdir:
        result = subprocess.run(
            ["pdfimages", "-all", pdf_path, os.path.join(tmpdir, "img")],
            capture_output=True, text=True
        )
        if result.returncode != 0:
            raise RuntimeError(f"pdfimages fehlgeschlagen: {result.stderr}")

        images = sorted(Path(tmpdir).glob("img-*"))
        if not images:
            raise RuntimeError("Kein Bild in der PDF gefunden.")

        # Erstes Bild ist der Datamatrix-Code
        img = Image.open(images[0])
        img.load()  # Vor dem Schließen des tmpdir laden

    return img


def center_on_canvas(dm_img: Image.Image, width: int, height: int) -> Image.Image:
    """Zentriert das Datamatrix-Bild auf einem weißen Canvas der Zielgröße."""
    if dm_img.mode != "1":
        dm_img = dm_img.convert("1")

    canvas = Image.new("1", (width, height), color=1)  # weißer Hintergrund

    x_off = (width - dm_img.width) // 2
    y_off = (height - dm_img.height) // 2

    canvas.paste(dm_img, (x_off, y_off))
    return canvas


def save_p4_pbm(img: Image.Image, output_path: str):
    """Speichert ein 1-Bit-Bild als P4 (binäres) PBM."""
    if img.mode != "1":
        img = img.convert("1")

    width, height = img.size

    # P4-Header schreiben
    with open(output_path, "wb") as f:
        f.write(f"P4\n{width} {height}\n".encode("ascii"))

        # Pixel zeilenweise als Bits verpacken (MSB first)
        # P4: 1=schwarz, 0=weiß
        row_bytes = (width + 7) // 8
        pixels = img.load()
        for y in range(height):
            row = bytearray(row_bytes)  # alle Bits 0 = weiß in P4
            for x in range(width):
                # PIL mode "1": 0=schwarz, 255=weiß
                if pixels[x, y] == 0:  # schwarz → Bit auf 1 setzen
                    row[x // 8] |= (0x80 >> (x % 8))
            f.write(bytes(row))


def parse_size(value: str) -> tuple[int, int]:
    """Parst eine Größenangabe im Format WxH."""
    try:
        w, h = value.lower().split("x")
        return int(w), int(h)
    except ValueError:
        raise argparse.ArgumentTypeError(f"Ungültiges Format '{value}', erwartet: WxH (z.B. 272x252)")


def main():
    parser = argparse.ArgumentParser(
        description="Extrahiert den Datamatrix-Code aus einer Internetmarke-PDF als P4-PBM."
    )
    parser.add_argument("input", help="Eingabe-PDF")
    parser.add_argument("output", nargs="?", default="marke.pbm", help="Ausgabe-PBM (Standard: marke.pbm)")
    parser.add_argument(
        "--extend", metavar="WxH", type=parse_size,
        help="Canvas-Größe, in der der Code zentriert wird (z.B. 272x252). "
             "Ohne diese Option entspricht die PBM-Größe exakt dem extrahierten Bild."
    )
    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"Fehler: Datei nicht gefunden: {args.input}", file=sys.stderr)
        sys.exit(1)

    print(f"Lese PDF: {args.input}")
    dm_img = extract_datamatrix_from_pdf(args.input)
    print(f"Datamatrix gefunden: {dm_img.width}x{dm_img.height} Pixel, Modus: {dm_img.mode}")

    if args.extend:
        w, h = args.extend
        if dm_img.width > w or dm_img.height > h:
            print(f"Fehler: Canvas {w}x{h} ist kleiner als das Bild {dm_img.width}x{dm_img.height}.", file=sys.stderr)
            sys.exit(1)
        canvas = center_on_canvas(dm_img, w, h)
        x_off = (w - dm_img.width) // 2
        y_off = (h - dm_img.height) // 2
        print(f"Zentriert auf {w}x{h}, Offset: ({x_off}, {y_off})")
    else:
        canvas = dm_img if dm_img.mode == "1" else dm_img.convert("1")

    save_p4_pbm(canvas, args.output)
    print(f"Gespeichert: {args.output}")


if __name__ == "__main__":
    main()
