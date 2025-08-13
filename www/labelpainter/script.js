import Pbm4 from "../js/pbm4.js";

function postUint8Array(uint8) {
    fetch("/pbm", {
        method: "POST",
        headers: {
            "Content-Type": "application/octet-stream",
        },
        body: uint8 // raw binary data
    });
}

document.getElementById("printCanvasBtn").addEventListener("click", function () {
  postUint8Array(Pbm4.fromCanvasElement("canvas").toUint8Array());
});
