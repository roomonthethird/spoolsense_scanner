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
      <a href="/update">Update</a>
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
        <form id="writerForm">
          <section>
            <h2 class="section-title">Filament</h2>
            <div class="grid-2">
              <div class="field">
                <label for="material_id">Material</label>
                <select id="material_id" name="material_id" required>
                  <option value="38219">PLA</option>
                  <option value="46591">PLA+</option>
                  <option value="38256">PETG</option>
                  <option value="43518">TPU</option>
                  <option value="20562">ABS</option>
                  <option value="12844">ASA</option>
                  <option value="30458">PC</option>
                  <option value="15041">PCTG</option>
                  <option value="30884">PP</option>
                  <option value="56666">PA6</option>
                  <option value="55796">PA12</option>
                  <option value="59328">PA</option>
                  <option value="26029">HIPS</option>
                  <option value="52077">PET</option>
                  <option value="45962">PVB</option>
                  <option value="9483">PVA</option>
                  <option value="29815">PEEK</option>
                  <option value="53970">PEKK</option>
                  <option value="56527">PEI</option>
                  <option value="55279">PBT</option>
                  <option value="33958">TPE</option>
                  <option value="48310">PLA-CF</option>
                  <option value="55418">PETG-CF</option>
                  <option value="48815">PAHT-CF</option>
                  <option value="12264">PA6-CF</option>
                  <option value="10602">PLA Silk</option>
                  <option value="48001">PLA Wood</option>
                  <option value="65535">None</option>
                </select>
              </div>

              <div class="field">
                <label for="brand_id">Brand</label>
                <select id="brand_id" name="brand_id" required>
                  <option value="65535">Generic</option>
                  <option value="35123">Bambu Lab</option>
                  <option value="46392">Prusa</option>
                  <option value="50604">Polymaker</option>
                  <option value="47930">eSun</option>
                  <option value="51857">Sunlu</option>
                  <option value="46203">Overture</option>
                  <option value="3132">Hatchbox</option>
                  <option value="26956">Creality</option>
                  <option value="15962">Anycubic</option>
                  <option value="57632">Elegoo</option>
                  <option value="7812">Jayo</option>
                  <option value="52222">ColorFabb</option>
                  <option value="7980">Fillamentum</option>
                  <option value="8182">Fiberlogy</option>
                  <option value="53043">FormFutura</option>
                  <option value="58410">AzureFilm</option>
                  <option value="4344">MatterHackers</option>
                  <option value="2">Proto-Pasta</option>
                  <option value="58231">IC3D</option>
                  <option value="39652">3DXTech</option>
                  <option value="51443">BASF</option>
                  <option value="9798">AMOLEN</option>
                  <option value="28940">Eryone</option>
                  <option value="63340">Flashforge</option>
                  <option value="8384">Taulman3D</option>
                </select>
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

          <div class="write-warning">Keep the tag still &mdash; do not remove until writing is complete.</div>

          <div class="actions">
            <button type="submit" class="btn-primary" id="writeBtn">Write TigerTag</button>
            <button type="reset" class="btn-ghost">Clear</button>
          </div>
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
              setStepState('step-verify', 'done');
              setBanner('statusBanner', 'Write successful.');
              setResult('resultBox', 'TigerTag data verified successfully.', 'success');
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

  </script>
</body>
</html>
)rawliteral";
