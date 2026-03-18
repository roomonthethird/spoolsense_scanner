#pragma once

// Placeholder HTML served at GET / for the SpoolSense tag writer web UI.
// Replace the content of TAG_WRITER_HTML with your finished page before flashing.
//
// API endpoints available to the page:
//   GET  /api/status      — current tag state JSON
//   POST /api/write-tag   — write all fields to tag (JSON body)
//   POST /api/format-tag  — format a blank tag (optional JSON body: {"uid":"..."})
//
// POST /api/write-tag JSON body (all fields optional except core ones):
//   {
//     "uid":             "DAD4E374080104E0",  // must match tag present (empty = skip check)
//     "material_type":   0,                   // OPT_MATERIAL_TYPE_* integer (0=PLA 1=PETG 2=TPU 3=ABS 4=ASA 5=PC ...)
//     "material_name":   "Royal Blue PLA",    // custom display name (optional)
//     "color":           "#FF0000",           // hex color string
//     "manufacturer":    "Prusament",
//     "initial_weight_g": 1000.0,
//     "remaining_g":     750.0,
//     "density":         1.24,               // g/cm³ (optional, 0 = skip)
//     "diameter_mm":     1.75,               // mm (optional, 0 = skip)
//     "min_print_temp":  190,                // °C (optional, 0 = skip)
//     "max_print_temp":  220,                // °C (optional, 0 = skip)
//     "preheat_temp":    210,                // °C (optional, 0 = skip)
//     "min_bed_temp":    25,                 // °C (optional, 0 = skip)
//     "max_bed_temp":    60,                 // °C (optional, 0 = skip)
//     "spoolman_id":     -1                  // omit or -1 to skip
//   }
//
// GET /api/status response:
//   {
//     "present": true,
//     "uid": "DAD4E374080104E0",
//     "tag_data_valid": true,
//     "material_type": 0,
//     "material_name": "PLA",
//     "color": "#FF0000",
//     "manufacturer": "Prusament",
//     "remaining_g": 750.0,
//     "initial_weight_g": 1000.0,
//     "spoolman_id": -1
//   }

