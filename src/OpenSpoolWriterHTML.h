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
                <label>Color</label>
                <div class="color-row">
                  <input id="colorPicker" type="color" value="#FF0000" />
                  <input id="colorHex" type="text" value="#FF0000" maxlength="7" placeholder="#FF0000" required />
                </div>
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

        <div id="spoolmanEnrichment" class="hidden" style="margin-top:16px;padding:14px;background:var(--card-alt,#1e1e35);border:1px solid var(--blue,#4a9eff);border-radius:8px;">
          <div style="display:flex;align-items:center;gap:8px;margin-bottom:4px">
            <span style="color:var(--blue,#4a9eff);font-size:11px;text-transform:uppercase;letter-spacing:1px;font-weight:600">Spoolman Enrichment</span>
            <span id="spoolmanMatchBadge" class="hidden" style="background:#4a9eff22;border:1px solid #4a9eff55;border-radius:3px;padding:0 6px;color:#4a9eff;font-size:10px"></span>
          </div>
          <p class="card-subtitle" style="font-size:11px;margin-bottom:10px">
            Extra data Spoolman stores that OpenSpool format cannot &mdash; saved to Spoolman on write.
          </p>
          <div class="grid-2">
            <div class="field">
              <label for="enrich-bed-min">Bed Temp Min (&deg;C)</label>
              <input id="enrich-bed-min" type="number" placeholder="e.g. 55" min="0" max="150" />
            </div>
            <div class="field">
              <label for="enrich-bed-max">Bed Temp Max (&deg;C)</label>
              <input id="enrich-bed-max" type="number" placeholder="e.g. 65" min="0" max="150" />
            </div>
            <div class="field">
              <label for="enrich-diameter">Diameter (mm)</label>
              <input id="enrich-diameter" type="number" placeholder="1.75" min="0.1" max="5" step="0.01" />
            </div>
            <div class="field">
              <label for="enrich-density">Density (g/cm&sup3;)</label>
              <input id="enrich-density" type="number" placeholder="1.24" min="0.1" max="5" step="0.001" />
            </div>
            <div class="field">
              <label for="enrich-remaining">Remaining Weight (g)</label>
              <input id="enrich-remaining" type="number" placeholder="e.g. 1000" min="0" max="10000" />
            </div>
          </div>
        </div>
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

    syncColorPicker('colorPicker', 'colorHex');

    renderSpoolmanPicker('spoolmanPicker', {
      manufacturer: 'brand',
      material: 'type',
      color: 'colorHex',
      colorPicker: 'colorPicker',
      nozzle_min: 'min_temp',
      nozzle_max: 'max_temp',
      // enrichment fields:
      bed_min: 'enrich-bed-min',
      bed_max: 'enrich-bed-max',
      diameter: 'enrich-diameter',
      density: 'enrich-density',
      remaining: 'enrich-remaining'
    });

    // Show enrichment section if Spoolman is configured (spools endpoint succeeds)
    (async function() {
      try {
        var r = await fetch('/api/spoolman/spools');
        if (r.ok) {
          document.getElementById('spoolmanEnrichment').classList.remove('hidden');
        }
      } catch(e) {}
    })();

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
      if (sp.bed_temp && sp.bed_temp > 0) {
        setVal('enrich-bed-min', sp.bed_temp);
        setVal('enrich-bed-max', sp.bed_temp);
      }
      if (sp.diameter_mm && sp.diameter_mm > 0) setVal('enrich-diameter', sp.diameter_mm);
      if (sp.density && sp.density > 0) setVal('enrich-density', sp.density);
    }

    async function startRead() {
      setReadWaiting(true);
      var deadline = Date.now() + 30000;
      while (readWaiting && Date.now() < deadline) {
        try {
          var status = await fetch('/api/status').then(r => r.json());
          if (status.present && status.tag_kind === 'OpenSpoolTag') {
            var os = status.openspool || {};
            setVal('brand', os.brand || '');
            setVal('type', os.material || '');
            if (os.color_hex) {
              setVal('colorHex', os.color_hex);
              setVal('colorPicker', os.color_hex.startsWith('#') ? os.color_hex : '#' + os.color_hex);
            }
            if (os.min_temp) setVal('min_temp', os.min_temp);
            if (os.max_temp) setVal('max_temp', os.max_temp);
            fillEnrichmentFromStatus(status);
            if (status.spoolman && status.spoolman.spool_id > 0) {
              showMatchBadge('Spool #' + status.spoolman.spool_id + ' matched');
            } else {
              showMatchBadge('no Spoolman match');
            }
            break;
          } else if (status.present) {
            showMatchBadge('wrong format \u2014 expected OpenSpool');
            break;
          }
        } catch(e) {}
        await new Promise(r => setTimeout(r, 500));
      }
      setReadWaiting(false);
    }

    readBtn.onclick = startRead;

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
      document.getElementById('colorPicker').value = '#FF0000';
      document.getElementById('colorHex').value = '#FF0000';
    });

    function buildPayload(uid) {
      var colorHex = normalizeHex(document.getElementById('colorHex').value).replace('#', '');
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
