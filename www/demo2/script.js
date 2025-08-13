/*
 * QR Code generator output demo (TypeScript)
 *
 * Copyright (c) Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/qr-code-generator-library
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */
import Pbm4 from "../js/pbm4.js";


function postUint8Array(uint8, ip, copies) {
    fetch("/pbm?" + new URLSearchParams({
        ip: ip || "0.0.0.0",
        copies: copies,
    }).toString(), {
        method: "POST",
        headers: {
            "Content-Type": "application/octet-stream",
        },
        body: uint8 // raw binary data
    });
}

"use strict";
(function () {
    let canvas = document.getElementById("qrcode");
    document.getElementById("text").addEventListener("input", () => drawQrCode());
    document.getElementById("print").addEventListener("click", function () {

      let ip = document.getElementById("ip").value;
      let copies = document.getElementById("count").value;
      postUint8Array(Pbm4.fromCanvasElement("qrcode").toUint8Array(), ip, copies);
    });

    // Creates a single QR Code, then appends it to the document.
    function drawQrCode() {
        const text = document.getElementById("text").value;
        console.log(text);
        const errCorLvl = qrcodegen.QrCode.Ecc.LOW; // Error correction level
        const qr = qrcodegen.QrCode.encodeText(text, errCorLvl); // Make the QR Code symbol
        drawCanvas(qr, 6, 0, "#FFFFFF", "#000000", canvas); // Draw it on screen
    }

    // Draws the given QR Code, with the given module scale and border modules, onto the given HTML
    // canvas element. The canvas's width and height is resized to (qr.size + border * 2) * scale.
    // The drawn image is purely dark and light, and fully opaque.
    // The scale must be a positive integer and the border must be a non-negative integer.
    function drawCanvas(qr, scale, border, lightColor, darkColor, canvas) {
        if (scale <= 0 || border < 0)
            throw new RangeError("Value out of range");
        let width = (qr.size + border * 2) * scale;
        width = Math.ceil(width / 8) * 8; //round up a number to the nearest multiple of 8
        canvas.width = width;
        canvas.height = width;
        let ctx = canvas.getContext("2d");
        for (let y = -border; y < qr.size + border; y++) {
            for (let x = -border; x < qr.size + border; x++) {
                ctx.fillStyle = qr.getModule(x, y) ? darkColor : lightColor;
                ctx.fillRect((x + border) * scale, (y + border) * scale, scale, scale);
            }
        }
    }

    // initial call
    drawQrCode();
})();
