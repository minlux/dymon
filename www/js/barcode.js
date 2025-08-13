//ctx: document.getElementById("my-canvas").getContext("2d"); 
export function drawBarcodeEAN8(ctx, number, x, y, horizontal = false) {
   const barcodeWeight = 3;
   const barcodeHeight = 75;
   const barcodeWidth = 67 * barcodeWeight;
   const margin = 3 * barcodeWeight;

   const EAN8_LEFT = {
      "0": "0001101",
      "1": "0011001",
      "2": "0010011",
      "3": "0111101",
      "4": "0100011",
      "5": "0110001",
      "6": "0101111",
      "7": "0111011",
      "8": "0110111",
      "9": "0001011",
   };

   const EAN8_RIGHT = {
      "0": "1110010",
      "1": "1100110",
      "2": "1101100",
      "3": "1000010",
      "4": "1011100",
      "5": "1001110",
      "6": "1010000",
      "7": "1000100",
      "8": "1001000",
      "9": "1110100",
   };

   function getBarcodeDigits(value /*: Int*/) {
      let digits = new Uint8Array(8);
      //get 7 decimal digits
      digits[0] = (value / (1000000)) % 10;
      digits[1] = (value / (100000)) % 10;
      digits[2] = (value / (10000)) % 10;
      digits[3] = (value / (1000)) % 10;
      digits[4] = (value / (100)) % 10;
      digits[5] = (value / (10)) % 10;
      digits[6] = value % 10;

      //calc checksum digits -> digits[7]
      let oddDigits = digits[6] + digits[4] + digits[2] + digits[0];
      let evenDigits = digits[5] + digits[3] + digits[1];
      let remainder = ((3 * oddDigits + evenDigits) % 10);
      digits[7] = (10 - remainder) % 10;
      return digits.join("");
   }

   function drawBarsVertical(pattern, fullHeight = false) {
      const barWidth = barcodeWeight;
      const barHeight = barcodeHeight;
      for (let bit of pattern) {
         ctx.fillStyle = bit === "1" ? "black" : "white";
         ctx.fillRect(x, y, barWidth, fullHeight ? barHeight + 10 : barHeight);
         x += barWidth;
      }
   }

   function drawBarsHorizontal(pattern, fullLength = false) {
      const barLength = barcodeHeight;
      const barHeight = barcodeWeight;
      for (let bit of pattern) {
         ctx.fillStyle = bit === "1" ? "black" : "white";
         ctx.fillRect(x, y, fullLength ? barLength + 10 : barLength, barHeight);
         y += barHeight;
      }
   }

   // set respective function pointer
   let drawBars = horizontal ? drawBarsHorizontal : drawBarsVertical;

   // clear space
   let style = ctx.fillStyle;
   ctx.fillStyle = "white";
   if (horizontal) {
      ctx.fillRect(x - margin, y - margin, barcodeHeight + 2 * margin, barcodeWidth + 2 * margin);
   } else {
      ctx.fillRect(x - margin, y - margin, barcodeWidth + 2 * margin, barcodeHeight + 2 * margin);
   }

   // Convert the number (integer value) into the 8 barcode digits + the 9th checksum digit
   const digits = getBarcodeDigits(number);

   // Start guard (101)
   drawBars("101", false);

   // Left 4 digits
   for (let i = 0; i < 4; i++) {
      drawBars(EAN8_LEFT[digits[i]]);
   }

   // Center guard (01010)
   drawBars("01010", false);

   // Right 4 digits
   for (let i = 4; i < 8; i++) {
      drawBars(EAN8_RIGHT[digits[i]]);
   }

   // End guard (101)
   drawBars("101", false);
   ctx.fillStyle = style;
}

//A EAN8 barcode has 67+4 bars, for given weight. 
//this defines the total width of the barcode.
//E.g. For a weight of 3, the width = 3 * 67 = 201
/*
const BARCODE_WIDTH = 200; //almost 201 - but an even number
*/
