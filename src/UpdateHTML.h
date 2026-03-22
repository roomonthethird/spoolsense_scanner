#pragma once

// Firmware update page served at GET /update
// Supports auto-check from GitHub releases and manual .bin upload.

const char UPDATE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Firmware Update &mdash; SpoolSense</title>
  <link rel="stylesheet" href="/css/shared.css" />
  <style>
    .progress-wrap{margin:16px 0}
    .progress-bar{
      width:100%;height:24px;
      border-radius:12px;
      background:#0f1115;
      border:1px solid var(--border);
      overflow:hidden;
    }
    .progress-fill{
      height:100%;width:0%;
      background:linear-gradient(90deg,var(--blue),var(--green));
      border-radius:12px;
      transition:width .2s;
    }
    .progress-text{
      text-align:center;
      font-size:13px;
      color:var(--muted);
      margin-top:6px;
    }
    .version-info{
      display:flex;gap:12px;
      align-items:center;
      flex-wrap:wrap;
    }
    .version-badge{
      padding:6px 14px;
      border-radius:10px;
      font-size:13px;
      font-weight:700;
    }
    .version-current{
      background:rgba(59,130,246,.12);
      color:#93c5fd;
      border:1px solid rgba(59,130,246,.28);
    }
    .version-latest{
      background:rgba(34,197,94,.12);
      color:#86efac;
      border:1px solid rgba(34,197,94,.28);
    }
    .release-notes{
      margin-top:14px;
      padding:16px;
      border:1px solid var(--border);
      border-radius:14px;
      background:rgba(255,255,255,.02);
      white-space:pre-wrap;
      font-size:13px;
      color:var(--muted);
      line-height:1.6;
      max-height:300px;
      overflow-y:auto;
    }
    .file-input-wrap{
      display:flex;gap:10px;
      align-items:center;
      flex-wrap:wrap;
    }
    .file-input-wrap input[type="file"]{
      flex:1;min-width:200px;
    }
  </style>
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
      <a href="/troubleshooting">Troubleshooting</a>
      <a href="/config">Config</a>
    </nav>

    <!-- Auto Update Card -->
    <section class="card" style="margin-bottom:16px">
      <div class="card-head">
        <h1 class="card-title">Firmware Update</h1>
        <p class="card-subtitle">Check for new versions and update over WiFi.</p>
      </div>
      <div class="card-body">
        <div class="version-info" id="versionInfo">
          <span class="version-badge version-current" id="currentVersion">Current: ...</span>
          <span class="version-badge version-current" id="boardType" style="background:rgba(255,255,255,.06);color:var(--muted);border-color:var(--border)">Board: ...</span>
        </div>

        <div style="margin-top:16px">
          <div class="actions">
            <button type="button" class="btn-primary" id="checkBtn">Check for Updates</button>
          </div>
        </div>

        <div id="updateResult" class="hidden" style="margin-top:16px">
          <div id="updateStatus" class="status-banner">Checking...</div>
          <div id="releaseNotes" class="release-notes hidden"></div>
          <div class="actions" style="margin-top:12px">
            <button type="button" class="btn-primary hidden" id="updateBtn">Update Now</button>
          </div>
        </div>

        <div id="uploadProgress" class="hidden">
          <div class="progress-wrap">
            <div class="progress-bar"><div class="progress-fill" id="progressFill"></div></div>
            <div class="progress-text" id="progressText">0%</div>
          </div>
          <div class="result" id="uploadResult"></div>
        </div>
      </div>
    </section>

    <!-- Manual Upload Card -->
    <section class="card">
      <div class="card-head">
        <h1 class="card-title">Manual Upload</h1>
        <p class="card-subtitle">Upload a firmware .bin file directly.</p>
      </div>
      <div class="card-body">
        <div class="file-input-wrap">
          <input type="file" id="firmwareFile" accept=".bin" class="field" />
          <button type="button" class="btn-primary" id="uploadBtn" disabled>Upload</button>
        </div>

        <div class="write-warning" style="margin-top:14px">
          Do not power off or navigate away during upload.
        </div>

        <div id="manualProgress" class="hidden" style="margin-top:14px">
          <div class="progress-wrap">
            <div class="progress-bar"><div class="progress-fill" id="manualProgressFill"></div></div>
            <div class="progress-text" id="manualProgressText">0%</div>
          </div>
          <div class="result" id="manualResult"></div>
        </div>
      </div>
    </section>

    <div class="footer-note">OTA updates write to the active partition. Keep USB cable handy as a fallback.</div>
  </div>

  <script src="/js/shared.js"></script>
  <script>
    var currentVersion = '';
    var boardType = '';
    var latestAssetUrl = '';

    // Fetch current version from device
    api('/api/version').then(function(data) {
      currentVersion = data.version || '?';
      boardType = data.board || '?';
      document.getElementById('currentVersion').textContent = 'Current: v' + currentVersion;
      document.getElementById('boardType').textContent = 'Board: ' + boardType;
    }).catch(function() {
      document.getElementById('currentVersion').textContent = 'Current: unknown';
    });

    // Compare semver strings: returns 1 if a>b, -1 if a<b, 0 if equal
    function compareSemver(a, b) {
      var pa = a.replace(/^v/, '').split('.').map(Number);
      var pb = b.replace(/^v/, '').split('.').map(Number);
      for (var i = 0; i < 3; i++) {
        var va = pa[i] || 0, vb = pb[i] || 0;
        if (va > vb) return 1;
        if (va < vb) return -1;
      }
      return 0;
    }

    // Check for updates via GitHub API
    document.getElementById('checkBtn').addEventListener('click', function() {
      var result = document.getElementById('updateResult');
      var status = document.getElementById('updateStatus');
      var notes = document.getElementById('releaseNotes');
      var updateBtn = document.getElementById('updateBtn');

      result.classList.remove('hidden');
      notes.classList.add('hidden');
      updateBtn.classList.add('hidden');
      status.textContent = 'Checking GitHub for latest release...';
      status.className = 'status-banner';

      fetch('https://api.github.com/repos/SpoolSense/spoolsense_scanner/releases/latest')
        .then(function(r) { return r.json(); })
        .then(function(release) {
          var latestTag = release.tag_name || '';
          var latestVersion = latestTag.replace(/^v/, '');
          var body = release.body || 'No release notes available.';

          // Find matching asset for this board
          var assets = release.assets || [];
          var assetName = 'spoolsense_scanner_' + boardType + '.bin';
          latestAssetUrl = '';
          for (var i = 0; i < assets.length; i++) {
            if (assets[i].name === assetName) {
              latestAssetUrl = assets[i].browser_download_url;
              break;
            }
          }

          if (compareSemver(latestVersion, currentVersion) > 0) {
            status.textContent = 'Update available: v' + latestVersion;
            status.style.borderColor = 'rgba(34,197,94,.28)';
            status.style.background = 'rgba(34,197,94,.10)';
            status.style.color = '#d1fae5';

            notes.textContent = body;
            notes.classList.remove('hidden');

            if (latestAssetUrl) {
              updateBtn.classList.remove('hidden');
              updateBtn.textContent = 'Update to v' + latestVersion;
            } else {
              notes.textContent += '\n\nNo firmware binary found for board: ' + boardType;
            }
          } else {
            status.textContent = 'You are up to date (v' + currentVersion + ')';
            status.style.borderColor = 'rgba(59,130,246,.28)';
            status.style.background = 'rgba(59,130,246,.10)';
            status.style.color = '#dbeafe';
          }
        })
        .catch(function(err) {
          status.textContent = 'Failed to check for updates. No internet?';
          status.style.borderColor = 'rgba(239,68,68,.28)';
          status.style.background = 'rgba(239,68,68,.10)';
          status.style.color = '#fee2e2';
        });
    });

    // Auto-update: tell ESP32 to download .bin from GitHub and flash it
    document.getElementById('updateBtn').addEventListener('click', function() {
      if (!latestAssetUrl) return;

      var btn = document.getElementById('updateBtn');
      var checkBtn = document.getElementById('checkBtn');
      btn.disabled = true;
      checkBtn.disabled = true;
      btn.textContent = 'Updating...';

      var progressWrap = document.getElementById('uploadProgress');
      var resultEl = document.getElementById('uploadResult');
      var progressFill = document.getElementById('progressFill');
      var progressText = document.getElementById('progressText');
      progressWrap.classList.remove('hidden');
      resultEl.textContent = 'Starting download...';
      resultEl.className = 'result';
      progressFill.style.width = '0%';
      progressText.textContent = 'Starting...';

      // Kick off the download on the ESP32
      fetch('/api/update-from-url', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ url: latestAssetUrl })
      }).then(function(r) { return r.json(); })
        .then(function(data) {
          if (!data.success) throw new Error(data.error || 'Failed to start');
          // Poll OTA status
          var pollTimer = setInterval(function() {
            fetch('/api/ota-status').then(function(r) { return r.json(); })
              .then(function(s) {
                progressFill.style.width = s.progress + '%';
                if (s.state === 'downloading') {
                  progressText.textContent = 'Downloading... ' + s.progress + '%';
                  resultEl.textContent = 'Device is downloading firmware from GitHub...';
                } else if (s.state === 'flashing') {
                  progressText.textContent = 'Flashing... ' + s.progress + '%';
                  resultEl.textContent = 'Writing firmware to flash...';
                } else if (s.state === 'success') {
                  clearInterval(pollTimer);
                  progressFill.style.width = '100%';
                  progressText.textContent = '100%';
                  resultEl.textContent = 'Update successful! Device is rebooting...';
                  resultEl.className = 'result success';
                  setTimeout(function() {
                    resultEl.textContent += '\nReloading page in 10 seconds...';
                  }, 3000);
                  setTimeout(function() { window.location.reload(); }, 13000);
                } else if (s.state === 'failed') {
                  clearInterval(pollTimer);
                  progressFill.style.width = '0%';
                  progressText.textContent = '';
                  resultEl.textContent = s.error || 'Update failed';
                  resultEl.className = 'result error';
                  btn.disabled = false;
                  checkBtn.disabled = false;
                  btn.textContent = 'Update Now';
                }
              })
              .catch(function() {
                // ESP32 rebooted — stop polling and reload
                clearInterval(pollTimer);
                progressFill.style.width = '100%';
                progressText.textContent = 'Rebooting...';
                resultEl.textContent = 'Update complete. Reloading in 10 seconds...';
                resultEl.className = 'result success';
                setTimeout(function() { window.location.reload(); }, 10000);
              });
          }, 1500);
        })
        .catch(function(err) {
          btn.disabled = false;
          checkBtn.disabled = false;
          btn.textContent = 'Update Now';
          resultEl.textContent = err.message || 'Failed to start update';
          resultEl.className = 'result error';
        });
    });

    // Manual file upload
    var fileInput = document.getElementById('firmwareFile');
    var uploadBtn = document.getElementById('uploadBtn');

    fileInput.addEventListener('change', function() {
      uploadBtn.disabled = !fileInput.files.length;
    });

    uploadBtn.addEventListener('click', function() {
      if (!fileInput.files.length) return;
      var file = fileInput.files[0];

      // Basic validation
      if (file.size < 100000 || file.size > 2000000) {
        var result = document.getElementById('manualResult');
        document.getElementById('manualProgress').classList.remove('hidden');
        result.textContent = 'File size looks wrong (' + Math.round(file.size/1024) + ' KB). Expected 100KB-2MB for a firmware binary.';
        result.className = 'result error';
        return;
      }

      uploadBtn.disabled = true;
      uploadFirmware(file, 'manualProgressFill', 'manualProgressText', 'manualResult', 'manualProgress');
    });

    // Shared upload function
    function uploadFirmware(blob, fillId, textId, resultId, wrapId) {
      var progressWrap = document.getElementById(wrapId);
      var progressFill = document.getElementById(fillId);
      var progressText = document.getElementById(textId);
      var resultEl = document.getElementById(resultId);

      progressWrap.classList.remove('hidden');
      resultEl.textContent = 'Uploading firmware...';
      resultEl.className = 'result';

      var formData = new FormData();
      formData.append('firmware', blob, 'firmware.bin');

      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/api/upload-firmware', true);

      xhr.upload.addEventListener('progress', function(e) {
        if (e.lengthComputable) {
          var pct = Math.round((e.loaded / e.total) * 100);
          progressFill.style.width = pct + '%';
          progressText.textContent = pct + '% (' + Math.round(e.loaded/1024) + ' / ' + Math.round(e.total/1024) + ' KB)';
        }
      });

      xhr.addEventListener('load', function() {
        if (xhr.status === 200) {
          progressFill.style.width = '100%';
          progressText.textContent = '100%';
          resultEl.textContent = 'Update successful! Device is rebooting...';
          resultEl.className = 'result success';

          // Wait for reboot then reload
          setTimeout(function() {
            resultEl.textContent += '\nReloading page in 10 seconds...';
          }, 3000);
          setTimeout(function() {
            window.location.reload();
          }, 13000);
        } else {
          var msg = 'Upload failed (HTTP ' + xhr.status + ')';
          try { var j = JSON.parse(xhr.responseText); if (j.error) msg = j.error; } catch(e){}
          resultEl.textContent = msg;
          resultEl.className = 'result error';
        }
      });

      xhr.addEventListener('error', function() {
        resultEl.textContent = 'Upload failed. Connection lost.';
        resultEl.className = 'result error';
      });

      xhr.send(formData);
    }
  </script>
</body>
</html>
)rawliteral";
