#pragma once

// OpenTag3D writer page served at GET /writer/opentag3d
// Writes OpenTag3D binary format to NTAG215 via POST /api/write-opentag3d.

const char OPENTAG3D_WRITER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>OpenTag3D Writer &mdash; SpoolSense</title>
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
      <a href="/update">Update</a>
      <a href="/config">Config</a>
    </nav>

    <section class="card" id="createView">
      <div class="card-head">
        <div>
          <h1 class="card-title">Create OpenTag3D</h1>
          <p class="card-subtitle">Write filament data to an NTAG215 tag in OpenTag3D format.</p>
        </div>
      </div>

      <div class="card-body">
        <form id="writerForm">
          <section>
            <h2 class="section-title">Filament</h2>
            <div class="grid-2">
              <div class="field">
                <label for="base_material">Base Material</label>
                <input id="base_material" name="base_material" type="text" maxlength="5" placeholder="PLA" required />
              </div>

              <div class="field">
                <label for="material_modifiers">Modifiers</label>
                <input id="material_modifiers" name="material_modifiers" type="text" maxlength="5" placeholder="CF" />
              </div>

              <div class="field">
                <label for="manufacturer">Manufacturer</label>
                <input id="manufacturer" name="manufacturer" type="text" maxlength="16" placeholder="Polymaker" required />
              </div>

              <div class="field">
                <label for="target_weight_g">Weight (g)</label>
                <input id="target_weight_g" name="target_weight_g" type="number" min="1" max="65535" step="1" placeholder="1000" required />
              </div>

              <div class="field">
                <label for="diameter_um">Diameter</label>
                <select id="diameter_um" name="diameter_um">
                  <option value="1750">1.75 mm</option>
                  <option value="2850">2.85 mm</option>
                </select>
              </div>

              <div class="field">
                <label for="density_ugcm3">Density (&micro;g/cm&sup3;)</label>
                <input id="density_ugcm3" name="density_ugcm3" type="number" min="0" max="65535" step="1" placeholder="1240" />
              </div>
            </div>
          </section>

          <section>
            <h2 class="section-title">Color</h2>
            <div class="grid-2">
              <div class="field">
                <label for="color_name">Color Name</label>
                <input id="color_name" name="color_name" type="text" maxlength="32" placeholder="Galaxy Purple" />
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
            <h2 class="section-title">Temperatures</h2>
            <div class="grid-2">
              <div class="field">
                <label for="print_temp_c">Print Temp (&deg;C)</label>
                <input id="print_temp_c" name="print_temp_c" type="number" min="0" max="1275" step="5" placeholder="210" required />
              </div>
              <div class="field">
                <label for="bed_temp_c">Bed Temp (&deg;C)</label>
                <input id="bed_temp_c" name="bed_temp_c" type="number" min="0" max="1275" step="5" placeholder="60" required />
              </div>
            </div>
          </section>

          <section>
            <details id="extendedDetails">
              <summary class="section-title" style="cursor:pointer">Extended Fields (optional)</summary>
              <div style="margin-top:12px">
                <div class="grid-2">
                  <div class="field">
                    <label for="serial_number">Serial Number</label>
                    <input id="serial_number" name="serial_number" type="text" maxlength="16" placeholder="SN-2026-001" />
                  </div>
                  <div class="field">
                    <label for="online_url">URL</label>
                    <input id="online_url" name="online_url" type="text" maxlength="32" placeholder="example.com/spool" />
                  </div>
                  <div class="field">
                    <label for="measured_filament_weight_g">Measured Weight (g)</label>
                    <input id="measured_filament_weight_g" name="measured_filament_weight_g" type="number" min="0" max="65535" step="1" placeholder="980" />
                  </div>
                  <div class="field">
                    <label for="empty_spool_weight_g">Empty Spool (g)</label>
                    <input id="empty_spool_weight_g" name="empty_spool_weight_g" type="number" min="0" max="65535" step="1" placeholder="250" />
                  </div>
                  <div class="field">
                    <label for="min_print_temp_c">Min Print Temp (&deg;C)</label>
                    <input id="min_print_temp_c" name="min_print_temp_c" type="number" min="0" max="1275" step="5" placeholder="190" />
                  </div>
                  <div class="field">
                    <label for="max_print_temp_c">Max Print Temp (&deg;C)</label>
                    <input id="max_print_temp_c" name="max_print_temp_c" type="number" min="0" max="1275" step="5" placeholder="230" />
                  </div>
                  <div class="field">
                    <label for="min_bed_temp_c">Min Bed Temp (&deg;C)</label>
                    <input id="min_bed_temp_c" name="min_bed_temp_c" type="number" min="0" max="1275" step="5" placeholder="50" />
                  </div>
                  <div class="field">
                    <label for="max_bed_temp_c">Max Bed Temp (&deg;C)</label>
                    <input id="max_bed_temp_c" name="max_bed_temp_c" type="number" min="0" max="1275" step="5" placeholder="70" />
                  </div>
                  <div class="field">
                    <label for="max_dry_temp_c">Max Dry Temp (&deg;C)</label>
                    <input id="max_dry_temp_c" name="max_dry_temp_c" type="number" min="0" max="1275" step="5" placeholder="50" />
                  </div>
                  <div class="field">
                    <label for="dry_time_hours">Dry Time (hrs)</label>
                    <input id="dry_time_hours" name="dry_time_hours" type="number" min="0" max="255" step="1" placeholder="4" />
                  </div>
                  <div class="field">
                    <label for="target_volumetric_speed">Target Vol. Speed (mm&sup3;/s)</label>
                    <input id="target_volumetric_speed" name="target_volumetric_speed" type="number" min="0" max="255" step="1" placeholder="12" />
                  </div>
                  <div class="field">
                    <label for="transmission_distance">Transmission Dist. (mm&times;0.1)</label>
                    <input id="transmission_distance" name="transmission_distance" type="number" min="0" max="65535" step="1" placeholder="50" />
                  </div>
                </div>
              </div>
            </details>
          </section>

          <div class="write-warning">Keep the tag still &mdash; do not remove until writing is complete.</div>

          <div class="actions">
            <button type="submit" class="btn-primary" id="writeBtn">Write OpenTag3D</button>
            <button type="reset" class="btn-ghost">Clear</button>
          </div>
        </form>
      </div>
    </section>

    <section class="card hidden" id="statusView">
      <div class="card-head">
        <h1 class="card-title">Writing OpenTag3D</h1>
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
                <div class="step-sub">Place an NTAG215 on the scanner.</div>
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
                <div class="step-sub">Writing OpenTag3D payload to tag.</div>
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

    <div class="footer-note">OpenTag3D v1.000 &mdash; NTAG215</div>
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

    var EXTENDED_IDS = [
      'serial_number', 'online_url', 'measured_filament_weight_g',
      'empty_spool_weight_g', 'min_print_temp_c', 'max_print_temp_c',
      'min_bed_temp_c', 'max_bed_temp_c', 'max_dry_temp_c',
      'dry_time_hours', 'target_volumetric_speed', 'transmission_distance'
    ];

    function hasExtended() {
      for (var i = 0; i < EXTENDED_IDS.length; i++) {
        var el = document.getElementById(EXTENDED_IDS[i]);
        if (el && el.value.trim() !== '') return true;
      }
      return false;
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

    function buildPayload(uid) {
      var color = normalizeHex(document.getElementById('colorHex').value);
      if (!color) throw new Error('Color must be a valid 6-digit hex value');

      var r = parseInt(color.substr(1,2), 16);
      var g = parseInt(color.substr(3,2), 16);
      var b = parseInt(color.substr(5,2), 16);

      var body = {
        uid: uid || '',
        base_material: readString('base_material') || '',
        material_modifiers: readString('material_modifiers') || '',
        manufacturer: readString('manufacturer') || '',
        color_name: readString('color_name') || '',
        color_r: r,
        color_g: g,
        color_b: b,
        color_a: 255,
        diameter_um: readRequiredNumber('diameter_um'),
        target_weight_g: readRequiredNumber('target_weight_g'),
        print_temp_c: readRequiredNumber('print_temp_c'),
        bed_temp_c: readRequiredNumber('bed_temp_c'),
        density_ugcm3: readPositiveNumber('density_ugcm3') || 0
      };

      if (hasExtended()) {
        var sn = readString('serial_number');
        if (sn !== undefined) body.serial_number = sn;
        var url = readString('online_url');
        if (url !== undefined) body.online_url = url;
        var mfw = readPositiveNumber('measured_filament_weight_g');
        if (mfw !== undefined) body.measured_filament_weight_g = mfw;
        var esw = readPositiveNumber('empty_spool_weight_g');
        if (esw !== undefined) body.empty_spool_weight_g = esw;
        var mnp = readPositiveNumber('min_print_temp_c');
        if (mnp !== undefined) body.min_print_temp_c = mnp;
        var mxp = readPositiveNumber('max_print_temp_c');
        if (mxp !== undefined) body.max_print_temp_c = mxp;
        var mnb = readPositiveNumber('min_bed_temp_c');
        if (mnb !== undefined) body.min_bed_temp_c = mnb;
        var mxb = readPositiveNumber('max_bed_temp_c');
        if (mxb !== undefined) body.max_bed_temp_c = mxb;
        var mdt = readPositiveNumber('max_dry_temp_c');
        if (mdt !== undefined) body.max_dry_temp_c = mdt;
        var dth = readPositiveNumber('dry_time_hours');
        if (dth !== undefined) body.dry_time_hours = dth;
        var tvs = readPositiveNumber('target_volumetric_speed');
        if (tvs !== undefined) body.target_volumetric_speed = tvs;
        var td = readPositiveNumber('transmission_distance');
        if (td !== undefined) body.transmission_distance = td;
      }

      return body;
    }

    async function waitForTag(timeoutMs) {
      var deadline = Date.now() + timeoutMs;
      setStepState('step-wait', 'active');
      setBanner('statusBanner', 'Waiting for tag\u2026');
      setResult('resultBox', 'Place and hold an NTAG215 tag on the scanner.', '');

      while (Date.now() < deadline) {
        var status = await api('/api/status');
        if (status.present) {
          setStepState('step-wait', 'done');
          return status;
        }
        await sleep(2000);
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
        setBanner('statusBanner', 'Writing OpenTag3D data\u2026');
        setResult('resultBox', 'Sending OpenTag3D payload to scanner.', '');
        await api('/api/write-opentag3d', {
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
          await sleep(2000);
          var status = await api('/api/status');
          if (status.present && status.tag_kind === 'OpenTag3D') {
            setStepState('step-verify', 'done');
            setBanner('statusBanner', 'Write successful.');
            setResult('resultBox', 'OpenTag3D data verified successfully.', 'success');
            backBtn.classList.remove('hidden');
            anotherBtn.classList.remove('hidden');
            return;
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
