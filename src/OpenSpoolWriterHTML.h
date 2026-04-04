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

    <section class="card" id="createView">
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
                <label for="color_hex">Color</label>
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

          <div class="write-warning">Keep the tag still &mdash; do not remove until writing is complete.</div>

          <div class="actions">
            <button type="submit" class="btn-primary" id="writeBtn">Write Tag</button>
            <button type="reset" class="btn-ghost">Clear</button>
          </div>
        </form>
      </div>
    </section>

    <section class="card hidden" id="statusView">
      <div class="card-head">
        <h1 class="card-title">Writing OpenSpool Tag</h1>
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
                <div class="step-sub">Place an NTAG215 or NTAG216 on the scanner.</div>
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
                <div class="step-sub">Writing OpenSpool NDEF payload to tag.</div>
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

    <div class="footer-note">OpenSpool V1.0 &mdash; NDEF JSON on NTAG215/216</div>
  </div>

  <script src="/js/shared.js"></script>
  <script>
    var createView = document.getElementById('createView');
    var statusView = document.getElementById('statusView');
    var writerForm = document.getElementById('writerForm');
    var backBtn = document.getElementById('backBtn');
    var anotherBtn = document.getElementById('anotherBtn');

    var STEP_IDS = ['step-wait', 'step-detect', 'step-write', 'step-verify'];

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

    backBtn.addEventListener('click', showCreateView);
    anotherBtn.addEventListener('click', function() {
      showCreateView();
      writerForm.reset();
      document.getElementById('colorPreview').style.background = '#FF0000';
    });

    function buildPayload(uid) {
      var colorHex = document.getElementById('color_hex').value.replace('#', '').toUpperCase();
      return {
        uid: uid || '',
        protocol: 'openspool',
        version: '1.0',
        type: document.getElementById('type').value,
        color_hex: colorHex,
        brand: document.getElementById('brand').value.trim(),
        min_temp: document.getElementById('min_temp').value,
        max_temp: document.getElementById('max_temp').value
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
        setBanner('statusBanner', 'Writing OpenSpool data\u2026');
        setResult('resultBox', 'Sending OpenSpool JSON to scanner.', '');
        await api('/api/write-openspool', {
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
          if (status.present && status.tag_kind === 'OpenSpoolTag') {
            setBanner('statusBanner', 'Tag verified \u2014 hold for a moment\u2026');
            await sleep(2000);
            setStepState('step-verify', 'done');
            setBanner('statusBanner', 'Write complete \u2014 safe to remove tag.');
            setResult('resultBox', 'OpenSpool tag written and verified successfully.', 'success');
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
  </script>
</body>
</html>
)rawliteral";
