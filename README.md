# dymon
Command line tools for DYMO LabelWriter USB. !Windows operation hasn't been tested!
This is a stripped down verion of the project dymon by minlux https://github.com/minlux/dymon/tree/master.

![dymon_srv](doc/lw.jpg)

This project implements `dymon_pbm` which allows printing of labels from command line.

![dymon_srv](doc/webif.png)



## Intro
To automate the calibration process and put a QR code with a link to the calibration results on the device. 


## Implementation
Most of the application code relates to the creation of the bitmap (text and barcode). The actual code to interface with the *LabelWriter* can be found in the files in folder `dymon` and in `src/print.cpp`.

## Notes
- *Dymon* is an abstact class. There is a specialization for Windows and one for Linux.
- To change the oriantation of the label, the bitmap must be rotated. The bitmap class in this project demostrates this.
- To get an idea how to change the label size/format have a look into `src/print.cpp`. There is an implementation for two different label formats with different orientation.


## How to build
Build process is based on CMake.

1. Create a build directory (e.g. `mkdir build`)
2. Within the build directory execute `cmake ..`
3. Within the build directory execute `make`


## Usage

### dymon_pbm
`dymon_pbm` is a command line tool, that allows to print *pbm P4* image files. on the *LabelWriter*.
The tool expects the following arguments:
- 1st argument:
   - the path to the USB *LabelWriter* (like `usb:/dev/usb/lp` on linux, or `usb:vid_0922` on windows)
- 2nd argument: full path to the bitmap file

In folder `doc` you can find some example files that can be printed (on the respective lables) like this:
```
./dymon_pbm "usb:/dev/usb/lp0"   ../doc/label_25x25.pbm

```

You can use *GIMP* to convert regular image files into pbm files. Therefore just *export* your image as `filename.pbm` and store it as *raw* pbm.
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


## See also
[Commands and Status](doc/cmd_status.md)
[Labels](doc/paper_size.md)

https://developers.dymo.com/#/article/1417


