#pragma once

// TigerTag writer page served at GET /writer/tigertag
// Writes 40-byte TigerTag binary format to NTAG213/215 via POST /api/write-tigertag.

const char TIGERTAG_WRITER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>TigerTag Writer &mdash; SpoolSense</title>
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
      <a href="/troubleshooting">Troubleshooting</a>
      <a href="/config">Config</a>
    </nav>

    <section class="card" id="createView">
      <div class="card-head" style="display:flex;align-items:center;gap:16px">
        <img src="/img/tigertag.png" alt="TigerTag" style="height:48px;border-radius:8px" />
        <div>
          <h1 class="card-title">Create TigerTag</h1>
          <p class="card-subtitle">Write filament data to an NTAG213/215 tag in TigerTag format.</p>
        </div>
      </div>

      <div class="card-body">
        <div id="spoolmanPicker" style="background:var(--card-alt,#1e1e35);border:1px solid var(--border);border-radius:8px;padding:12px;margin-bottom:16px"></div>
        <form id="writerForm">
          <section>
            <h2 class="section-title">Filament</h2>
            <div class="grid-2">
              <div class="field">
                <label for="material_search">Material</label>
                <input id="material_search" name="material_search" list="material-list" placeholder="Type to search materials" required />
                <datalist id="material-list">
                  <option data-id="38219" value="PLA"></option>
                  <option data-id="46591" value="PLA+"></option>
                  <option data-id="38256" value="PETG"></option>
                  <option data-id="43518" value="TPU"></option>
                  <option data-id="20562" value="ABS"></option>
                  <option data-id="12844" value="ASA"></option>
                  <option data-id="30458" value="PC"></option>
                  <option data-id="15041" value="PCTG"></option>
                  <option data-id="30884" value="PP"></option>
                  <option data-id="56666" value="PA6"></option>
                  <option data-id="55796" value="PA12"></option>
                  <option data-id="59328" value="PA"></option>
                  <option data-id="26029" value="HIPS"></option>
                  <option data-id="52077" value="PET"></option>
                  <option data-id="45962" value="PVB"></option>
                  <option data-id="9483" value="PVA"></option>
                  <option data-id="29815" value="PEEK"></option>
                  <option data-id="53970" value="PEKK"></option>
                  <option data-id="56527" value="PEI"></option>
                  <option data-id="55279" value="PBT"></option>
                  <option data-id="33958" value="TPE"></option>
                  <option data-id="48310" value="PLA-CF"></option>
                  <option data-id="55418" value="PETG-CF"></option>
                  <option data-id="48815" value="PAHT-CF"></option>
                  <option data-id="12264" value="PA6-CF"></option>
                  <option data-id="10602" value="PLA Silk"></option>
                  <option data-id="48001" value="PLA Wood"></option>
                  <option data-id="65535" value="None"></option>
                </datalist>
                <input type="hidden" id="material_id" name="material_id" value="38219" />
              </div>

              <div class="field">
                <label for="brand_name">Brand</label>
                <input id="brand_name" name="brand_name" list="brand-list" placeholder="Type to search brands" required />
                <datalist id="brand-list">
                  <option data-id="65535" value="Generic"></option>
                  <option data-id="35123" value="Bambu Lab"></option>
                  <option data-id="46392" value="Prusa"></option>
                  <option data-id="50604" value="Polymaker"></option>
                  <option data-id="47930" value="eSun"></option>
                  <option data-id="51857" value="Sunlu"></option>
                  <option data-id="46203" value="Overture"></option>
                  <option data-id="3132" value="Hatchbox"></option>
                  <option data-id="26956" value="Creality"></option>
                  <option data-id="15962" value="Anycubic"></option>
                  <option data-id="57632" value="Elegoo"></option>
                  <option data-id="7812" value="Jayo"></option>
                  <option data-id="52222" value="ColorFabb"></option>
                  <option data-id="7980" value="Fillamentum"></option>
                  <option data-id="8182" value="Fiberlogy"></option>
                  <option data-id="53043" value="FormFutura"></option>
                  <option data-id="58410" value="AzureFilm"></option>
                  <option data-id="4344" value="MatterHackers"></option>
                  <option data-id="2" value="Proto-Pasta"></option>
                  <option data-id="58231" value="IC3D"></option>
                  <option data-id="39652" value="3DXTech"></option>
                  <option data-id="51443" value="BASF"></option>
                  <option data-id="9798" value="AMOLEN"></option>
                  <option data-id="28940" value="Eryone"></option>
                  <option data-id="63340" value="Flashforge"></option>
                  <option data-id="8384" value="Taulman3D"></option>
                </datalist>
                <input type="hidden" id="brand_id" name="brand_id" value="65535" />
                <div style="font-size:11px;color:var(--muted);margin-top:4px">
                  Brand not listed? Use Generic. <a href="https://github.com/TigerTag-Project/TigerTag-RFID-Guide/issues" target="_blank" rel="noopener noreferrer" style="color:var(--blue)">Request a new brand &rarr;</a>
                </div>
              </div>

              <div class="field">
                <label for="weight_g">Weight (g)</label>
                <input id="weight_g" name="weight_g" type="number" min="1" max="16777215" step="1" placeholder="1000" required />
              </div>

              <div class="field">
                <label>Color</label>
                <div class="color-row">
                  <input id="colorPicker" type="color" value="#ffffff" />
                  <input id="colorHex" name="color" type="text" value="#FFFFFF" maxlength="7" placeholder="#FFFFFF" required />
                </div>
              </div>
            </div>
          </section>

          <section>
            <h2 class="section-title">Properties</h2>
            <div class="grid-3">
              <div class="field">
                <label for="diameter_id">Diameter</label>
                <select id="diameter_id" name="diameter_id">
                  <option value="56">1.75 mm</option>
                  <option value="221">2.85 mm</option>
                </select>
              </div>

              <div class="field">
                <label for="aspect1_id">Aspect 1</label>
                <select id="aspect1_id" name="aspect1_id">
                  <option value="255">None</option>
                  <option value="104">Basic</option>
                  <option value="247">Matt</option>
                  <option value="129">Gloss</option>
                  <option value="134">Satin</option>
                  <option value="92">Silk</option>
                  <option value="126">Pearl</option>
                  <option value="64">Glitter</option>
                  <option value="67">Translucent</option>
                  <option value="21">Clear</option>
                  <option value="91">Glow</option>
                  <option value="216">Neon</option>
                  <option value="220">Pastel</option>
                  <option value="232">Marble</option>
                  <option value="238">Carbon</option>
                  <option value="123">Wood</option>
                  <option value="173">Stone</option>
                  <option value="145">Rainbow</option>
                  <option value="252">Bicolor</option>
                  <option value="24">Tricolor</option>
                  <option value="168">Thermoreactif</option>
                  <option value="97">Lithophane</option>
                </select>
              </div>

              <div class="field">
                <label for="aspect2_id">Aspect 2</label>
                <select id="aspect2_id" name="aspect2_id">
                  <option value="255">None</option>
                  <option value="104">Basic</option>
                  <option value="247">Matt</option>
                  <option value="129">Gloss</option>
                  <option value="134">Satin</option>
                  <option value="92">Silk</option>
                  <option value="126">Pearl</option>
                  <option value="64">Glitter</option>
                  <option value="67">Translucent</option>
                  <option value="21">Clear</option>
                  <option value="91">Glow</option>
                  <option value="216">Neon</option>
                  <option value="220">Pastel</option>
                  <option value="232">Marble</option>
                  <option value="238">Carbon</option>
                  <option value="123">Wood</option>
                  <option value="173">Stone</option>
                  <option value="145">Rainbow</option>
                  <option value="252">Bicolor</option>
                  <option value="24">Tricolor</option>
                  <option value="168">Thermoreactif</option>
                  <option value="97">Lithophane</option>
                </select>
              </div>
            </div>
          </section>

          <section>
            <button type="button" class="advanced-toggle" id="advancedToggle" aria-expanded="false">
              <span>Temperature settings</span>
              <span data-toggle-text>Show</span>
            </button>

            <div class="advanced-box hidden" id="advancedBox">
              <div class="grid-3">
                <div class="field">
                  <label for="nozzle_min">Nozzle Min (&deg;C)</label>
                  <input id="nozzle_min" name="nozzle_min" type="number" min="0" max="500" step="1" placeholder="190" />
                </div>
                <div class="field">
                  <label for="nozzle_max">Nozzle Max (&deg;C)</label>
                  <input id="nozzle_max" name="nozzle_max" type="number" min="0" max="500" step="1" placeholder="220" />
                </div>
                <div class="field">
                  <label for="bed_min">Bed Min (&deg;C)</label>
                  <input id="bed_min" name="bed_min" type="number" min="0" max="255" step="1" placeholder="55" />
                </div>
                <div class="field">
                  <label for="bed_max">Bed Max (&deg;C)</label>
                  <input id="bed_max" name="bed_max" type="number" min="0" max="255" step="1" placeholder="65" />
                </div>
                <div class="field">
                  <label for="dry_temp">Dry Temp (&deg;C)</label>
                  <input id="dry_temp" name="dry_temp" type="number" min="0" max="255" step="1" placeholder="45" />
                </div>
                <div class="field">
                  <label for="dry_time">Dry Time (hrs)</label>
                  <input id="dry_time" name="dry_time" type="number" min="0" max="255" step="1" placeholder="4" />
                </div>
              </div>
            </div>
          </section>

          <div id="spoolmanEnrichment" class="hidden" style="margin-top:16px;padding:14px;background:var(--card-alt,#1e1e35);border:1px solid var(--blue,#4a9eff);border-radius:8px;">
            <div style="display:flex;align-items:center;gap:8px;margin-bottom:4px">
              <span style="color:var(--blue,#4a9eff);font-size:11px;text-transform:uppercase;letter-spacing:1px;font-weight:600">Spoolman Enrichment</span>
              <span id="spoolmanMatchBadge" class="hidden" style="background:#4a9eff22;border:1px solid #4a9eff55;border-radius:3px;padding:0 6px;color:#4a9eff;font-size:10px"></span>
            </div>
            <p class="card-subtitle" style="font-size:11px;margin-bottom:10px">
              Extra data Spoolman stores that TigerTag format cannot &mdash; saved to Spoolman on write.
            </p>
            <div class="grid-2">
              <div class="field">
                <label for="enrich-density">Density (g/cm&sup3;)</label>
                <input id="enrich-density" type="number" placeholder="1.24" min="0.1" max="5" step="0.001" />
              </div>
              <div class="field">
                <label for="enrich-remaining">Remaining Weight (g)</label>
                <input id="enrich-remaining" type="number" placeholder="e.g. 1000" min="1" max="10000" />
              </div>
            </div>
          </div>

          <div class="write-warning">Keep the tag still &mdash; do not remove until writing is complete.</div>

          <div id="readPrompt" class="hidden write-warning" style="background:#0d2a1a;border-color:#2a7a4a;color:#4adf8a;text-align:center;padding:10px">
            Place tag on reader&hellip; <span style="font-size:11px;color:#5a9a6a">hold still until detected</span>
          </div>

          <div class="actions">
            <button type="submit" class="btn-primary" id="writeBtn">Write Tag</button>
            <button type="button" class="btn-secondary" id="readBtn">Read</button>
            <button type="reset" class="btn-ghost">Clear</button>
          </div>
          <p class="card-subtitle" style="margin-top:4px;font-size:11px">
            Use <strong>Read</strong> to load an existing tag before overwriting.
          </p>
        </form>
      </div>
    </section>

    <section class="card hidden" id="statusView">
      <div class="card-head">
        <h1 class="card-title">Writing TigerTag</h1>
        <p class="card-subtitle">Keep the tag on the scanner until verification completes.</p>
      </div>

      <div class="card-body">
        <div class="status-wrap">
          <div class="status-banner" id="statusBanner">Starting write flow&hellip;</div>

          <div class="steps">
            <div class="step" id="step-wait">
              <div class="dot"></div>
              <div>
                <div class="step-title">Waiting for tag</div>
                <div class="step-sub">Place an NTAG213 or NTAG215 on the scanner.</div>
              </div>
            </div>

            <div class="step" id="step-detect">
              <div class="dot"></div>
              <div>
                <div class="step-title">Tag detected</div>
                <div class="step-sub">Tag presence and UID confirmed.</div>
              </div>
            </div>

            <div class="step" id="step-write">
              <div class="dot"></div>
              <div>
                <div class="step-title">Writing data</div>
                <div class="step-sub">Writing TigerTag binary to pages 4-13.</div>
              </div>
            </div>

            <div class="step" id="step-verify">
              <div class="dot"></div>
              <div>
                <div class="step-title">Verifying write</div>
                <div class="step-sub">Reading back tag to confirm data matches.</div>
              </div>
            </div>
          </div>

          <div class="result" id="resultBox">Waiting&hellip;</div>

          <div class="actions">
            <button type="button" class="btn-secondary hidden" id="backBtn">Back to Form</button>
            <button type="button" class="btn-primary hidden" id="anotherBtn">Write Another</button>
          </div>
        </div>
      </div>
    </section>

    <div class="footer-note">TigerTag V1.0 Maker format &mdash; 40 bytes, pages 4-13</div>
  </div>

  <script src="/js/shared.js"></script>
  <script>
    var createView = document.getElementById('createView');
    var statusView = document.getElementById('statusView');
    var writerForm = document.getElementById('writerForm');
    var backBtn = document.getElementById('backBtn');
    var anotherBtn = document.getElementById('anotherBtn');

    var STEP_IDS = ['step-wait', 'step-detect', 'step-write', 'step-verify'];

    syncColorPicker('colorPicker', 'colorHex');
    setupAdvancedToggle('advancedToggle', 'advancedBox');

    // Sync brand name input → hidden brand_id
    var brandNameEl = document.getElementById('brand_name');
    var brandIdEl = document.getElementById('brand_id');
    var brandList = document.getElementById('brand-list');
    if (brandNameEl && brandIdEl && brandList) {
      brandNameEl.addEventListener('input', function() {
        var opts = brandList.querySelectorAll('option');
        brandIdEl.value = '65535'; // default to Generic
        for (var i = 0; i < opts.length; i++) {
          if (opts[i].value === brandNameEl.value) {
            brandIdEl.value = opts[i].dataset.id;
            break;
          }
        }
      });
    }

    // Fetch TigerTag API data — fallback to hardcoded options on failure
    var materialData = {};  // Store full material data for auto-fill on selection

    function validateResponse(data, requiredFields) {
      if (!Array.isArray(data)) return null;
      var valid = data.filter(function(item) {
        if (typeof item !== 'object' || item === null) return false;
        return requiredFields.every(function(f) {
          return f.type === 'number' ? (typeof item[f.key] === 'number' && isFinite(item[f.key]))
               : f.type === 'string' ? (typeof item[f.key] === 'string' && item[f.key].trim().length > 0)
               : true;
        });
      });
      return valid;
    }

    function populateSelect(selectId, items, valueFn, labelFn) {
      var el = document.getElementById(selectId);
      var currentVal = el.value;
      var hardcodedCount = el.options.length;
      if (items.length < hardcodedCount) return;
      var frag = document.createDocumentFragment();
      items.forEach(function(item) {
        var opt = document.createElement('option');
        opt.value = valueFn(item);
        opt.textContent = labelFn(item);
        frag.appendChild(opt);
      });
      el.innerHTML = '';
      el.appendChild(frag);
      if (currentVal) {
        el.value = currentVal;
        if (!el.value) el.selectedIndex = 0;
      }
    }

    // Sync material search → hidden material_id
    var materialSearchEl = document.getElementById('material_search');
    var materialIdEl = document.getElementById('material_id');
    var materialListEl = document.getElementById('material-list');

    function syncMaterialId() {
      var opts = materialListEl.querySelectorAll('option');
      materialIdEl.value = '38219'; // default PLA
      for (var i = 0; i < opts.length; i++) {
        if (opts[i].value === materialSearchEl.value) {
          materialIdEl.value = opts[i].dataset.id;
          break;
        }
      }
    }

    trackAutoFill(['nozzle_min','nozzle_max','bed_min','bed_max','dry_temp','dry_time']);

    function ttSetOrClear(el, value) {
      if (!el || el.dataset.autoFilled === 'false') return;
      el.value = (value !== undefined && value !== null) ? value : '';
      el.dataset.autoFilled = 'true';
    }

    function autoFillFromMaterial() {
      syncMaterialId();
      var id = materialIdEl.value;
      var m = materialData[id];
      var r = (m && m.recommended) ? m.recommended : {};
      ttSetOrClear(document.getElementById('nozzle_min'), r.nozzleTempMin);
      ttSetOrClear(document.getElementById('nozzle_max'), r.nozzleTempMax);
      ttSetOrClear(document.getElementById('bed_min'), r.bedTempMin);
      ttSetOrClear(document.getElementById('bed_max'), r.bedTempMax);
      ttSetOrClear(document.getElementById('dry_temp'), r.dryTemp);
      ttSetOrClear(document.getElementById('dry_time'), r.dryTime);
    }

    materialSearchEl.addEventListener('input', autoFillFromMaterial);

    // Pre-fill from scanned tag if present
    prefillFromTag({
      material: 'material_search',
      color: 'colorHex',
      colorPicker: 'colorPicker',
      manufacturer: 'brand_search',
      weight: 'weight_g',
      nozzle_min: 'nozzle_min',
      nozzle_max: 'nozzle_max',
      bed_min: 'bed_min',
      bed_max: 'bed_max',
      dry_temp: 'dry_temp',
      dry_time: 'dry_time'
    }).then(function(d) {
      if (!d) return;
      // Sync hidden material_id from material name
      if (d.tigertag_material_id !== undefined) {
        document.getElementById('material_id').value = d.tigertag_material_id;
      } else {
        syncMaterialId();
      }
      // Sync hidden brand_id from brand name
      if (d.tigertag_brand_id !== undefined) {
        document.getElementById('brand_id').value = d.tigertag_brand_id;
      }
    });

    (async function loadTigerTagAPI() {
      try {
        var resp = await fetch('https://raw.githubusercontent.com/TigerTag-Project/TigerTag-RFID-Guide/main/database/id_material.json');
        if (resp.ok) {
          var raw = await resp.json();
          var materials = validateResponse(raw, [
            {key: 'id', type: 'number'},
            {key: 'label', type: 'string'}
          ]);
          if (materials) {
            materials.forEach(function(m) { materialData[m.id] = m; });
            materials.sort(function(a, b) { return a.label.localeCompare(b.label); });
            var dl = document.getElementById('material-list');
            if (dl && materials.length > dl.options.length) {
              dl.innerHTML = '';
              materials.forEach(function(m) {
                var opt = document.createElement('option');
                opt.value = m.label;
                opt.dataset.id = m.id;
                dl.appendChild(opt);
              });
            }
          }
        }
      } catch(e) { /* keep hardcoded fallback */ }

      try {
        var resp2 = await fetch('https://raw.githubusercontent.com/TigerTag-Project/TigerTag-RFID-Guide/main/database/id_brand.json');
        if (resp2.ok) {
          var raw2 = await resp2.json();
          var brands = validateResponse(raw2, [
            {key: 'id', type: 'number'},
            {key: 'name', type: 'string'}
          ]);
          if (brands) {
            brands.sort(function(a, b) { return a.name.localeCompare(b.name); });
            var dl = document.getElementById('brand-list');
            if (dl && brands.length > dl.options.length) {
              dl.innerHTML = '';
              brands.forEach(function(b) {
                var opt = document.createElement('option');
                opt.value = b.name;
                opt.dataset.id = b.id;
                dl.appendChild(opt);
              });
            }
          }
        }
      } catch(e) { /* keep hardcoded fallback */ }
    })();

    function showStatusView() {
      createView.classList.add('hidden');
      statusView.classList.remove('hidden');
      backBtn.classList.add('hidden');
      anotherBtn.classList.add('hidden');
    }

    function showCreateView() {
      statusView.classList.add('hidden');
      createView.classList.remove('hidden');
    }

    function intVal(id, fallback) {
      var raw = document.getElementById(id).value.trim();
      if (raw === '') return fallback;
      var n = parseInt(raw, 10);
      return isNaN(n) ? fallback : n;
    }

    function buildPayload(uid) {
      var color = normalizeHex(document.getElementById('colorHex').value);
      if (!color) throw new Error('Color must be a valid 6-digit hex value');

      var r = parseInt(color.substr(1,2), 16);
      var g = parseInt(color.substr(3,2), 16);
      var b = parseInt(color.substr(5,2), 16);

      return {
        uid: uid || '',
        material_id: intVal('material_id', 38219),
        brand_id: intVal('brand_id', 65535),
        weight_g: intVal('weight_g', 1000),
        color_r: r,
        color_g: g,
        color_b: b,
        color_a: 255,
        diameter_id: intVal('diameter_id', 56),
        aspect1_id: intVal('aspect1_id', 255),
        aspect2_id: intVal('aspect2_id', 255),
        nozzle_min: intVal('nozzle_min', 0),
        nozzle_max: intVal('nozzle_max', 0),
        bed_min: intVal('bed_min', 0),
        bed_max: intVal('bed_max', 0),
        dry_temp: intVal('dry_temp', 0),
        dry_time: intVal('dry_time', 0)
      };
    }

    async function waitForTag(timeoutMs) {
      var deadline = Date.now() + timeoutMs;
      setStepState('step-wait', 'active');
      setBanner('statusBanner', 'Waiting for tag\u2026');
      setResult('resultBox', 'Place and hold an NTAG tag on the scanner.', '');

      while (Date.now() < deadline) {
        var status = await api('/api/status');
        if (status.present) {
          setStepState('step-wait', 'done');
          return status;
        }
        await sleep(500);
      }

      setStepState('step-wait', 'error');
      throw new Error('No tag detected. Place the tag on the scanner and try again.');
    }

    async function writeFlow() {
      resetAllSteps(STEP_IDS);
      showStatusView();

      try {
        var presentStatus = await waitForTag(8000);

        setStepState('step-detect', 'active');
        setBanner('statusBanner', 'Tag detected.');
        setResult('resultBox', 'UID: ' + (presentStatus.uid || 'Unknown'), '');
        await sleep(250);
        setStepState('step-detect', 'done');

        var payload = buildPayload(presentStatus.uid);

        setStepState('step-write', 'active');
        setBanner('statusBanner', 'Writing TigerTag data\u2026');
        setResult('resultBox', 'Sending TigerTag binary to scanner.', '');
        await api('/api/write-tigertag', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });
        setStepState('step-write', 'done');

        setStepState('step-verify', 'active');
        setBanner('statusBanner', 'Verifying write\u2026');
        setResult('resultBox', 'Reading tag back to confirm data matches.', '');

        var verifyDeadline = Date.now() + 15000;
        while (Date.now() < verifyDeadline) {
          await sleep(500);
          var status = await api('/api/status');
          if (status.present && status.tag_kind === 'TigerTag' && status.tigertag) {
            var tt = status.tigertag;
            if (tt.material_id === payload.material_id && tt.brand_id === payload.brand_id) {
              // Wait for the full read cycle to complete before declaring success
              setBanner('statusBanner', 'Tag verified \u2014 hold for a moment\u2026');
              await sleep(2000);
              setStepState('step-verify', 'done');
              setBanner('statusBanner', 'Write complete \u2014 safe to remove tag.');
              setResult('resultBox', 'TigerTag data written and verified successfully.', 'success');
              await saveEnrichment(presentStatus.uid);
              backBtn.classList.remove('hidden');
              anotherBtn.classList.remove('hidden');
              return;
            }
          }
        }

        setStepState('step-verify', 'error');
        throw new Error('Verification timed out. Keep the tag on the scanner and try again.');
      } catch (err) {
        var msg = err && err.message ? err.message : 'Write failed';
        setBanner('statusBanner', 'Write failed.');
        setResult('resultBox', msg, 'error');

        STEP_IDS.forEach(function(id) {
          var el = document.getElementById(id);
          if (el && el.classList.contains('active')) {
            setStepState(id, 'error');
          }
        });

        backBtn.classList.remove('hidden');
      }
    }

    writerForm.addEventListener('submit', function(e) {
      e.preventDefault();
      writeFlow();
    });

    backBtn.addEventListener('click', showCreateView);
    anotherBtn.addEventListener('click', showCreateView);

    renderSpoolmanPicker('spoolmanPicker', {
      material: 'material_search',
      manufacturer: 'brand_name',
      color: 'colorHex',
      colorPicker: 'colorPicker',
      remaining: 'weight_g',
      diameter: 'diameter_id',
      nozzle_min: 'nozzle_min',
      nozzle_max: 'nozzle_max',
      bed_min: 'bed_min',
      bed_max: 'bed_max',
      density: 'enrich-density'
    });

    // Show enrichment section once spool picker loads (confirms Spoolman is configured)
    var _enrichCheck = setInterval(function() {
      if (document.querySelector('#spoolmanPickerSearch')) {
        document.getElementById('spoolmanEnrichment').classList.remove('hidden');
        clearInterval(_enrichCheck);
      }
    }, 500);
    setTimeout(function() { clearInterval(_enrichCheck); }, 15000);

    var readBtn = document.getElementById('readBtn');
    var writeBtn = document.getElementById('writeBtn');
    var readWaiting = false;

    function setReadWaiting(active) {
      readWaiting = active;
      writeBtn.disabled = active;
      if (active) {
        readBtn.textContent = 'Cancel';
        readBtn.onclick = cancelRead;
        document.getElementById('readPrompt').classList.remove('hidden');
      } else {
        readBtn.textContent = 'Read';
        readBtn.onclick = startRead;
        document.getElementById('readPrompt').classList.add('hidden');
      }
    }

    function cancelRead() {
      readWaiting = false;
      setReadWaiting(false);
    }

    function showMatchBadge(text) {
      var badge = document.getElementById('spoolmanMatchBadge');
      badge.textContent = text;
      badge.classList.remove('hidden');
    }

    function setVal(id, val) {
      var el = document.getElementById(id);
      if (el && val !== undefined && val !== null) el.value = val;
    }

    function fillEnrichmentFromStatus(status) {
      var sp = status.spoolman || {};
      if (sp.remaining_g !== undefined) setVal('enrich-remaining', sp.remaining_g.toFixed(1));
      if (sp.density && sp.density > 0) setVal('enrich-density', sp.density);
    }

    async function startRead() {
      setReadWaiting(true);
      var deadline = Date.now() + 30000;
      while (readWaiting && Date.now() < deadline) {
        try {
          var status = await fetch('/api/status').then(r => r.json());
          if (status.present && status.tag_kind === 'TigerTag') {
            var tt = status.tigertag || {};
            setVal('material_search', tt.material_name || '');
            setVal('brand_name', tt.brand_name || '');
            if (tt.color_hex) {
              var c = tt.color_hex.startsWith('#') ? tt.color_hex : '#' + tt.color_hex;
              setVal('colorHex', c);
              setVal('colorPicker', c);
            }
            if (tt.weight_g) setVal('weight_g', tt.weight_g);
            if (tt.nozzle_temp_min) setVal('nozzle_min', tt.nozzle_temp_min);
            if (tt.nozzle_temp_max) setVal('nozzle_max', tt.nozzle_temp_max);
            if (tt.bed_temp_min) setVal('bed_min', tt.bed_temp_min);
            if (tt.bed_temp_max) setVal('bed_max', tt.bed_temp_max);
            if (tt.dry_temp) setVal('dry_temp', tt.dry_temp);
            if (tt.dry_time_hours) setVal('dry_time', tt.dry_time_hours);
            if (tt.material_id !== undefined) document.getElementById('material_id').value = tt.material_id;
            if (tt.brand_id !== undefined) document.getElementById('brand_id').value = tt.brand_id;
            // Sync material ID from name if not set directly
            syncMaterialId();
            fillEnrichmentFromStatus(status);
            if (status.spoolman && status.spoolman.spool_id > 0) {
              showMatchBadge('Spool #' + status.spoolman.spool_id + ' matched');
            } else {
              showMatchBadge('no Spoolman match');
            }
            break;
          } else if (status.present) {
            showMatchBadge('wrong format \u2014 expected TigerTag');
            break;
          }
        } catch(e) {}
        await new Promise(r => setTimeout(r, 500));
      }
      setReadWaiting(false);
    }

    readBtn.onclick = startRead;

    function enrichmentHasData() {
      var ids = ['enrich-remaining', 'enrich-density'];
      return ids.some(function(id) {
        var el = document.getElementById(id);
        return el && el.value && parseFloat(el.value) > 0;
      });
    }

    async function saveEnrichment(uid) {
      if (!enrichmentHasData()) return;

      var manufacturer = document.getElementById('brand_name').value.trim();
      var material = document.getElementById('material_search').value.trim();
      var colorHex = document.getElementById('colorHex').value || '';
      var remainingG = parseFloat(document.getElementById('enrich-remaining').value) || 0;
      var density = parseFloat(document.getElementById('enrich-density').value) || 0;
      var nozzleMin = parseInt(document.getElementById('nozzle_min').value) || 0;
      var nozzleMax = parseInt(document.getElementById('nozzle_max').value) || 0;
      var bedMin = parseInt(document.getElementById('bed_min').value) || 0;
      var bedMax = parseInt(document.getElementById('bed_max').value) || 0;
      var bedTemp = bedMin || bedMax;
      var nozzleTemp = nozzleMin || nozzleMax;

      var vendorId = -1;
      if (manufacturer) {
        try {
          var vr = await fetch('/api/spoolman/find-vendor?name=' + encodeURIComponent(manufacturer)).then(function(r) { return r.json(); });
          if (vr.found) {
            var confirmed = confirm('Found existing manufacturer "' + vr.name + '" in Spoolman. Use it?');
            vendorId = confirmed ? vr.id : -2;
          }
        } catch(e) {}
      }

      var filamentId = -1;
      if (vendorId > 0 && material) {
        try {
          var fr = await fetch('/api/spoolman/find-filament?vendor_id=' + vendorId
                           + '&material=' + encodeURIComponent(material)
                           + (colorHex ? '&color_hex=' + encodeURIComponent(colorHex.replace('#','')) : '')).then(function(r) { return r.json(); });
          if (fr.found) {
            var fconfirmed = confirm('Found existing filament "' + fr.name + '" in Spoolman. Use it?');
            filamentId = fconfirmed ? fr.id : -2;
          }
        } catch(e) {}
      }

      try {
        var resp = await fetch('/api/spoolman/save-enrichment', {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({
            uid: uid,
            manufacturer: manufacturer,
            material: material,
            color_hex: colorHex.replace('#', ''),
            remaining_g: remainingG,
            bed_temp: bedTemp,
            nozzle_temp: nozzleTemp,
            diameter_mm: 0,
            density: density,
            vendor_id: vendorId,
            filament_id: filamentId
          })
        });
        var result = await resp.json();
        if (!resp.ok || result.success === false) {
          throw new Error(result.error || 'save failed');
        }
        setBanner('statusBanner', 'Tag written \u2713 Spoolman enrichment saved \u2713');
      } catch(e) {
        setBanner('statusBanner', 'Tag written \u2713 Spoolman save failed \u2014 check connection');
      }
    }

  </script>
</body>
</html>
)rawliteral";
