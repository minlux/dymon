#!/usr/bin/env python3
"""
Erzeugt ein Testmuster als P4-PBM, um Größe, Ränder und Versatz eines
Etiketts zu prüfen (Rahmen, mm-Skala, Diagonalen, Mittelkreuz und ein
Ausrichtungsquadrat oben links).

Verwendung: python3 testpattern.py [output.pbm] [--size WxH] [--dpi DPI]
"""

import argparse


def render(width: int, height: int, dpi: int) -> list:
    """Zeichnet das Testmuster in ein 2D-Array (1 = schwarz)."""
    px = [[0] * width for _ in range(height)]
    dpmm = dpi / 25.4

    def dot(x, y):
        if 0 <= x < width and 0 <= y < height:
            px[y][x] = 1

    def rect(x0, y0, x1, y1):
        for y in range(max(y0, 0), min(y1, height)):
            for x in range(max(x0, 0), min(x1, width)):
                px[y][x] = 1

    # Rahmen (3 Punkte breit) an allen Kanten
    rect(0, 0, width, 3)
    rect(0, height - 3, width, height)
    rect(0, 0, 3, height)
    rect(width - 3, 0, width, height)

    # Diagonalen
    for x in range(width):
        y = round(x * (height - 1) / (width - 1))
        for d in range(-1, 2):
            dot(x, y + d)
            dot(x, height - 1 - y + d)

    # Mittelkreuz
    rect(width // 2 - 1, 0, width // 2 + 2, height)
    rect(0, height // 2 - 1, width, height // 2 + 2)

    # mm-Skala an allen Rändern (lang alle 10 mm, mittel alle 5 mm)
    def tick_len(mm):
        if mm % 10 == 0:
            return 40
        if mm % 5 == 0:
            return 22
        return 10

    for mm in range(int(width / dpmm) + 1):
        x = round(mm * dpmm)
        l = tick_len(mm)
        rect(x, 0, x + 2, l)
        rect(x, height - l, x + 2, height)
    for mm in range(int(height / dpmm) + 1):
        y = round(mm * dpmm)
        l = tick_len(mm)
        rect(0, y, l, y + 2)
        rect(width - l, y, width, y + 2)

    # Ausrichtungsmarke: gefülltes Quadrat oben links
    rect(60, 60, 110, 110)
    return px


def write_pbm(path: str, px: list, comment: str) -> None:
    """Schreibt das Bild als P4-PBM (rawbits)."""
    height = len(px)
    width = len(px[0])
    with open(path, "wb") as f:
        f.write(b"P4\n# %s\n%d %d\n" % (comment.encode(), width, height))
        for row in px:
            for i in range(0, width, 8):
                b = 0
                for bit in row[i:i + 8]:
                    b = (b << 1) | bit
                b <<= 8 - len(row[i:i + 8])  # letzte Bytes einer Zeile auffüllen
                f.write(bytes([b]))


def main():
    parser = argparse.ArgumentParser(description="Erzeugt ein Testmuster als P4-PBM.")
    parser.add_argument("output", nargs="?", default="testpattern.pbm",
                        help="Ausgabedatei (Standard: testpattern.pbm)")
    parser.add_argument("--size", default="672x378",
                        help="Bildgröße in Pixel als WxH (Standard: 672x378 = 57x32 mm bei 300 dpi)")
    parser.add_argument("--dpi", type=int, default=300,
                        help="Auflösung für die mm-Skala (Standard: 300)")
    args = parser.parse_args()

    width, height = (int(v) for v in args.size.lower().split("x"))
    dpmm = args.dpi / 25.4
    comment = "dymon test pattern %dx%d px (%.1fx%.1f mm @ %d dpi)" % (
        width, height, width / dpmm, height / dpmm, args.dpi)
    write_pbm(args.output, render(width, height, args.dpi), comment)
    print("%s: %s" % (args.output, comment))


if __name__ == "__main__":
    main()
