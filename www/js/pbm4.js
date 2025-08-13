export default class Pbm4 {
  #canvas;

  constructor(canvas) {
    this.#canvas = canvas;
  }

  static fromCanvasElement(id) {
    return new Pbm4(document.getElementById(id));
  }


  toUint8Array(rotate) {
    // Define PBM header: P4 defines binary representation
    let header = `P4\n${this.#canvas.width} ${this.#canvas.height}\n`;
    if (rotate) {
      header = `P4\n${this.#canvas.height} ${this.#canvas.width}\n`;
    }

    // Allocate buffer for the actual bitmap with a preceiding header
    let pbm = new Uint8Array(header.length + (this.#canvas.width * this.#canvas.height) / 8);
    let src = 0;
    let dst = 0;

    // Copy header
    for (; dst < header.length; dst++) {
      pbm[dst] = header.charCodeAt(src++);
    }

    // Get imagedata encoded as rbga bytes,
    // convert the pixel to 1-bit black and white bits,
    // pack 8 bits into 1 byte
    const ctx = this.#canvas.getContext("2d");
    const imageData = ctx.getImageData(0, 0, this.#canvas.width, this.#canvas.height);
    if (!rotate) {
      for (src = 0; src < imageData.data.length;) {
        let bitmap = 0;
        let bitmask = 0x80;
        for (let bit = 0; bit < 8; bit++) {
          const r = imageData.data[src++];
          const g = imageData.data[src++];
          const b = imageData.data[src++];
          const a = imageData.data[src++];
          // const grayscale = 0.299 * r + 0.587 * g + 0.114 * b;
          if (g <= 128) {
            bitmap = (bitmap | bitmask);
          }
          bitmask = (bitmask >> 1);
        }
        pbm[dst++] = bitmap;
      }
    } else {
      let pos0 = 4 * (this.#canvas.width * (this.#canvas.height - 1));
      for (let x = 0; x < 4 * this.#canvas.width; x += 4) {
        for (let y = pos0; y >= 0;) {
          let bitmap = 0;
          let bitmask = 0x80;
          for (let bit = 0; bit < 8; bit++) {
            const r = imageData.data[y + x + 0];
            const g = imageData.data[y + x + 1];
            const b = imageData.data[y + x + 2];
            const a = imageData.data[y + x + 3];
            // const grayscale = 0.299 * r + 0.587 * g + 0.114 * b;
            if (g <= 128) {
              bitmap = (bitmap | bitmask);
            }
            bitmask = (bitmask >> 1);
            y = y - 4 * this.#canvas.width;
          }
          pbm[dst++] = bitmap;
        }
      }
    }
    return pbm;
  }


  toBase64(rotate) {
    const binString = Array.from(this.toUint8Array(rotate), (byte) =>
      String.fromCodePoint(byte),
    ).join("");
    return btoa(binString);
  }
}
