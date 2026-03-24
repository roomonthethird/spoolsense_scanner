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
  <link rel="stylesheet" href="/css/shared.css" />
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
                <select id="material_type" name="material_type" required>
                  <option value="0">PLA</option>
                  <option value="1">PETG</option>
                  <option value="2">TPU</option>
                  <option value="3">ABS</option>
                  <option value="4">ASA</option>
                  <option value="5">PC</option>
                  <option value="6">PCTG</option>
                  <option value="7">PP</option>
                  <option value="8">PA6 (Nylon 6)</option>
                  <option value="9">PA11 (Nylon 11)</option>
                  <option value="10">PA12 (Nylon 12)</option>
                  <option value="11">PA66 (Nylon 66)</option>
                  <option value="12">CPE</option>
                  <option value="13">TPE</option>
                  <option value="14">HIPS</option>
                  <option value="15">PHA</option>
                  <option value="16">PET</option>
                  <option value="17">PEI</option>
                  <option value="18">PBT</option>
                  <option value="19">PVB</option>
                  <option value="20">PVA</option>
                  <option value="21">PEKK</option>
                  <option value="22">PEEK</option>
                </select>
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
                  <label for="min_print_temp">Min Print Temp (&deg;C)</label>
                  <input id="min_print_temp" name="min_print_temp" type="number" min="0" step="1" placeholder="205" />
                </div>

                <div class="field">
                  <label for="max_print_temp">Max Print Temp (&deg;C)</label>
                  <input id="max_print_temp" name="max_print_temp" type="number" min="0" step="1" placeholder="225" />
                </div>

                <div class="field">
                  <label for="min_bed_temp">Min Bed Temp (&deg;C)</label>
                  <input id="min_bed_temp" name="min_bed_temp" type="number" min="0" step="1" placeholder="55" />
                </div>

                <div class="field">
                  <label for="max_bed_temp">Max Bed Temp (&deg;C)</label>
                  <input id="max_bed_temp" name="max_bed_temp" type="number" min="0" step="1" placeholder="65" />
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

  <script src="/js/shared.js"></script>
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

    function syncMaterialNameFromSelection() {
      var selectedText = materialTypeEl.options[materialTypeEl.selectedIndex].text;
      if (!materialNameEl.value.trim() || materialNameEl.dataset.autoFilled === 'true') {
        materialNameEl.value = selectedText.toUpperCase();
        materialNameEl.dataset.autoFilled = 'true';
      }
    }

    function setResult(msg, type) {
      resultBox.textContent = msg;
      resultBox.className = type ? 'result ' + type : 'result';
    }

    materialTypeEl.addEventListener('change', syncMaterialNameFromSelection);

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

    readUidBtn.addEventListener('click', function() {
      uidValue.textContent = 'Reading\u2026';
      fetch('/api/status')
        .then(function(r) { return r.json(); })
        .then(function(d) {
          if (d.uid) {
            currentUid = d.uid;
            uidValue.textContent = d.uid;
          } else {
            currentUid = '';
            uidValue.textContent = 'No tag detected';
          }
        })
        .catch(function() {
          currentUid = '';
          uidValue.textContent = 'Error reading scanner';
        });
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

      setResult('Fetching Spoolman URL\u2026', '');

      var spoolmanUrl = '';
      try {
        var statusData = await api('/api/status');
        spoolmanUrl = (statusData.spoolman_url || '').replace(/\/$/, '');
      } catch (e) {
        setResult('Could not reach scanner to get Spoolman URL.', 'error');
        return;
      }

      if (!spoolmanUrl) {
        setResult('Spoolman URL is not configured. Set it in Config.', 'error');
        return;
      }

      var materialTypeText = materialTypeEl.options[materialTypeEl.selectedIndex].text;
      var materialName = readString('material_name');
      if (materialName) materialName = materialName.toUpperCase();
      var filamentName = (materialName || materialTypeText.toUpperCase()) + ' ' + manufacturer;

      var density = readPositiveNumber('density');
      var diameter = readPositiveNumber('diameter_mm');
      var minPrint = readPositiveNumber('min_print_temp');
      var maxPrint = readPositiveNumber('max_print_temp');
      var minBed = readPositiveNumber('min_bed_temp');
      var maxBed = readPositiveNumber('max_bed_temp');

      // Step 1: find or create vendor
      setResult('Looking up vendor in Spoolman\u2026', '');
      var vendorId = null;
      try {
        var vendorRes = await fetch(spoolmanUrl + '/api/v1/vendor?name=' + encodeURIComponent(manufacturer));
        if (vendorRes.ok) {
          var vendors = await vendorRes.json();
          if (Array.isArray(vendors) && vendors.length > 0) {
            vendorId = vendors[0].id;
          }
        }
      } catch (e) { /* vendor lookup optional */ }

      // Step 2: create filament
      setResult('Creating filament in Spoolman\u2026', '');
      var filamentPayload = {
        name: filamentName,
        material: materialName || materialTypeText.toUpperCase()
      };
      if (vendorId !== null) filamentPayload.vendor_id = vendorId;
      if (density !== undefined) filamentPayload.density = density;
      if (diameter !== undefined) filamentPayload.diameter = diameter;
      if (minPrint !== undefined) filamentPayload.settings_extruder_temp = minPrint;
      if (maxPrint !== undefined) filamentPayload.settings_extruder_temp_max = maxPrint;
      if (minBed !== undefined) filamentPayload.settings_bed_temp = minBed;
      if (maxBed !== undefined) filamentPayload.settings_bed_temp_max = maxBed;
      filamentPayload.color_hex = color.replace('#', '');

      var filamentId = null;
      try {
        var filamentRes = await fetch(spoolmanUrl + '/api/v1/filament', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(filamentPayload)
        });
        if (!filamentRes.ok) {
          var errText = await filamentRes.text();
          setResult('Failed to create filament: ' + errText, 'error');
          return;
        }
        var filamentData = await filamentRes.json();
        filamentId = filamentData.id;
      } catch (e) {
        setResult('Error creating filament: ' + (e.message || e), 'error');
        return;
      }

      // Step 3: create spool
      setResult('Creating spool in Spoolman\u2026', '');
      var spoolPayload = {
        filament_id: filamentId,
        remaining_weight: remainingWeight,
        initial_weight: initialWeight,
        extra: { nfc_id: currentUid }
      };

      try {
        var spoolRes = await fetch(spoolmanUrl + '/api/v1/spool', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(spoolPayload)
        });
        if (!spoolRes.ok) {
          var spoolErr = await spoolRes.text();
          setResult('Failed to create spool: ' + spoolErr, 'error');
          return;
        }
        var spoolData = await spoolRes.json();
        setResult('Spool #' + spoolData.id + ' registered successfully. UID: ' + currentUid, 'success');
      } catch (e) {
        setResult('Error creating spool: ' + (e.message || e), 'error');
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
