#pragma once

// Device configuration page served at GET /config
// Allows changing WiFi, MQTT, Spoolman, and hardware settings via web UI.

const char CONFIG_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Configuration &mdash; SpoolSense</title>
  <link rel="stylesheet" href="/css/shared.css" />
  <style>
    .toggle-row{
      display:flex;align-items:center;justify-content:space-between;
      padding:12px 14px;border:1px solid var(--border);border-radius:12px;
      background:rgba(255,255,255,.02);
    }
    .toggle-label{font-size:14px;font-weight:700;color:#e5e7eb}
    .toggle-switch{position:relative;width:44px;height:24px;cursor:pointer}
    .toggle-switch input{opacity:0;width:0;height:0}
    .toggle-track{
      position:absolute;top:0;left:0;right:0;bottom:0;
      background:#23262f;border-radius:12px;border:1px solid var(--border);
      transition:background .2s;
    }
    .toggle-track:before{
      content:'';position:absolute;width:18px;height:18px;left:3px;top:2px;
      background:#fff;border-radius:50%;transition:transform .2s;
    }
    .toggle-switch input:checked+.toggle-track{background:var(--green);border-color:var(--green)}
    .toggle-switch input:checked+.toggle-track:before{transform:translateX(19px)}
    .pass-wrap{position:relative}
    .pass-wrap input{padding-right:48px}
    .pass-toggle{
      position:absolute;right:12px;top:50%;transform:translateY(-50%);
      background:none;border:none;color:var(--muted);cursor:pointer;
      font-size:12px;font-weight:700;padding:4px;
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
      <a href="/config">Config</a>
    </nav>

    <section class="card">
      <div class="card-head">
        <h1 class="card-title">Device Configuration</h1>
        <p class="card-subtitle">Change settings and save to apply. Device will reboot after saving.</p>
      </div>

      <div class="card-body">
        <form id="configForm">
          <section>
            <h2 class="section-title">WiFi</h2>
            <div class="grid-2">
              <div class="field">
                <label for="wifi_ssid">SSID</label>
                <input id="wifi_ssid" type="text" placeholder="Your WiFi network" required />
              </div>
              <div class="field">
                <label for="wifi_pass">Password</label>
                <div class="pass-wrap">
                  <input id="wifi_pass" type="password" placeholder="Leave blank to keep current" />
                  <button type="button" class="pass-toggle" onclick="togglePass('wifi_pass',this)">Show</button>
                </div>
              </div>
            </div>
          </section>

          <section>
            <h2 class="section-title">MQTT / Home Assistant</h2>
            <div class="grid-2">
              <div class="field">
                <label for="mqtt_host">Broker Host</label>
                <input id="mqtt_host" type="text" placeholder="192.168.1.100" />
              </div>
              <div class="field">
                <label for="mqtt_port">Port</label>
                <input id="mqtt_port" type="number" min="1" max="65535" value="1883" />
              </div>
              <div class="field">
                <label for="mqtt_user">Username</label>
                <input id="mqtt_user" type="text" placeholder="Optional" />
              </div>
              <div class="field">
                <label for="mqtt_pass">Password</label>
                <div class="pass-wrap">
                  <input id="mqtt_pass" type="password" placeholder="Leave blank to keep current" />
                  <button type="button" class="pass-toggle" onclick="togglePass('mqtt_pass',this)">Show</button>
                </div>
              </div>
            </div>
          </section>

          <section>
            <h2 class="section-title">Spoolman</h2>
            <div class="toggle-row" style="margin-bottom:14px">
              <span class="toggle-label">Enable Spoolman</span>
              <label class="toggle-switch">
                <input type="checkbox" id="spoolman_on" />
                <span class="toggle-track"></span>
              </label>
            </div>
            <div class="field" id="spoolman_url_field">
              <label for="spoolman_url">Spoolman URL</label>
              <input id="spoolman_url" type="text" placeholder="http://spoolman.local:7912" />
            </div>
          </section>

          <section>
            <h2 class="section-title">Automation</h2>
            <div class="field">
              <label for="auto_mode">Mode</label>
              <select id="auto_mode">
                <option value="0">Self Directed &mdash; scanner auto-deducts filament weight</option>
                <option value="1">Controlled by HA &mdash; Home Assistant controls deduction</option>
              </select>
            </div>
          </section>

          <section>
            <h2 class="section-title">Hardware</h2>
            <div class="hint" style="margin-bottom:12px">LCD and LED settings are stored but require matching compile flags to take effect.</div>
            <div style="display:grid;gap:10px">
              <div class="toggle-row">
                <span class="toggle-label">LCD Display</span>
                <label class="toggle-switch">
                  <input type="checkbox" id="lcd_enabled" />
                  <span class="toggle-track"></span>
                </label>
              </div>
              <div class="toggle-row">
                <span class="toggle-label">Status LED</span>
                <label class="toggle-switch">
                  <input type="checkbox" id="led_enabled" />
                  <span class="toggle-track"></span>
                </label>
              </div>
            </div>
          </section>

          <div id="saveResult" class="hidden"></div>

          <div class="actions">
            <button type="submit" class="btn-primary" id="saveBtn">Save &amp; Reboot</button>
          </div>
        </form>
      </div>
    </section>

    <div class="footer-note">Settings are saved to NVS and persist across OTA updates.</div>
  </div>

  <script src="/js/shared.js"></script>
  <script>
    function togglePass(id, btn) {
      var el = document.getElementById(id);
      if (el.type === 'password') { el.type = 'text'; btn.textContent = 'Hide'; }
      else { el.type = 'password'; btn.textContent = 'Show'; }
    }

    // Load current config
    api('/api/config').then(function(cfg) {
      maybeSetValue('wifi_ssid', cfg.wifi_ssid);
      maybeSetValue('mqtt_host', cfg.mqtt_host);
      maybeSetValue('mqtt_port', cfg.mqtt_port);
      maybeSetValue('mqtt_user', cfg.mqtt_user);
      maybeSetValue('spoolman_url', cfg.spoolman_url);
      maybeSetValue('auto_mode', cfg.auto_mode);
      document.getElementById('spoolman_on').checked = !!cfg.spoolman_on;
      document.getElementById('lcd_enabled').checked = !!cfg.lcd_enabled;
      document.getElementById('led_enabled').checked = !!cfg.led_enabled;
      // Password placeholders
      if (cfg.wifi_pass_set) document.getElementById('wifi_pass').placeholder = '(set) Leave blank to keep';
      if (cfg.mqtt_pass_set) document.getElementById('mqtt_pass').placeholder = '(set) Leave blank to keep';
    }).catch(function() {});

    // Show/hide Spoolman URL based on toggle
    document.getElementById('spoolman_on').addEventListener('change', function() {
      document.getElementById('spoolman_url_field').style.display = this.checked ? '' : 'none';
    });

    document.getElementById('configForm').addEventListener('submit', function(e) {
      e.preventDefault();
      var btn = document.getElementById('saveBtn');
      var result = document.getElementById('saveResult');
      btn.disabled = true;
      btn.textContent = 'Saving...';

      var body = {
        wifi_ssid: document.getElementById('wifi_ssid').value.trim(),
        wifi_pass: document.getElementById('wifi_pass').value,
        mqtt_host: document.getElementById('mqtt_host').value.trim(),
        mqtt_port: parseInt(document.getElementById('mqtt_port').value) || 1883,
        mqtt_user: document.getElementById('mqtt_user').value.trim(),
        mqtt_pass: document.getElementById('mqtt_pass').value,
        spoolman_on: document.getElementById('spoolman_on').checked ? 1 : 0,
        spoolman_url: document.getElementById('spoolman_url').value.trim(),
        auto_mode: parseInt(document.getElementById('auto_mode').value) || 0,
        lcd_enabled: document.getElementById('lcd_enabled').checked ? 1 : 0,
        led_enabled: document.getElementById('led_enabled').checked ? 1 : 0
      };

      fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body)
      }).then(function(r) { return r.json(); })
        .then(function(data) {
          if (data.success) {
            result.textContent = 'Settings saved! Rebooting in 3 seconds...';
            result.className = 'result success';
            result.classList.remove('hidden');
            setTimeout(function() {
              result.textContent += '\nReloading page in 10 seconds...';
            }, 3000);
            setTimeout(function() { window.location.reload(); }, 13000);
          } else {
            throw new Error(data.error || 'Save failed');
          }
        })
        .catch(function(err) {
          btn.disabled = false;
          btn.textContent = 'Save & Reboot';
          result.textContent = err.message || 'Failed to save settings';
          result.className = 'result error';
          result.classList.remove('hidden');
        });
    });
  </script>
</body>
</html>
)rawliteral";
