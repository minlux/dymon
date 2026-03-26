# PBM Creation

Printing with `dymon_pbm` requires label content to be provided as a PBM (Portable Bitmap) file. There are several tools that can be used to create PBM files — some of them are described below.

## txt2pbm

```
Convert text to P4 portable bitmap.

Usage: txt2pbm
 [-vr] [--help] -w <PIXEL> -h <HEIGHT> [-c <TEXT>] -o <OUTPUT> [<INPUT>]

Options:
  --help                    Print help and exit
  -v, --version             Print version and exit
  -w, --width=<PIXEL>       Width of PBM
  -h, --height=<HEIGHT>     Height of PBM
  -c, --comment=<TEXT>      Comment, embedded into PBM
  -r, --rotate              Rotate final PBM by 90 degrees
  -o, --output=<OUTPUT>     Output PBM file (use '-' for stdout)
  <INPUT>                   Input text file [default: stdin]
```

You can use a minimal markup language to format each line of the input text.
It supports the same format specifier as `dymon_srv` (in fact both tools use the same "text processor").
Format specifier starts at the beginning of every line with a backslash `\`
followed by one or more of the following options:

- *NUMBER*: specify font size (15, 18, 21, 24), line width (pixels) or barcode weight
- `l`/`c`/`r`: specify horizontal text alignment (left, centered, right)
- `_`: to draw a horizontal line
- `#`: to draw a EAN-9 barcode with the following value

**Examples:**

Read from file:

```
txt2pbm -w 272 -h 252 -o test.pbm test.txt
```

Pipe text:

```
echo -e "Hello World\nLorem ipsum" | txt2pbm -w 272 -h 252 -o test.pbm
```

Read from *stdin*:

```
txt2pbm -w 392 -h 960 -r -o test.pbm
```

Note: To finish reading from stdin:
- "CTRL+D" (on Linux)
- "CTRL+Z" followed by "Enter" (on Windows)


## GIMP

You can use *GIMP* to convert regular image files into pbm files.
Therefore just *export* your image as `filename.pbm` and store it as *raw* pbm.


## Imagemagick

You can also use *Imagemagick* to convert a file into pbm. For example:

```
convert your_pic.jpg your_pic.pbm
```

You can also do more advanced conversion at once using *Imagemagick's convert*. The following command resizes an image but keeps the aspect ration. It will be resized so that the width is maximal 960 pixel, the height is maximal 392 pixel. It depends on the geometry of the input image which rule applies. Then it extends the canvas to 960x392 and aligns the image centered in the canvas. Then the image is rotated by 90 degrees (counter clockwise). Finally it is converted to pbm p4. The resulting image would fit to a 36mm x 89mm label.

```
convert -resize 960x392 -extent 960x392 -gravity center -rotate 90 eagle.jpg eagle_36x89.pbm
```

Same for a 25mm x 25mm label:

```
convert -resize 272x252 -extent 272x252 -gravity center logo.svg logo.pbm
```
