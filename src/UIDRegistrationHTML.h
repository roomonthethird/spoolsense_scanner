#pragma once

// UID Registration page served at GET /register/uid
//
// API endpoints available to the page:
//   GET  /api/status      — current tag state JSON (includes uid, spoolman_url)
//   (Spoolman API calls are made directly from the browser to the configured Spoolman instance)

const char UID_REGISTRATION_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>UID Registration &mdash; SpoolSense</title>
  <link rel="stylesheet" href="/css/shared.css?v=1.7.0" />
</head>
<body>
  <div class="wrap">
    <nav>
      <span class="nav-brand">SpoolSense</span>
      <a href="/">Home</a>
      <a href="/reader">Reader</a>
      <a href="/writer/openprinttag">OpenPrintTag</a>
      <a href="/writer/tigertag">TigerTag</a>
      <a href="/writer/opentag3d">OpenTag3D</a>
      <a href="/writer/openspool">OpenSpool</a>
      <a href="/register/uid" class="active">NFC+</a>
      <a href="/update">Update</a>
      <a href="/troubleshooting">Troubleshooting</a>
      <a href="/config">Config</a>
    </nav>

    <section class="card" id="createView">
      <div class="card-head">
        <div>
          <h1 class="card-title">Register Spool in Spoolman</h1>
          <p class="card-subtitle">Create a Spoolman spool entry for a plain NFC tag. No data is written to the tag &mdash; it&apos;s used as a UID identifier only.</p>
        </div>
      </div>

      <div class="card-body">
        <div style="background:rgba(99,102,241,.12);border:1px solid rgba(99,102,241,.35);border-radius:14px;padding:18px 22px;margin-bottom:20px;text-align:center">
          <div style="font-size:12px;color:var(--muted);text-transform:uppercase;letter-spacing:.08em;margin-bottom:6px">Tag UID</div>
          <div id="uidValue" style="font-size:16px;font-weight:700;letter-spacing:.08em;font-family:monospace;color:#a5b4fc">Place tag &amp; click Read</div>
        </div>

        <form id="registerForm">
          <section>
            <h2 class="section-title">Basic</h2>
            <div class="grid-2">
              <div class="field">
                <label for="material_type">Material</label>
                <input id="material_type" name="material_type" list="material-list" placeholder="Type to search materials" required />
                <datalist id="material-list">
                  <option value="PLA"></option>
                  <option value="PETG"></option>
                  <option value="TPU"></option>
                  <option value="ABS"></option>
                  <option value="ASA"></option>
                  <option value="PC"></option>
                  <option value="PCTG"></option>
                  <option value="PP"></option>
                  <option value="PA6"></option>
                  <option value="PA11"></option>
                  <option value="PA12"></option>
                  <option value="PA66"></option>
                  <option value="CPE"></option>
                  <option value="TPE"></option>
                  <option value="HIPS"></option>
                  <option value="PHA"></option>
                  <option value="PET"></option>
                  <option value="PEI"></option>
                  <option value="PBT"></option>
                  <option value="PVB"></option>
                  <option value="PVA"></option>
                  <option value="PEKK"></option>
                  <option value="PEEK"></option>
                </datalist>
              </div>

              <div class="field">
                <label for="manufacturer">Brand / Manufacturer</label>
                <input id="manufacturer" name="manufacturer" list="brand-list" placeholder="Select or type a brand" required />
                <datalist id="brand-list">
                  <option value="3DXTech"></option>
                  <option value="Amolen"></option>
                  <option value="Ambrosia"></option>
                  <option value="Anycubic"></option>
                  <option value="Atomic Filament"></option>
                  <option value="AzureFilm"></option>
                  <option value="Bambu Lab"></option>
                  <option value="ColorFabb"></option>
                  <option value="Creality"></option>
                  <option value="eSun"></option>
                  <option value="Elegoo"></option>
                  <option value="Fiberlogy"></option>
                  <option value="Fillamentum"></option>
                  <option value="FlashForge"></option>
                  <option value="FormFutura"></option>
                  <option value="Hatchbox"></option>
                  <option value="IC3D"></option>
                  <option value="Inland"></option>
                  <option value="Jessie"></option>
                  <option value="MatterHackers"></option>
                  <option value="Overture"></option>
                  <option value="Polymaker"></option>
                  <option value="Prusament"></option>
                  <option value="Proto-Pasta"></option>
                  <option value="Siraya-Tech"></option>
                  <option value="Sunlu"></option>
                  <option value="Taulman3D"></option>
                </datalist>
              </div>

              <div class="field">
                <label for="material_name">Material Name</label>
                <input id="material_name" name="material_name" type="text" placeholder="PLA" />
                <div class="hint">Material name is forced to ALL CAPS.</div>
              </div>

              <div class="field">
                <label for="initial_weight_g">Full Weight (g)</label>
                <input id="initial_weight_g" name="initial_weight_g" type="number" min="0" step="0.1" placeholder="1000.0" required />
              </div>

              <div class="field">
                <label for="remaining_g">Remaining Weight (g)</label>
                <input id="remaining_g" name="remaining_g" type="number" min="0" step="0.1" placeholder="750.0" required />
              </div>

              <div class="field">
                <label>Color</label>
                <div class="color-row">
                  <input id="colorPicker" type="color" value="#ff0000" />
                  <input id="colorHex" name="color" type="text" value="#FF0000" maxlength="7" placeholder="#FF0000" required />
                </div>
                <div class="hint">Pick a color or type a hex value.</div>
              </div>
            </div>
          </section>

          <section>
            <button type="button" class="advanced-toggle" id="advancedToggle" aria-expanded="false">
              <span>Advanced fields</span>
              <span data-toggle-text>Show</span>
            </button>

            <div class="advanced-box hidden" id="advancedBox">
              <div class="grid-3">
                <div class="field">
                  <label for="density">Density</label>
                  <input id="density" name="density" type="number" min="0" step="0.01" placeholder="1.24" />
                </div>

                <div class="field">
                  <label for="diameter_mm">Diameter (mm)</label>
                  <input id="diameter_mm" name="diameter_mm" type="number" min="0" step="0.01" placeholder="1.75" />
                </div>

                <div class="field">
                  <label for="extruder_temp">Extruder Temp (&deg;C)</label>
                  <input id="extruder_temp" name="extruder_temp" type="number" min="0" step="1" placeholder="215" />
                </div>

                <div class="field">
                  <label for="bed_temp">Bed Temp (&deg;C)</label>
                  <input id="bed_temp" name="bed_temp" type="number" min="0" step="1" placeholder="60" />
                </div>
              </div>
            </div>
          </section>

          <div class="actions">
            <button type="button" class="btn-secondary" id="readUidBtn">Read Tag UID</button>
            <button type="submit" class="btn-primary" id="registerBtn">Register in Spoolman</button>
            <button type="reset" class="btn-ghost">Clear</button>
          </div>

          <div id="resultBox" style="margin-top:14px"></div>

          <div class="hint" style="margin-top:12px">This creates a spool entry in Spoolman with the tag&apos;s UID as the identifier. No data is written to the tag.</div>
        </form>
      </div>
    </section>

    <div class="footer-note">UID Registration &mdash; Spoolman spool via NFC tag UID</div>
  </div>

  <script src="/js/shared.js?v=1.7.0"></script>
  <script>
    var registerForm = document.getElementById('registerForm');
    var uidValue = document.getElementById('uidValue');
    var resultBox = document.getElementById('resultBox');
    var readUidBtn = document.getElementById('readUidBtn');

    var materialTypeEl = document.getElementById('material_type');
    var materialNameEl = document.getElementById('material_name');

    var currentUid = '';

    syncColorPicker('colorPicker', 'colorHex');
    setupAdvancedToggle('advancedToggle', 'advancedBox');

    // Auto-fill temps and density from material selection
    var nfcFieldMap = {
      density: 'density'
    };
    trackAutoFill(['extruder_temp','bed_temp','density']);
    materialTypeEl.addEventListener('input', function() {
      autoFillMaterialData(materialTypeEl.value, nfcFieldMap);
      // Average min/max temps into single fields
      var m = lookupMaterial(materialTypeEl.value);
      if (m) {
        var et = document.getElementById('extruder_temp');
        if (et && et.dataset.autoFilled !== 'false' && m.minPrintTemp && m.maxPrintTemp) {
          et.value = Math.round((m.minPrintTemp + m.maxPrintTemp) / 2);
          et.dataset.autoFilled = 'true';
        }
        var bt = document.getElementById('bed_temp');
        if (bt && bt.dataset.autoFilled !== 'false' && m.minBedTemp && m.maxBedTemp) {
          bt.value = Math.round((m.minBedTemp + m.maxBedTemp) / 2);
          bt.dataset.autoFilled = 'true';
        }
      }
    });
    loadMaterialDb().then(function(db) {
      var dl = document.getElementById('material-list');
      var existing = {};
      var opts = dl.querySelectorAll('option');
      for (var i = 0; i < opts.length; i++) existing[opts[i].value.toUpperCase()] = true;
      Object.keys(db).sort().forEach(function(key) {
        if (!existing[key]) {
          var opt = document.createElement('option');
          opt.value = db[key].material || key;
          dl.appendChild(opt);
        }
      });
    });

    function syncMaterialNameFromSelection() {
      var selectedText = materialTypeEl.value || 'PLA';
      if (!materialNameEl.value.trim() || materialNameEl.dataset.autoFilled === 'true') {
        materialNameEl.value = selectedText.toUpperCase();
        materialNameEl.dataset.autoFilled = 'true';
      }
    }

    function setResult(msg, type) {
      resultBox.textContent = msg;
      resultBox.className = type ? 'result ' + type : 'result';
    }

    materialTypeEl.addEventListener('input', syncMaterialNameFromSelection);

    materialNameEl.addEventListener('input', function() {
      materialNameEl.value = materialNameEl.value.toUpperCase();
      materialNameEl.dataset.autoFilled = 'false';
    });

    registerForm.addEventListener('reset', function() {
      setTimeout(function() {
        document.getElementById('colorPicker').value = '#ff0000';
        document.getElementById('colorHex').value = '#FF0000';
        materialNameEl.dataset.autoFilled = 'true';
        syncMaterialNameFromSelection();
        currentUid = '';
        uidValue.textContent = 'Place tag \u0026 click Read';
        resultBox.textContent = '';
        resultBox.className = '';
      }, 0);
    });

    var readAbort = null;

    readUidBtn.addEventListener('click', function() {
      if (readAbort) { readAbort = true; return; }
      readUidBtn.textContent = 'Waiting for tag\u2026';
      readUidBtn.disabled = true;
      uidValue.textContent = 'Place tag on scanner\u2026';
      readAbort = false;
      var attempts = 0;
      var maxAttempts = 60;

      function poll() {
        if (readAbort || attempts >= maxAttempts) {
          readUidBtn.textContent = 'Read Tag UID';
          readUidBtn.disabled = false;
          readAbort = null;
          if (attempts >= maxAttempts) {
            uidValue.textContent = 'Timed out \u2014 no tag detected';
          }
          return;
        }
        attempts++;
        fetch('/api/status')
          .then(function(r) { return r.json(); })
          .then(function(d) {
            if (d.present && d.uid) {
              currentUid = d.uid;
              uidValue.textContent = d.uid;
              uidValue.style.fontSize = '22px';
              readUidBtn.textContent = 'Read Tag UID';
              readUidBtn.disabled = false;
              readAbort = null;
            } else {
              setTimeout(poll, 500);
            }
          })
          .catch(function() {
            currentUid = '';
            uidValue.textContent = 'Error reading scanner';
            readUidBtn.textContent = 'Read Tag UID';
            readUidBtn.disabled = false;
            readAbort = null;
          });
      }
      poll();
    });

    async function registerFlow() {
      setResult('', '');

      if (!currentUid) {
        setResult('No UID read yet. Place the tag on the scanner and click Read Tag UID first.', 'error');
        return;
      }

      var manufacturer = readString('manufacturer');
      if (!manufacturer) {
        setResult('Manufacturer is required.', 'error');
        return;
      }

      var initialWeight = readRequiredNumber('initial_weight_g');
      var remainingWeight = readRequiredNumber('remaining_g');

      var color = normalizeHex(document.getElementById('colorHex').value);
      if (!color) {
        setResult('Color must be a valid 6-digit hex value.', 'error');
        return;
      }

      var materialTypeText = materialTypeEl.value || 'PLA';
      var materialName = readString('material_name');
      if (materialName) materialName = materialName.toUpperCase();

      var payload = {
        uid: currentUid,
        manufacturer: manufacturer,
        material: materialName || materialTypeText.toUpperCase(),
        material_name: materialName || '',
        color: color.replace('#', ''),
        initial_weight_g: initialWeight,
        remaining_g: remainingWeight,
        density: readPositiveNumber('density') || 0,
        diameter_mm: readPositiveNumber('diameter_mm') || 1.75,
        extruder_temp: readPositiveNumber('extruder_temp') || 0,
        bed_temp: readPositiveNumber('bed_temp') || 0
      };

      setResult('Registering in Spoolman\u2026', '');

      try {
        var result = await api('/api/register-uid', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });
        setResult('Spool #' + result.spool_id + ' registered successfully. UID: ' + currentUid, 'success');
      } catch (e) {
        setResult('Registration failed: ' + (e.message || e), 'error');
      }
    }

    registerForm.addEventListener('submit', function(e) {
      e.preventDefault();
      registerFlow();
    });

    syncMaterialNameFromSelection();

    (async function init() {
      try {
        var status = await api('/api/status');
        if (status.uid) {
          currentUid = status.uid;
          uidValue.textContent = status.uid;
        }
      } catch (e) {}
    })();
  </script>
</body>
</html>
)rawliteral";
