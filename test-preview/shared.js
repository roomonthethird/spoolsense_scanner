/* ---- Nav bar highlight ---- */
(function(){
  var path = location.pathname;
  document.querySelectorAll('nav a[href]').forEach(function(a){
    if(a.getAttribute('href') === path) a.classList.add('active');
  });
})();

/* ---- API helper ---- */
function api(url, options) {
  return fetch(url, options).then(function(res){
    return res.json().then(function(data){
      if(!res.ok || (data && data.success === false))
        throw new Error((data && data.error) ? data.error : ('HTTP ' + res.status));
      return data;
    });
  });
}

function sleep(ms) {
  return new Promise(function(resolve){ setTimeout(resolve, ms); });
}

function nearlyEqual(a, b, tolerance) {
  return Math.abs(Number(a) - Number(b)) < tolerance;
}

/* ---- Color helpers ---- */
function normalizeHex(value) {
  var v = String(value || '').trim().toUpperCase();
  if (!v) return '#FF0000';
  if (!v.startsWith('#')) v = '#' + v;
  if (/^#[0-9A-F]{6}$/.test(v)) return v;
  return null;
}

function syncColorPicker(pickerId, hexId) {
  var picker = document.getElementById(pickerId);
  var hex = document.getElementById(hexId);
  if (!picker || !hex) return;

  picker.addEventListener('input', function(){
    hex.value = picker.value.toUpperCase();
  });
  hex.addEventListener('input', function(){
    var valid = normalizeHex(hex.value);
    if (valid) { hex.value = valid; picker.value = valid; }
  });
  hex.addEventListener('blur', function(){
    var valid = normalizeHex(hex.value);
    if (valid) { hex.value = valid; picker.value = valid; }
  });
}

/* ---- Form helpers ---- */
function readString(id) {
  var raw = document.getElementById(id).value.trim();
  return raw === '' ? undefined : raw;
}

function readPositiveNumber(id) {
  var raw = document.getElementById(id).value.trim();
  if (raw === '') return undefined;
  var num = Number(raw);
  if (!Number.isFinite(num) || num <= 0) return undefined;
  return num;
}

function readRequiredNumber(id) {
  var raw = document.getElementById(id).value.trim();
  var num = Number(raw);
  if (!Number.isFinite(num)) throw new Error('Invalid number for ' + id);
  return num;
}

function maybeSetValue(id, value) {
  if (value === undefined || value === null) return;
  var el = document.getElementById(id);
  if (el) el.value = value;
}

/* ---- Step indicators ---- */
function setStepState(id, state) {
  var el = document.getElementById(id);
  if (!el) return;
  el.classList.remove('active', 'done', 'error');
  if (state) el.classList.add(state);
}

function resetAllSteps(stepIds) {
  stepIds.forEach(function(id){ setStepState(id, ''); });
}

function setBanner(id, text) {
  var el = document.getElementById(id);
  if (el) el.textContent = text;
}

function setResult(id, text, type) {
  var el = document.getElementById(id);
  if (!el) return;
  el.textContent = text;
  el.className = 'result' + (type ? ' ' + type : '');
}

/* ---- Advanced toggle ---- */
function setupAdvancedToggle(toggleId, boxId) {
  var toggle = document.getElementById(toggleId);
  var box = document.getElementById(boxId);
  if (!toggle || !box) return;
  var textSpan = toggle.querySelector('[data-toggle-text]');

  function set(open) {
    box.classList.toggle('hidden', !open);
    toggle.setAttribute('aria-expanded', open ? 'true' : 'false');
    if (textSpan) textSpan.textContent = open ? 'Hide' : 'Show';
  }

  toggle.addEventListener('click', function(){
    set(box.classList.contains('hidden'));
  });

  return { open: function(){ set(true); }, close: function(){ set(false); } };
}

/* ---- Material data auto-fill ---- */

// Hardcoded fallback — works offline when API is unreachable
var _materialFallback = {
  'PLA':  {extruder_temp:240, bed_temp:55,  density:1.24},
  'PETG': {extruder_temp:270, bed_temp:75,  density:1.27},
  'ABS':  {extruder_temp:280, bed_temp:90,  density:1.04},
  'ASA':  {extruder_temp:280, bed_temp:100, density:1.05},
  'TPU':  {extruder_temp:250, bed_temp:40,  density:1.21},
  'PC':   {extruder_temp:290, bed_temp:115, density:1.3},
  'PCTG': {extruder_temp:270, bed_temp:75,  density:1.21},
  'PP':   {extruder_temp:250, bed_temp:60,  density:0.9},
  'HIPS': {extruder_temp:270, bed_temp:95,  density:1.03},
  'PVA':  {extruder_temp:240, bed_temp:60,  density:1.23},
  'PEEK': {extruder_temp:410, bed_temp:145, density:1.32},
  'PA6':  {extruder_temp:300, bed_temp:45,  density:1.52},
  'PA12': {extruder_temp:300, bed_temp:45,  density:1.52},
  'PEI':  {extruder_temp:380, bed_temp:130, density:1.27},
  'CPE':  {extruder_temp:270, bed_temp:80,  density:1.25}
};
var _materialDb = {};
var _materialDbLoaded = false;

function loadMaterialDb() {
  if (_materialDbLoaded) return Promise.resolve(_materialDb);
  // Start with fallback data
  Object.keys(_materialFallback).forEach(function(k) {
    _materialDb[k] = _materialFallback[k];
  });
  return fetch('https://api.tigertag.io/api:tigertag/SpoolmanDB/materials')
    .then(function(r) { return r.ok ? r.json() : []; })
    .then(function(data) {
      if (Array.isArray(data)) {
        data.forEach(function(m) {
          if (m.material) _materialDb[m.material.toUpperCase()] = m;
        });
      }
      _materialDbLoaded = true;
      return _materialDb;
    })
    .catch(function() {
      _materialDbLoaded = true;
      return _materialDb;
    });
}

function lookupMaterial(name) {
  if (!name) return null;
  var key = name.toUpperCase().replace(/\s+/g, '');
  // Exact match first
  if (_materialDb[key]) return _materialDb[key];
  // Try with common separators
  var withDash = key.replace(/([A-Z]+)(\d)/, '$1-$2');
  if (_materialDb[withDash]) return _materialDb[withDash];
  // Prefix match (e.g. "PA6 (Nylon 6)" → "PA6")
  var prefix = key.split(/[^A-Z0-9-]/)[0];
  if (prefix && _materialDb[prefix]) return _materialDb[prefix];
  return null;
}

// Track which fields the user has manually edited
function trackAutoFill(fieldIds) {
  fieldIds.forEach(function(id) {
    var el = document.getElementById(id);
    if (!el) return;
    el.dataset.autoFilled = 'true';
    el.addEventListener('input', function() {
      el.dataset.autoFilled = 'false';
    });
    el.addEventListener('change', function() {
      if (el.value === '') el.dataset.autoFilled = 'true';
    });
  });
}

function autoFillMaterialData(materialName, fieldMap) {
  var m = lookupMaterial(materialName);
  if (!m) return;
  if (m.extruder_temp) {
    if (fieldMap.minPrintTemp) {
      var el = document.getElementById(fieldMap.minPrintTemp);
      if (el && el.dataset.autoFilled !== 'false') {
        el.value = Math.max(0, m.extruder_temp - 10);
        el.dataset.autoFilled = 'true';
      }
    }
    if (fieldMap.maxPrintTemp) {
      var el = document.getElementById(fieldMap.maxPrintTemp);
      if (el && el.dataset.autoFilled !== 'false') {
        el.value = m.extruder_temp + 10;
        el.dataset.autoFilled = 'true';
      }
    }
  }
  if (m.bed_temp) {
    if (fieldMap.minBedTemp) {
      var el = document.getElementById(fieldMap.minBedTemp);
      if (el && el.dataset.autoFilled !== 'false') {
        el.value = Math.max(0, m.bed_temp - 5);
        el.dataset.autoFilled = 'true';
      }
    }
    if (fieldMap.maxBedTemp) {
      var el = document.getElementById(fieldMap.maxBedTemp);
      if (el && el.dataset.autoFilled !== 'false') {
        el.value = m.bed_temp + 5;
        el.dataset.autoFilled = 'true';
      }
    }
  }
  if (m.density && fieldMap.density) {
    var el = document.getElementById(fieldMap.density);
    if (el && el.dataset.autoFilled !== 'false') {
      el.value = m.density;
      el.dataset.autoFilled = 'true';
    }
  }
}

/* ---- Tag kind labels ---- */
var TAG_KIND_LABELS = {
  'OpenPrintTag': 'OpenPrintTag',
  'GenericUidTag': 'Generic UID Tag',
  'TigerTag': 'TigerTag',
  'BambuTag': 'Bambu Lab Tag',
  'BlankTag': 'Blank Tag',
  'OpenTag3D': 'OpenTag3D',
  'Unsupported': 'Unsupported'
};

function tagKindLabel(kind) {
  return TAG_KIND_LABELS[kind] || kind || 'Unknown';
}
