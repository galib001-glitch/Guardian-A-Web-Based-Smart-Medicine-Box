let qr;

/* ================= GENERATE QR ================= */
function generateQR() {

  /* ---- Fixed Schedule ---- */
  const schedule = {
    morning: { hour: 8, minute: 0 },
    noon: { hour: 13, minute: 0 },
    evening: { hour: 18, minute: 0 },
    night: { hour: 22, minute: 0 }
  };

  /* ---- Medicines Object ---- */
  const medicines = {
    morning: [],
    noon: [],
    evening: [],
    night: []
  };

  const input = document.getElementById("medInput").value.trim();
  if (!input) {
    alert("Please enter medicine data");
    return;
  }

  const lines = input.split("\n");

  lines.forEach(line => {
    const parts = line.split(",");

    if (parts.length !== 3) return;

    const time = parts[0].trim().toLowerCase();
    const name = parts[1].trim();
    const qty = Number(parts[2].trim());

    if (!medicines[time]) return;

    medicines[time].push({
      name: name,
      qty: qty
    });
  });

  /* ---- Final JSON ---- */
  const jsonData = {
    schedule: schedule,
    medicines: medicines
  };

  const jsonText = JSON.stringify(jsonData, null, 2);
  document.getElementById("jsonOutput").innerText = jsonText;

  /* ---- Generate QR ---- */
  document.getElementById("qrcode").innerHTML = "";

  qr = new QRCode(document.getElementById("qrcode"), {
    text: JSON.stringify(jsonData),
    width: 256,
    height: 256
  });

  document.getElementById("downloadBtn").disabled = false;
}

/* ================= DOWNLOAD QR ================= */
function downloadQR() {
  const img = document.querySelector("#qrcode img");
  if (!img) return;

  const link = document.createElement("a");
  link.href = img.src;
  link.download = "prescription_qr.png";
  link.click();
}
