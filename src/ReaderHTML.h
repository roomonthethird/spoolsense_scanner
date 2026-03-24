#pragma once

// Tag Reader page served at GET /reader
// Auto-detects tag format and displays all data read-only.

const char READER_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>Tag Reader &mdash; SpoolSense</title>
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
      <a href="/register/uid">NFC+</a>
      <a href="/update">Update</a>
      <a href="/troubleshooting">Troubleshooting</a>
      <a href="/config">Config</a>
    </nav>

    <section class="card">
      <div class="card-head">
        <h1 class="card-title">Tag Reader</h1>
        <p class="card-subtitle">Place a tag on the scanner to read its contents.</p>
      </div>

      <div class="card-body">
        <!-- No tag state -->
        <div id="noTag">
          <div class="status-banner">Waiting for tag&hellip;</div>
        </div>

        <!-- Tag present -->
        <div id="tagView" class="hidden">
          <div class="tag-info" id="tagFields"></div>
        </div>
      </div>
    </section>

    <div class="actions" style="margin-top:16px;justify-content:center">
      <button type="button" class="btn-primary hidden" id="scanBtn">Scan Again</button>
    </div>

    <div class="footer-note">Stops polling once a tag is detected.</div>
  </div>

  <script src="/js/shared.js"></script>
  <script>
    var noTag = document.getElementById('noTag');
    var tagView = document.getElementById('tagView');
    var tagFields = document.getElementById('tagFields');
    var lastJson = '';
    var pollTimer = null;
    var tagFound = false;

    function row(label, value) {
      return '<div class="tag-row"><span class="label">' + label + '</span><span class="value">' + value + '</span></div>';
    }

    function colorValue(hex) {
      if (!hex) return '&mdash;';
      return '<span class="color-swatch" style="background:' + hex + '"></span>' + hex;
    }

    function renderOpenPrintTag(s) {
      var html = '';
      html += row('Format', 'OpenPrintTag');
      html += row('UID', s.uid || '&mdash;');
      html += row('Material Type', s.material_name || ('Type ' + s.material_type));
      if (s.color) html += row('Color', colorValue(s.color));
      if (s.manufacturer) html += row('Manufacturer', s.manufacturer);
      html += row('Initial Weight', (s.initial_weight_g !== undefined ? s.initial_weight_g + ' g' : '&mdash;'));
      html += row('Remaining', (s.remaining_g !== undefined ? s.remaining_g.toFixed(1) + ' g' : '&mdash;'));
      if (s.density) html += row('Density', s.density + ' g/cm\u00B3');
      if (s.diameter_mm) html += row('Diameter', s.diameter_mm + ' mm');
      if (s.min_print_temp) html += row('Print Temp', s.min_print_temp + ' \u2013 ' + (s.max_print_temp || '?') + ' \u00B0C');
      if (s.preheat_temp) html += row('Preheat', s.preheat_temp + ' \u00B0C');
      if (s.min_bed_temp) html += row('Bed Temp', s.min_bed_temp + ' \u2013 ' + (s.max_bed_temp || '?') + ' \u00B0C');
      if (s.spoolman_id > 0) html += row('Spoolman ID', s.spoolman_id);
      return html;
    }

    function renderTigerTag(s) {
      var t = s.tigertag || {};
      var html = '';
      html += row('Format', 'TigerTag');
      html += row('UID', s.uid || '&mdash;');
      if (t.material_name) html += row('Material', t.material_name);
      if (t.brand_name) html += row('Brand', t.brand_name);
      if (t.color_hex) html += row('Color', colorValue(t.color_hex));
      if (t.weight_g !== undefined) html += row('Weight', t.weight_g + ' g');
      if (t.diameter_mm) html += row('Diameter', t.diameter_mm + ' mm');
      if (t.aspect1_name && t.aspect1_name !== 'Unknown') html += row('Aspect', t.aspect1_name + (t.aspect2_name && t.aspect2_name !== 'Unknown' ? ' / ' + t.aspect2_name : ''));
      if (t.nozzle_temp_min) html += row('Nozzle Temp', t.nozzle_temp_min + ' \u2013 ' + (t.nozzle_temp_max || '?') + ' \u00B0C');
      if (t.bed_temp_min) html += row('Bed Temp', t.bed_temp_min + ' \u2013 ' + (t.bed_temp_max || '?') + ' \u00B0C');
      if (t.dry_temp) html += row('Dry', t.dry_temp + ' \u00B0C / ' + (t.dry_time_hours || '?') + ' hrs');
      return html;
    }

    function renderOpenTag3D(s) {
      var t = s.opentag3d || {};
      var html = '';
      html += row('Format', 'OpenTag3D');
      html += row('UID', s.uid || '&mdash;');
      if (t.base_material) html += row('Material', t.base_material + (t.modifiers ? ' (' + t.modifiers + ')' : ''));
      if (t.manufacturer) html += row('Manufacturer', t.manufacturer);
      if (t.color_hex) html += row('Color', colorValue(t.color_hex) + (t.color_name ? ' ' + t.color_name : ''));
      if (t.target_weight_g !== undefined) html += row('Weight', t.target_weight_g + ' g');
      if (t.measured_weight_g) html += row('Measured Weight', t.measured_weight_g + ' g');
      if (t.diameter_mm) html += row('Diameter', t.diameter_mm + ' mm');
      if (t.density) html += row('Density', t.density.toFixed(3) + ' g/cm\u00B3');
      if (t.print_temp) {
        var tempStr = t.print_temp + ' \u00B0C';
        if (t.min_print_temp && t.max_print_temp) tempStr = t.min_print_temp + ' \u2013 ' + t.max_print_temp + ' \u00B0C';
        html += row('Print Temp', tempStr);
      }
      if (t.bed_temp) {
        var bedStr = t.bed_temp + ' \u00B0C';
        if (t.min_bed_temp && t.max_bed_temp) bedStr = t.min_bed_temp + ' \u2013 ' + t.max_bed_temp + ' \u00B0C';
        html += row('Bed Temp', bedStr);
      }
      if (t.dry_temp) html += row('Dry', t.dry_temp + ' \u00B0C / ' + (t.dry_time_hours || '?') + ' hrs');
      if (t.serial_number) html += row('Serial', t.serial_number);
      if (t.empty_spool_g) html += row('Empty Spool', t.empty_spool_g + ' g');
      return html;
    }

    function renderGenericUid(s) {
      var html = '';
      html += row('Format', tagKindLabel(s.tag_kind));
      html += row('UID', s.uid || '&mdash;');
      if (!s.tag_data_valid) html += row('Data', 'No parseable data');
      return html;
    }

    function render(s) {
      if (!s.present) {
        noTag.classList.remove('hidden');
        tagView.classList.add('hidden');
        return;
      }

      noTag.classList.add('hidden');
      tagView.classList.remove('hidden');

      var html;
      var kind = s.tag_kind || '';

      if (kind === 'TigerTag' && s.tigertag) {
        html = renderTigerTag(s);
      } else if (kind === 'OpenTag3D' && s.opentag3d) {
        html = renderOpenTag3D(s);
      } else if (kind === 'OpenPrintTag' || (s.tag_data_valid && !kind)) {
        html = renderOpenPrintTag(s);
      } else {
        html = renderGenericUid(s);
      }

      tagFields.innerHTML = html;
    }

    function stopPolling() {
      if (pollTimer) { clearInterval(pollTimer); pollTimer = null; }
    }

    function startPolling() {
      stopPolling();
      tagFound = false;
      lastJson = '';
      noTag.classList.remove('hidden');
      tagView.classList.add('hidden');
      scanBtn.classList.add('hidden');
      pollTimer = setInterval(poll, 1000);
      poll();
    }

    function poll() {
      api('/api/status').then(function(s){
        if (s.present && !tagFound) {
          tagFound = true;
          stopPolling();
          render(s);
          scanBtn.classList.remove('hidden');
        } else if (!tagFound) {
          render(s);
        }
      }).catch(function(){});
    }

    var scanBtn = document.getElementById('scanBtn');
    scanBtn.addEventListener('click', startPolling);

    startPolling();
  </script>
</body>
</html>
)rawliteral";
