/* ================= FIREBASE CONFIG ================= */
const firebaseConfig = {
  apiKey: "AIzaSyCY2gkrVGul7Uz8cHMeAIaKc32Wwioj8mA",
  authDomain: "guardian-medicine-box.firebaseapp.com",
  databaseURL: "https://guardian-medicine-box-default-rtdb.firebaseio.com",
  projectId: "guardian-medicine-box",
  storageBucket: "guardian-medicine-box.appspot.com",
  messagingSenderId: "511434884488",
  appId: "1:511434884488:web:b096d79d77bed540c6a63d"
};

firebase.initializeApp(firebaseConfig);
const db = firebase.database();

/* ================= DEVICE STATUS ================= */
const deviceStatus = document.getElementById("deviceStatus");

db.ref("device").on("value", snap => {
  if (!snap.exists()) return;
  deviceStatus.innerText = snap.val().online ? "🟢 Online" : "🔴 Offline";
});

/* ================= LOAD SCHEDULE ================= */
for (let i = 1; i <= 4; i++) {
  db.ref("schedule/" + i).on("value", s => {
    if (!s.exists()) return;
    document.getElementById("h" + i).value = s.val().hour;
    document.getElementById("m" + i).value = s.val().minute;
  });
}

/* ================= QR CAMERA ================= */
function startQRScanner() {
  const qr = new Html5Qrcode("qr-reader");
  qr.start(
    { facingMode: "environment" },
    { fps: 10, qrbox: 250 },
    text => {
      handleQRData(text);
      qr.stop();
    }
  );
}

/* ================= QR IMAGE ================= */
function scanFromImage() {
  const file = qrImage.files[0];
  if (!file) return alert("Select QR image");

  const qr = new Html5Qrcode("qr-reader");
  qr.scanFile(file, true)
    .then(text => handleQRData(text))
    .catch(() => alert("QR not readable"));
}

/* ================= HANDLE QR DATA (UPDATED) ================= */
function handleQRData(text) {
  try {
    const data = JSON.parse(text);

    const timeToDrawer = {
      morning: 1,
      noon: 2,
      evening: 3,
      night: 4
    };

    /* ---- APPLY SCHEDULE ---- */
    if (data.schedule) {
      const convertedSchedule = {};
      for (const t in data.schedule) {
        const d = timeToDrawer[t];
        if (d) convertedSchedule[d] = data.schedule[t];
      }
      db.ref("schedule").set(convertedSchedule);
    }

    /* ---- APPLY MEDICINES ---- */
    if (data.medicines) {
      for (const t in data.medicines) {
        const d = timeToDrawer[t];
        if (!d) continue;

        db.ref("drawers/" + d + "/medicines").set(null);

        data.medicines[t].forEach(med => {
          db.ref("drawers/" + d + "/medicines").push({
            name: med.name,
            qty: med.qty
          });
        });
      }
    }

    alert("✅ Prescription imported successfully");

  } catch (e) {
    alert("❌ Invalid QR Code");
  }
}

/* ================= MEDICINE LOADING GUIDE ================= */
const loadingGuide = document.getElementById("loadingGuide");

function loadMedicineGuide() {
  loadingGuide.innerHTML = "";

  db.ref("schedule").once("value").then(snapS => {
    db.ref("drawers").once("value").then(snapD => {

      const schedule = snapS.val();
      const drawers = snapD.val();

      for (let d = 1; d <= 4; d++) {
        const time = schedule?.[d];
        const meds = drawers?.[d]?.medicines;

        let html = `<h3>📦 Drawer ${d}</h3>`;

        if (time) {
          html += `<p>⏰ ${time.hour}:${time.minute}</p>`;
        }

        if (meds) {
          html += "<ul>";
          Object.values(meds).forEach(m =>
            html += `<li>💊 ${m.name} × ${m.qty}</li>`
          );
          html += "</ul>";
        } else {
          html += `<p>No medicines</p>`;
        }

        loadingGuide.innerHTML += html + "<hr>";
      }
    });
  });
}

setInterval(loadMedicineGuide, 3000);
loadMedicineGuide();
