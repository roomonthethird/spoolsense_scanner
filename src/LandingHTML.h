#pragma once

// Landing page served at GET /
// Shows SpoolSense logo and links to all tools.

const char LANDING_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>SpoolSense Scanner</title>
  <link rel="stylesheet" href="/css/shared.css?v=)rawliteral" FIRMWARE_VERSION R"rawliteral(" />
</head>
<body>
  <div class="wrap">
    <nav>
      <span class="nav-brand">SpoolSense</span>
      <a href="/" class="active">Home</a>
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

    <div style="text-align:center;margin:32px 0 28px">
      <svg width="280" height="64" viewBox="0 0 280 64" fill="none" xmlns="http://www.w3.org/2000/svg" role="img" aria-label="SpoolSense Scanner logo">
        <defs>
          <linearGradient id="gRed" x1="0" x2="1">
            <stop offset="0" stop-color="#EF4444"/>
            <stop offset="1" stop-color="#DC2626"/>
          </linearGradient>
        </defs>
        <circle cx="30" cy="30" r="20" fill="#111318" stroke="#2A2E36" stroke-width="2.5"/>
        <circle cx="30" cy="30" r="7" fill="#0B0B0D" stroke="#52525B" stroke-width="2.5"/>
        <path d="M16 19C19.5 15.5 25 13 30 13C40 13 48 21 48 31" stroke="url(#gRed)" stroke-width="5" stroke-linecap="round"/>
        <path d="M13 29C13 39 21 47 31 47C36 47 41 45 45 42" stroke="url(#gRed)" stroke-width="5" stroke-linecap="round"/>
        <path d="M60 16C67 16 73 21 73 30" stroke="url(#gRed)" stroke-width="3.5" stroke-linecap="round"/>
        <path d="M65 11C74 11 82 19 82 30" stroke="url(#gRed)" stroke-width="3.5" stroke-linecap="round" opacity=".85"/>
        <text x="96" y="33" fill="#F4F4F5" font-size="26" font-weight="800" font-family="Inter, Arial, sans-serif">SpoolSense</text>
        <text x="96" y="52" fill="#EF4444" font-size="13" font-weight="800" font-family="Inter, Arial, sans-serif" letter-spacing="2">SCANNER</text>
      </svg>
    </div>

    <div class="tool-grid">
      <a href="/reader" class="tool-card">
        <div class="tool-icon">&#128270;</div>
        <div class="tool-name">Tag Reader</div>
        <div class="tool-desc">Auto-detect and display tag data. Supports OpenPrintTag, TigerTag, and generic UID tags.</div>
      </a>

      <a href="/writer/openprinttag" class="tool-card">
        <img src="/img/openprinttag.png" alt="OpenPrintTag" style="height:48px;border-radius:6px" />
        <div class="tool-name">OpenPrintTag Writer</div>
        <div class="tool-desc">Write filament data to ISO15693 tags using the OpenPrintTag format.</div>
      </a>

      <a href="/writer/tigertag" class="tool-card">
        <img src="/img/tigertag.png" alt="TigerTag" style="height:56px;border-radius:6px" />
        <div class="tool-name">TigerTag Writer</div>
        <div class="tool-desc">Write filament data to NTAG213/215 tags using the TigerTag binary format.</div>
      </a>

      <a href="/writer/opentag3d" class="tool-card">
        <img src="/img/opentag3d.png" alt="OpenTag3D" style="height:48px;border-radius:6px" />
        <div class="tool-name">OpenTag3D Writer</div>
        <div class="tool-desc">Write filament data to NTAG215/216 tags using the OpenTag3D format.</div>
      </a>

      <a href="/writer/openspool" class="tool-card">
        <img src="/img/openspool.png" alt="OpenSpool" style="height:48px;border-radius:6px" />
        <div class="tool-name">OpenSpool Writer</div>
        <div class="tool-desc">Write filament data to NTAG215/216 tags using the OpenSpool format.</div>
      </a>

      <a href="/register/uid" class="tool-card">
        <div class="tool-icon">&#128179;</div>
        <div class="tool-name">NFC+ Registration</div>
        <div class="tool-desc">Register a plain NFC tag in Spoolman using its UID. No data written to the tag.</div>
      </a>

      <a href="/update" class="tool-card">
        <div class="tool-icon">&#9889;</div>
        <div class="tool-name">Firmware Update</div>
        <div class="tool-desc">Check for new firmware versions and update over WiFi.</div>
      </a>

      <a href="/troubleshooting" class="tool-card">
        <div class="tool-icon">&#128736;</div>
        <div class="tool-name">Troubleshooting</div>
        <div class="tool-desc">Verify scanner connectivity, MQTT, Spoolman, and NFC reader status.</div>
      </a>

      <a href="/logs" class="tool-card">
        <div class="tool-icon">&#128220;</div>
        <div class="tool-name">Serial Log</div>
        <div class="tool-desc">View live scanner serial output for debugging.</div>
      </a>

      <a href="/config" class="tool-card">
        <div class="tool-icon">&#9881;</div>
        <div class="tool-name">Configuration</div>
        <div class="tool-desc">Change WiFi, MQTT, Spoolman, and hardware settings.</div>
      </a>
    </div>

    <div class="card" style="margin-top:24px;padding:16px 20px">
      <div style="display:flex;justify-content:space-between;align-items:center;flex-wrap:wrap;gap:8px">
        <div>
          <div style="font-size:11px;color:#A1A1AA;text-transform:uppercase;letter-spacing:1px">Device ID</div>
          <div id="deviceId" style="font-size:20px;font-weight:700;font-family:monospace;color:#F4F4F5;user-select:all">loading&hellip;</div>
        </div>
        <div style="text-align:right">
          <div style="font-size:11px;color:#A1A1AA;text-transform:uppercase;letter-spacing:1px">Firmware</div>
          <div id="fwVersion" style="font-size:14px;color:#A1A1AA">--</div>
        </div>
      </div>
      <div style="margin-top:8px;font-size:12px;color:#71717A">Use this Device ID when configuring the SpoolSense middleware.</div>
    </div>

    <div class="footer-note" id="footerHostname" style="margin-top:28px">SpoolSense Scanner &mdash; spoolsense.local</div>
  </div>

  <script src="/js/shared.js?v=)rawliteral" FIRMWARE_VERSION R"rawliteral("></script>
  <script>
    fetch('/api/status').then(function(r){return r.json()}).then(function(d){
      if(d.device_id) document.getElementById('deviceId').textContent = d.device_id;
      if(d.firmware_version) document.getElementById('fwVersion').textContent = 'v' + d.firmware_version;
    }).catch(function(){});
    fetch('/api/config').then(function(r){return r.json()}).then(function(cfg){
      if(cfg.hostname) document.getElementById('footerHostname').textContent = 'SpoolSense Scanner \u2014 ' + cfg.hostname + '.local';
    }).catch(function(){});
  </script>
</body>
</html>
)rawliteral";
