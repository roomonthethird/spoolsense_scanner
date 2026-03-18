#pragma once

// Shared JavaScript utilities for the SpoolSense Scanner multi-page web UI.
// Served at GET /js/shared.js — referenced by all pages.

const char SHARED_JS[] PROGMEM = R"rawliteral(
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

/* ---- Tag kind labels ---- */
var TAG_KIND_LABELS = {
  'OpenPrintTag': 'OpenPrintTag',
  'GenericUidTag': 'Generic UID Tag',
  'TigerTag': 'TigerTag',
  'BlankTag': 'Blank Tag',
  'OpenTag3D': 'OpenTag3D',
  'Unsupported': 'Unsupported'
};

function tagKindLabel(kind) {
  return TAG_KIND_LABELS[kind] || kind || 'Unknown';
}
)rawliteral";
