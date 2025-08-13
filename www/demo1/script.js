import Pbm4 from "../js/pbm4.js";
import { drawBarcodeEAN8 } from "../js/barcode.js";


function postUint8Array(uint8) {
    fetch("/pbm", {
        method: "POST",
        headers: {
            "Content-Type": "application/octet-stream",
        },
        body: uint8 // raw binary data
    });
}


const label = document.getElementById("label");
document.getElementById("label-button").addEventListener("click", function () {
  postUint8Array(Pbm4.fromCanvasElement("label").toUint8Array());
});
const label_ctx = label.getContext("2d");


const billet = document.getElementById("billet");
document.getElementById("billet-button").addEventListener("click", function () {
  let rotate = true;  
  postUint8Array(Pbm4.fromCanvasElement("billet").toBase64(rotate));
});
const billet_ctx = billet.getContext("2d");


//first of all, fill the entire label "white"
label_ctx.fillStyle = "white";
label_ctx.fillRect(0, 0, label.width, label.height);
label_ctx.strokeRect(0, 0, label.width, label.height);

//now we can draw with black color to the label
label_ctx.fillStyle = "black";
let margin = 10;
let fontSize = 29;
label_ctx.font = `bold ${fontSize}px sans-serif`; //[style] [variant] [weight] [size] [family]
label_ctx.fillText("4194 Heiß Manuel", margin, 1 * (fontSize + margin));
label_ctx.font = `${fontSize}px sans-serif`; //[style] [variant] [weight] [size] [family]
label_ctx.fillText("LG", margin, 2 * (fontSize + margin));
label_ctx.fillText("MST/FST/", margin, 3 * (fontSize + margin));
fontSize = 27;
label_ctx.font = `${fontSize}px sans-serif`; //[style] [variant] [weight] [size] [family]
label_ctx.fillText("#1177.1777 23.09.25", margin, 4 * (fontSize + margin) + 5);

const BARCODE_WIDTH = 200; //almost 201 - but an even number
drawBarcodeEAN8(label_ctx, 1777, (label.width-BARCODE_WIDTH)/2, 4 * (fontSize + margin) + 3 * margin); // Replace with your 8-digits EAN



// Example: draw a filled circle
billet_ctx.fillStyle = "white";
billet_ctx.fillRect(0, 0, billet.width, billet.height);
billet_ctx.strokeRect(0, 0, billet.width, billet.height);

billet_ctx.fillStyle = "black";
billet_ctx.font = `bold 100px serif`;
billet_ctx.fillText("Hallo Welt", 480, 380);

billet_ctx.beginPath();
billet_ctx.arc(50, 50, 30, 0, Math.PI * 2);
billet_ctx.fill();

billet_ctx.beginPath();
billet_ctx.moveTo(0, 0);
billet_ctx.lineTo(0, 31);
billet_ctx.lineTo(31, 0);
billet_ctx.stroke();



drawBarcodeEAN8(billet_ctx, 815, billet.width - 100, 2*margin, true);