const char TAG_WRITER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>SpoolSense Writer</title>
  <style>
    :root{
      --bg:#0b0b0d;
      --panel:#141519;
      --panel-2:#1a1c21;
      --border:#2a2e36;
      --text:#f4f4f5;
      --muted:#a1a1aa;
      --red:#dc2626;
      --red-2:#ef4444;
      --green:#22c55e;
      --blue:#3b82f6;
      --radius:16px;
      --shadow:0 16px 40px rgba(0,0,0,.35);
    }

    *{box-sizing:border-box}
    html,body{margin:0;padding:0}
    body{
      font-family:Inter,system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;
      background:linear-gradient(180deg,#09090b,#111216);
      color:var(--text);
      min-height:100vh;
    }

    .wrap{
      max-width:860px;
      margin:0 auto;
      padding:24px 18px 40px;
    }

    .brand{
      display:flex;
      align-items:center;
      justify-content:center;
      margin:6px 0 22px;
    }

    .card{
      background:linear-gradient(180deg,var(--panel),var(--panel-2));
      border:1px solid var(--border);
      border-radius:var(--radius);
      box-shadow:var(--shadow);
      overflow:hidden;
    }

    .card-head{
      padding:18px 20px;
      border-bottom:1px solid var(--border);
    }

    .card-title{
      margin:0;
      font-size:22px;
      font-weight:800;
      letter-spacing:-.02em;
    }

    .card-subtitle{
      margin:6px 0 0;
      color:var(--muted);
      font-size:14px;
    }

    .card-body{
      padding:20px;
    }

    .hidden{display:none !important}

    form{
      display:grid;
      gap:20px;
    }

    .section-title{
      margin:0 0 12px;
      font-size:14px;
      font-weight:800;
      color:#fff;
      text-transform:uppercase;
      letter-spacing:.08em;
    }

    .grid-2,.grid-3{
      display:grid;
      gap:14px;
    }

    .grid-2{grid-template-columns:repeat(2,minmax(0,1fr))}
    .grid-3{grid-template-columns:repeat(3,minmax(0,1fr))}

    @media (max-width:760px){
      .grid-2,.grid-3{grid-template-columns:1fr}
    }

    .field{
      display:grid;
      gap:7px;
    }

    .field label{
      font-size:13px;
      font-weight:700;
      color:#e5e7eb;
    }

    .field input,
    .field select{
      width:100%;
      border:1px solid var(--border);
      border-radius:12px;
      background:#0f1115;
      color:var(--text);
      padding:12px 13px;
      font-size:14px;
      outline:none;
    }

    .field input[type="color"]{
      min-height:46px;
      padding:6px;
    }

    .hint{
      color:var(--muted);
      font-size:12px;
      line-height:1.4;
    }

    .color-row{
      display:grid;
      grid-template-columns:92px 1fr;
      gap:10px;
    }

    .advanced-toggle{
      width:100%;
      display:flex;
      align-items:center;
      justify-content:space-between;
      gap:12px;
      border:1px solid var(--border);
      border-radius:14px;
      background:#12141a;
      color:var(--text);
      padding:14px 16px;
      cursor:pointer;
      font-size:14px;
      font-weight:800;
    }

    .advanced-box{
      margin-top:12px;
      padding:16px;
      border:1px solid var(--border);
      border-radius:14px;
      background:rgba(255,255,255,.02);
    }

    .actions{
      display:flex;
      flex-wrap:wrap;
      gap:10px;
    }

    button{
      border:0;
      border-radius:12px;
      padding:12px 16px;
      font-size:14px;
      font-weight:800;
      cursor:pointer;
      transition:transform .05s ease, opacity .15s ease;
    }

    button:active{transform:translateY(1px)}
    button:disabled{opacity:.55;cursor:not-allowed}

    .btn-primary{
      background:linear-gradient(180deg,var(--red-2),var(--red));
      color:#fff;
    }

    .btn-secondary{
      background:#23262f;
      color:#fff;
      border:1px solid #353946;
    }

    .btn-ghost{
      background:transparent;
      color:#fff;
      border:1px solid var(--border);
    }

    .status-wrap{
      display:grid;
      gap:16px;
    }

    .status-banner{
      padding:14px 16px;
      border-radius:14px;
      border:1px solid rgba(59,130,246,.28);
      background:rgba(59,130,246,.10);
      color:#dbeafe;
      font-weight:700;
      white-space:pre-wrap;
    }

    .steps{
      display:grid;
      gap:12px;
    }

    .step{
      display:flex;
      align-items:center;
      gap:12px;
      padding:13px 14px;
      border-radius:14px;
      border:1px solid var(--border);
      background:rgba(255,255,255,.02);
    }

    .dot{
      width:14px;
      height:14px;
      border-radius:50%;
      border:2px solid #52525b;
      background:transparent;
      flex:0 0 14px;
    }

    .step.active .dot{
      border-color:var(--blue);
      background:var(--blue);
      box-shadow:0 0 0 4px rgba(59,130,246,.16);
    }

    .step.done .dot{
      border-color:var(--green);
      background:var(--green);
      box-shadow:0 0 0 4px rgba(34,197,94,.16);
    }

    .step.error .dot{
      border-color:var(--red-2);
      background:var(--red-2);
      box-shadow:0 0 0 4px rgba(239,68,68,.16);
    }

    .step-title{
      font-size:14px;
      font-weight:800;
      color:#fff;
    }

    .step-sub{
      margin-top:3px;
      font-size:12px;
      color:var(--muted);
    }

    .result{
      padding:14px 16px;
      border-radius:14px;
      border:1px solid var(--border);
      background:rgba(255,255,255,.03);
      white-space:pre-wrap;
      color:var(--muted);
    }

    .result.success{
      color:#d1fae5;
      border-color:rgba(34,197,94,.28);
      background:rgba(34,197,94,.10);
    }

    .result.error{
      color:#fee2e2;
      border-color:rgba(239,68,68,.28);
      background:rgba(239,68,68,.10);
    }

    .footer-note{
      margin-top:10px;
      color:var(--muted);
      font-size:12px;
      text-align:center;
    }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="brand" aria-label="SpoolSense Writer logo">
      <svg width="420" height="78" viewBox="0 0 420 78" fill="none" xmlns="http://www.w3.org/2000/svg" role="img">
        <defs>
          <linearGradient id="gRed" x1="0" x2="1">
            <stop offset="0" stop-color="#EF4444"/>
            <stop offset="1" stop-color="#DC2626"/>
          </linearGradient>
        </defs>
        <g transform="translate(0,6)">
          <circle cx="34" cy="33" r="22" fill="#111318" stroke="#2A2E36" stroke-width="3"/>
          <circle cx="34" cy="33" r="8" fill="#0B0B0D" stroke="#52525B" stroke-width="3"/>
          <path d="M18 21C22 17 28 14 34 14C45 14 54 23 54 34" stroke="url(#gRed)" stroke-width="6" stroke-linecap="round"/>
          <path d="M15 32C15 43 24 52 35 52C41 52 47 50 52 46" stroke="url(#gRed)" stroke-width="6" stroke-linecap="round"/>
          <path d="M68 18C76 18 83 24 83 33" stroke="url(#gRed)" stroke-width="4" stroke-linecap="round"/>
          <path d="M74 12C85 12 95 21 95 33" stroke="url(#gRed)" stroke-width="4" stroke-linecap="round" opacity=".85"/>
          <text x="112" y="36" fill="#F4F4F5" font-size="30" font-weight="800" font-family="Inter, Arial, sans-serif">SpoolSense</text>
          <text x="112" y="58" fill="#EF4444" font-size="17" font-weight="800" font-family="Inter, Arial, sans-serif" letter-spacing="2">WRITER</text>
        </g>
      </svg>
    </div>

    <section class="card" id="createView">
      <div class="card-head">
        <h1 class="card-title">Create OpenPrintTag</h1>
        <p class="card-subtitle">Fill in the spool details, then write them to the tag.</p>
      </div>

      <div class="card-body">
        <form id="writerForm">
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
                  <option value="9">PEBA</option>
                  <option value="9">PPA</option>
                  <option value="9">PP-CF</option>
                  <option value="9">PEKK</option>
                  <option value="9">ULTEM (PEI 9085 / 1010)</option>
                  <option value="6">PA6 / PA12</option>
                  <option value="9">PPS</option>
                  <option value="9">PPS-CF</option>
                  <option value="9">PPS-GF</option>
                  <option value="9">CUSTOM</option>
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
              <span id="advancedToggleText">Show</span>
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
                  <label for="spoolman_id">Spoolman ID</label>
                  <input id="spoolman_id" name="spoolman_id" type="number" step="1" value="-1" />
                </div>

                <div class="field">
                  <label for="min_print_temp">Min Print Temp (°C)</label>
                  <input id="min_print_temp" name="min_print_temp" type="number" min="0" step="1" placeholder="205" />
                </div>

                <div class="field">
                  <label for="max_print_temp">Max Print Temp (°C)</label>
                  <input id="max_print_temp" name="max_print_temp" type="number" min="0" step="1" placeholder="225" />
                </div>

                <div class="field">
                  <label for="preheat_temp">Preheat Temp (°C)</label>
                  <input id="preheat_temp" name="preheat_temp" type="number" min="0" step="1" placeholder="215" />
                </div>

                <div class="field">
                  <label for="min_bed_temp">Min Bed Temp (°C)</label>
                  <input id="min_bed_temp" name="min_bed_temp" type="number" min="0" step="1" placeholder="55" />
                </div>

                <div class="field">
                  <label for="max_bed_temp">Max Bed Temp (°C)</label>
                  <input id="max_bed_temp" name="max_bed_temp" type="number" min="0" step="1" placeholder="65" />
                </div>
              </div>
            </div>
          </section>

          <div class="actions">
            <button type="submit" class="btn-primary" id="writeBtn">Write Tag</button>
            <button type="button" class="btn-secondary" id="demoBtn">Fill Demo</button>
            <button type="reset" class="btn-ghost">Clear</button>
          </div>
        </form>
      </div>
    </section>

    <section class="card hidden" id="statusView">
      <div class="card-head">
        <h1 class="card-title">Writing Tag</h1>
        <p class="card-subtitle">Keep the tag on the scanner until verification completes.</p>
      </div>

      <div class="card-body">
        <div class="status-wrap">
          <div class="status-banner" id="statusBanner">Starting write flow…</div>

          <div class="steps">
            <div class="step" id="step-wait">
              <div class="dot"></div>
              <div>
                <div class="step-title">Waiting for tag</div>
                <div class="step-sub">Checking scanner status for a present tag.</div>
              </div>
            </div>

            <div class="step" id="step-detect">
              <div class="dot"></div>
              <div>
                <div class="step-title">Tag detected</div>
                <div class="step-sub">Tag presence and UID confirmed.</div>
              </div>
            </div>

            <div class="step" id="step-format">
              <div class="dot"></div>
              <div>
                <div class="step-title">Formatting if needed</div>
                <div class="step-sub">Blank tags need NDEF structure before writing.</div>
              </div>
            </div>

            <div class="step" id="step-write">
              <div class="dot"></div>
              <div>
                <div class="step-title">Writing data</div>
                <div class="step-sub">Sending OpenPrintTag fields to the scanner.</div>
              </div>
            </div>

            <div class="step" id="step-verify">
              <div class="dot"></div>
              <div>
                <div class="step-title">Verifying write</div>
                <div class="step-sub">Polling status until the written values match.</div>
              </div>
            </div>
          </div>

          <div class="result" id="resultBox">Waiting…</div>

          <div class="actions">
            <button type="button" class="btn-secondary hidden" id="backBtn">Back to Form</button>
            <button type="button" class="btn-primary hidden" id="anotherBtn">Write Another</button>
          </div>
        </div>
      </div>
    </section>

    <div class="footer-note">Single-file local writer for SpoolSense Scanner.</div>
  </div>

  <script>
    const createView = document.getElementById('createView');
    const statusView = document.getElementById('statusView');
    const writerForm = document.getElementById('writerForm');

    const advancedToggle = document.getElementById('advancedToggle');
    const advancedToggleText = document.getElementById('advancedToggleText');
    const advancedBox = document.getElementById('advancedBox');

    const colorPicker = document.getElementById('colorPicker');
    const colorHex = document.getElementById('colorHex');

    const materialTypeEl = document.getElementById('material_type');
    const materialNameEl = document.getElementById('material_name');

    const statusBanner = document.getElementById('statusBanner');
    const resultBox = document.getElementById('resultBox');
    const backBtn = document.getElementById('backBtn');
    const anotherBtn = document.getElementById('anotherBtn');
    const demoBtn = document.getElementById('demoBtn');

    const steps = {
      wait: document.getElementById('step-wait'),
      detect: document.getElementById('step-detect'),
      format: document.getElementById('step-format'),
      write: document.getElementById('step-write'),
      verify: document.getElementById('step-verify')
    };

    function sleep(ms) {
      return new Promise(resolve => setTimeout(resolve, ms));
    }

    function nearlyEqual(a, b, tolerance) {
      return Math.abs(Number(a) - Number(b)) < tolerance;
    }

    function setStepState(name, state) {
      const el = steps[name];
      if (!el) return;
      el.classList.remove('active', 'done', 'error');
      if (state) el.classList.add(state);
    }

    function resetSteps() {
      Object.keys(steps).forEach(key => setStepState(key, ''));
    }

    function setBanner(text) {
      statusBanner.textContent = text;
    }

    function setResult(text, type) {
      resultBox.textContent = text;
      resultBox.className = 'result' + (type ? ' ' + type : '');
    }

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

    function toggleAdvanced(forceOpen) {
      const open = typeof forceOpen === 'boolean' ? forceOpen : advancedBox.classList.contains('hidden');
      advancedBox.classList.toggle('hidden', !open);
      advancedToggle.setAttribute('aria-expanded', open ? 'true' : 'false');
      advancedToggleText.textContent = open ? 'Hide' : 'Show';
    }

    function normalizeHex(value) {
      let v = String(value || '').trim().toUpperCase();
      if (!v) return '#FF0000';
      if (!v.startsWith('#')) v = '#' + v;
      if (/^#[0-9A-F]{6}$/.test(v)) return v;
      return null;
    }

    function syncPickerFromHex() {
      const valid = normalizeHex(colorHex.value);
      if (valid) {
        colorHex.value = valid;
        colorPicker.value = valid;
      }
    }

    function syncHexFromPicker() {
      colorHex.value = colorPicker.value.toUpperCase();
    }

    function forceUppercase(el) {
      el.value = el.value.toUpperCase();
    }

    function syncMaterialNameFromSelection() {
      const selectedText = materialTypeEl.options[materialTypeEl.selectedIndex].text;
      if (!materialNameEl.value.trim() || materialNameEl.dataset.autoFilled === 'true') {
        materialNameEl.value = selectedText.toUpperCase();
        materialNameEl.dataset.autoFilled = 'true';
      }
    }

    function readString(id) {
      const raw = document.getElementById(id).value.trim();
      return raw === '' ? undefined : raw;
    }

    function readPositiveNumber(id) {
      const raw = document.getElementById(id).value.trim();
      if (raw === '') return undefined;
      const num = Number(raw);
      if (!Number.isFinite(num) || num <= 0) return undefined;
      return num;
    }

    function readRequiredNumber(id) {
      const raw = document.getElementById(id).value.trim();
      const num = Number(raw);
      if (!Number.isFinite(num)) {
        throw new Error('Invalid number for ' + id);
      }
      return num;
    }

    function readOptionalSpoolmanId() {
      const raw = document.getElementById('spoolman_id').value.trim();
      if (raw === '') return -1;
      const num = Number(raw);
      if (!Number.isFinite(num)) return -1;
      return num;
    }

    function maybeSetValue(id, value) {
      if (value === undefined || value === null) return;
      document.getElementById(id).value = value;
    }

    function prefillFromStatus(status) {
      if (!status || !status.present) return;

      maybeSetValue('material_type', status.material_type);
      if (status.manufacturer !== undefined) maybeSetValue('manufacturer', status.manufacturer);
      if (status.material_name !== undefined) {
        maybeSetValue('material_name', String(status.material_name).toUpperCase());
        materialNameEl.dataset.autoFilled = 'false';
      }
      if (status.initial_weight_g !== undefined) maybeSetValue('initial_weight_g', status.initial_weight_g);
      if (status.remaining_g !== undefined) maybeSetValue('remaining_g', status.remaining_g);

      if (status.color !== undefined) {
        const hex = normalizeHex(status.color);
        if (hex) {
          colorHex.value = hex;
          colorPicker.value = hex;
        }
      }

      if (status.density !== undefined) maybeSetValue('density', status.density);
      if (status.diameter_mm !== undefined) maybeSetValue('diameter_mm', status.diameter_mm);
      if (status.min_print_temp !== undefined) maybeSetValue('min_print_temp', status.min_print_temp);
      if (status.max_print_temp !== undefined) maybeSetValue('max_print_temp', status.max_print_temp);
      if (status.preheat_temp !== undefined) maybeSetValue('preheat_temp', status.preheat_temp);
      if (status.min_bed_temp !== undefined) maybeSetValue('min_bed_temp', status.min_bed_temp);
      if (status.max_bed_temp !== undefined) maybeSetValue('max_bed_temp', status.max_bed_temp);
      if (status.spoolman_id !== undefined) maybeSetValue('spoolman_id', status.spoolman_id);
    }

    function buildPayload(uid) {
      const color = normalizeHex(colorHex.value);
      if (!color) throw new Error('Color must be a valid 6-digit hex value');

      const manufacturer = readString('manufacturer');
      if (!manufacturer) {
        throw new Error('Manufacturer is required');
      }

      const payload = {
        uid: uid || '',
        material_type: Number(materialTypeEl.value),
        color: color,
        manufacturer: manufacturer,
        initial_weight_g: readRequiredNumber('initial_weight_g'),
        remaining_g: readRequiredNumber('remaining_g')
      };

      const materialName = readString('material_name');
      if (materialName) payload.material_name = materialName.toUpperCase();

      const density = readPositiveNumber('density');
      if (density !== undefined) payload.density = density;

      const diameter = readPositiveNumber('diameter_mm');
      if (diameter !== undefined) payload.diameter_mm = diameter;

      const minPrint = readPositiveNumber('min_print_temp');
      if (minPrint !== undefined) payload.min_print_temp = minPrint;

      const maxPrint = readPositiveNumber('max_print_temp');
      if (maxPrint !== undefined) payload.max_print_temp = maxPrint;

      const preheat = readPositiveNumber('preheat_temp');
      if (preheat !== undefined) payload.preheat_temp = preheat;

      const minBed = readPositiveNumber('min_bed_temp');
      if (minBed !== undefined) payload.min_bed_temp = minBed;

      const maxBed = readPositiveNumber('max_bed_temp');
      if (maxBed !== undefined) payload.max_bed_temp = maxBed;

      payload.spoolman_id = readOptionalSpoolmanId();

      return payload;
    }

    async function api(url, options) {
      const res = await fetch(url, options);
      let data;
      try {
        data = await res.json();
      } catch (e) {
        throw new Error('Invalid JSON response');
      }

      if (!res.ok || (data && data.success === false)) {
        throw new Error((data && data.error) ? data.error : ('HTTP ' + res.status));
      }

      return data;
    }

    function valuesMatch(status, payload) {
      const checks = [];

      if ('material_type' in payload) {
        checks.push(Number(status.material_type) === Number(payload.material_type));
      }

      if ('color' in payload) {
        checks.push(String(status.color || '').toUpperCase() === String(payload.color).toUpperCase());
      }

      if (payload.manufacturer !== undefined) {
        checks.push(String(status.manufacturer || '') === String(payload.manufacturer));
      }

      if ('initial_weight_g' in payload) {
        checks.push(nearlyEqual(status.initial_weight_g, payload.initial_weight_g, 0.5));
      }

      if ('remaining_g' in payload) {
        checks.push(nearlyEqual(status.remaining_g, payload.remaining_g, 0.5));
      }

      if ('material_name' in payload && status.material_name !== undefined) {
        checks.push(String(status.material_name || '').toUpperCase() === String(payload.material_name).toUpperCase());
      }

      if ('density' in payload && status.density !== undefined) {
        checks.push(nearlyEqual(status.density, payload.density, 0.01));
      }

      if ('diameter_mm' in payload && status.diameter_mm !== undefined) {
        checks.push(nearlyEqual(status.diameter_mm, payload.diameter_mm, 0.01));
      }

      if ('min_print_temp' in payload && status.min_print_temp !== undefined) {
        checks.push(Number(status.min_print_temp) === Number(payload.min_print_temp));
      }

      if ('max_print_temp' in payload && status.max_print_temp !== undefined) {
        checks.push(Number(status.max_print_temp) === Number(payload.max_print_temp));
      }

      if ('preheat_temp' in payload && status.preheat_temp !== undefined) {
        checks.push(Number(status.preheat_temp) === Number(payload.preheat_temp));
      }

      if ('min_bed_temp' in payload && status.min_bed_temp !== undefined) {
        checks.push(Number(status.min_bed_temp) === Number(payload.min_bed_temp));
      }

      if ('max_bed_temp' in payload && status.max_bed_temp !== undefined) {
        checks.push(Number(status.max_bed_temp) === Number(payload.max_bed_temp));
      }

      if ('spoolman_id' in payload && payload.spoolman_id > 0 && status.spoolman_id !== undefined) {
        checks.push(Number(status.spoolman_id) === Number(payload.spoolman_id));
      }

      return checks.length > 0 && checks.every(Boolean);
    }

    async function waitForPresentTag(timeoutMs) {
      const deadline = Date.now() + timeoutMs;
      setStepState('wait', 'active');
      setBanner('Waiting for tag…');
      setResult('Place and hold the tag on the scanner.', '');

      while (Date.now() < deadline) {
        const status = await api('/api/status');
        if (status.present) {
          setStepState('wait', 'done');
          return status;
        }
        await sleep(500);
      }

      setStepState('wait', 'error');
      throw new Error('No tag detected. Place the tag on the scanner and try again.');
    }

    async function writeFlow() {
      resetSteps();
      showStatusView();

      try {
        const presentStatus = await waitForPresentTag(8000);

        setStepState('detect', 'active');
        setBanner('Tag detected.');
        setResult('UID: ' + (presentStatus.uid || 'Unknown'), '');
        await sleep(250);
        setStepState('detect', 'done');

        const payload = buildPayload(presentStatus.uid);

        if (presentStatus.tag_data_valid === false) {
          setStepState('format', 'active');
          setBanner('Formatting blank tag…');
          setResult('Blank tag detected. Initializing NDEF structure.', '');
          await api('/api/format-tag', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ uid: presentStatus.uid })
          });
          await sleep(1000);
          setStepState('format', 'done');
        } else {
          setStepState('format', 'done');
        }

        setStepState('write', 'active');
        setBanner('Writing data…');
        setResult('Sending OpenPrintTag payload to scanner.', '');
        await api('/api/write-tag', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(payload)
        });
        setStepState('write', 'done');

        setStepState('verify', 'active');
        setBanner('Verifying write…');
        setResult('Reading tag back to confirm fields match.', '');

        const verifyDeadline = Date.now() + 15000;
        while (Date.now() < verifyDeadline) {
          await sleep(500);
          const status = await api('/api/status');
          if (status.present && valuesMatch(status, payload)) {
            setStepState('verify', 'done');
            setBanner('Write successful.');
            setResult('OpenPrintTag data verified successfully.', 'success');
            backBtn.classList.remove('hidden');
            anotherBtn.classList.remove('hidden');
            return;
          }
        }

        setStepState('verify', 'error');
        throw new Error('Write timed out. Keep the tag on the scanner and try again.');
      } catch (err) {
        const msg = err && err.message ? err.message : 'Write failed';
        setBanner('Write failed.');
        setResult(msg, 'error');

        Object.keys(steps).forEach(key => {
          if (steps[key].classList.contains('active')) {
            setStepState(key, 'error');
          }
        });

        backBtn.classList.remove('hidden');
      }
    }

    advancedToggle.addEventListener('click', () => toggleAdvanced());

    colorPicker.addEventListener('input', syncHexFromPicker);
    colorHex.addEventListener('input', syncPickerFromHex);
    colorHex.addEventListener('blur', syncPickerFromHex);

    materialTypeEl.addEventListener('change', syncMaterialNameFromSelection);

    materialNameEl.addEventListener('input', () => {
      forceUppercase(materialNameEl);
      materialNameEl.dataset.autoFilled = 'false';
    });

    demoBtn.addEventListener('click', () => {
      materialTypeEl.value = '0';
      document.getElementById('manufacturer').value = 'Prusament';
      materialNameEl.value = 'PLA';
      materialNameEl.dataset.autoFilled = 'false';
      document.getElementById('initial_weight_g').value = '1000.0';
      document.getElementById('remaining_g').value = '750.0';
      document.getElementById('density').value = '1.24';
      document.getElementById('diameter_mm').value = '1.75';
      document.getElementById('min_print_temp').value = '205';
      document.getElementById('max_print_temp').value = '225';
      document.getElementById('preheat_temp').value = '215';
      document.getElementById('min_bed_temp').value = '55';
      document.getElementById('max_bed_temp').value = '65';
      document.getElementById('spoolman_id').value = '-1';
      colorPicker.value = '#ff0000';
      syncHexFromPicker();
      toggleAdvanced(true);
    });

    writerForm.addEventListener('reset', () => {
      setTimeout(() => {
        colorPicker.value = '#ff0000';
        colorHex.value = '#FF0000';
        materialNameEl.dataset.autoFilled = 'true';
        syncMaterialNameFromSelection();
      }, 0);
    });

    writerForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      await writeFlow();
    });

    backBtn.addEventListener('click', showCreateView);
    anotherBtn.addEventListener('click', showCreateView);

    syncHexFromPicker();
    syncMaterialNameFromSelection();

    (async function init() {
      try {
        const status = await api('/api/status');
        prefillFromStatus(status);
      } catch (e) {
      }
    })();
  </script>
</body>
</html>
)rawliteral";
