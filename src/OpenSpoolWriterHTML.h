#pragma once

// OpenSpool writer page served at GET /writer/openspool
// Writes NDEF-wrapped OpenSpool JSON payload to NTAG215/216 via POST /api/write-openspool.

const char OPENSPOOL_WRITER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>OpenSpool Writer &mdash; SpoolSense</title>
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
      <a href="/writer/openspool">OpenSpool</a>
      <a href="/register/uid">NFC+</a>
      <a href="/update">Update</a>
      <a href="/config">Config</a>
    </nav>

    <section class="card">
      <div class="card-head">
        <div>
          <h1 class="card-title">Create OpenSpool Tag</h1>
          <p class="card-subtitle">Write filament data to an NTAG215/216 tag in OpenSpool format.</p>
        </div>
      </div>

      <div class="card-body">
        <div id="spoolmanPicker" style="background:var(--card-alt,#1e1e35);border:1px solid var(--border);border-radius:8px;padding:12px;margin-bottom:16px"></div>
        <form id="writerForm">
          <section>
            <h2 class="section-title">Filament</h2>
            <div class="grid-2">
              <div class="field">
                <label for="brand">Brand</label>
                <input id="brand" type="text" placeholder="e.g. Sunlu" required />
              </div>
              <div class="field">
                <label for="type">Material Type</label>
                <select id="type" required>
                  <option value="PLA">PLA</option>
                  <option value="PETG">PETG</option>
                  <option value="ABS">ABS</option>
                  <option value="ASA">ASA</option>
                  <option value="TPU">TPU</option>
                  <option value="PA">Nylon (PA)</option>
                  <option value="PC">PC</option>
                  <option value="PCTG">PCTG</option>
                  <option value="HIPS">HIPS</option>
                  <option value="PVB">PVB</option>
                </select>
              </div>
            </div>
          </section>

          <section>
            <h2 class="section-title">Color</h2>
            <div class="grid-2">
              <div class="field">
                <label for="color_hex">Color (hex)</label>
                <input id="color_hex" type="color" value="#FF0000" />
              </div>
              <div class="field">
                <label>Preview</label>
                <div id="colorPreview" style="width:48px;height:48px;border-radius:8px;border:1px solid var(--border);background:#FF0000"></div>
              </div>
            </div>
          </section>

          <section>
            <h2 class="section-title">Temperature</h2>
            <div class="grid-2">
              <div class="field">
                <label for="min_temp">Min Nozzle Temp (&deg;C)</label>
                <input id="min_temp" type="number" value="210" min="0" max="400" />
              </div>
              <div class="field">
                <label for="max_temp">Max Nozzle Temp (&deg;C)</label>
                <input id="max_temp" type="number" value="230" min="0" max="400" />
              </div>
            </div>
          </section>

          <div style="margin-top:16px">
            <button type="submit" class="btn" id="writeBtn">Write to Tag</button>
            <div id="writeResult" class="result hidden"></div>
          </div>
        </form>
      </div>
    </section>
  </div>

  <script src="/js/shared.js"></script>
  <script>
    document.getElementById('color_hex').addEventListener('input', function() {
      document.getElementById('colorPreview').style.background = this.value;
    });

    // Spoolman spool picker (if configured)
    if (typeof renderSpoolmanPicker === 'function') {
      renderSpoolmanPicker('spoolmanPicker', function(spool) {
        if (spool.filament) {
          var f = spool.filament;
          if (f.vendor && f.vendor.name) document.getElementById('brand').value = f.vendor.name;
          if (f.material) document.getElementById('type').value = f.material;
          if (f.color_hex) {
            document.getElementById('color_hex').value = '#' + f.color_hex;
            document.getElementById('colorPreview').style.background = '#' + f.color_hex;
          }
          if (f.settings_extruder_temp) {
            document.getElementById('min_temp').value = f.settings_extruder_temp;
            document.getElementById('max_temp').value = f.settings_extruder_temp;
          }
        }
      });
    }

    document.getElementById('writerForm').addEventListener('submit', function(e) {
      e.preventDefault();
      var btn = document.getElementById('writeBtn');
      var result = document.getElementById('writeResult');
      btn.disabled = true;
      btn.textContent = 'Writing...';

      var colorHex = document.getElementById('color_hex').value.replace('#', '').toUpperCase();

      var body = {
        protocol: 'openspool',
        version: '1.0',
        type: document.getElementById('type').value,
        color_hex: colorHex,
        brand: document.getElementById('brand').value.trim(),
        min_temp: document.getElementById('min_temp').value,
        max_temp: document.getElementById('max_temp').value
      };

      fetch('/api/write-openspool', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body)
      }).then(function(r) { return r.json(); })
        .then(function(data) {
          if (data.success) {
            result.textContent = 'Tag written successfully!';
            result.className = 'result success';
          } else {
            result.textContent = data.error || 'Write failed';
            result.className = 'result error';
          }
          result.classList.remove('hidden');
          btn.disabled = false;
          btn.textContent = 'Write to Tag';
        }).catch(function(err) {
          result.textContent = 'Error: ' + err.message;
          result.className = 'result error';
          result.classList.remove('hidden');
          btn.disabled = false;
          btn.textContent = 'Write to Tag';
        });
    });
  </script>
</body>
</html>
)rawliteral";
