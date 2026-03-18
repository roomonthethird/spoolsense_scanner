#pragma once

// Shared CSS for the SpoolSense Scanner multi-page web UI.
// Served at GET /css/shared.css — referenced by all pages.

const char SHARED_CSS[] PROGMEM = R"rawliteral(
:root{
  --bg:#0b0b0d;
  --panel:#141519;
  --panel-2:#1a1c21;
  --border:#2a2e36;
  --text:#f4f4f5;
  --muted:#a1a1aa;
  --red:#dc2626;
  --red-2:#ef4444;
  --green:#22c55e;
  --blue:#3b82f6;
  --orange:#f59e0b;
  --radius:16px;
  --shadow:0 16px 40px rgba(0,0,0,.35);
}

*{box-sizing:border-box}
html,body{margin:0;padding:0}
body{
  font-family:Inter,system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;
  background:linear-gradient(180deg,#09090b,#111216);
  color:var(--text);
  min-height:100vh;
}

.wrap{
  max-width:860px;
  margin:0 auto;
  padding:24px 18px 40px;
}

/* ---- Nav bar ---- */
nav{
  display:flex;
  align-items:center;
  gap:6px;
  padding:10px 14px;
  margin-bottom:20px;
  border:1px solid var(--border);
  border-radius:var(--radius);
  background:var(--panel);
  overflow-x:auto;
}

nav a{
  color:var(--muted);
  text-decoration:none;
  font-size:13px;
  font-weight:700;
  padding:8px 14px;
  border-radius:10px;
  white-space:nowrap;
  transition:background .15s,color .15s;
}

nav a:hover{
  background:rgba(255,255,255,.06);
  color:var(--text);
}

nav a.active{
  background:rgba(239,68,68,.12);
  color:var(--red-2);
}

nav .nav-brand{
  font-size:15px;
  font-weight:800;
  color:var(--text);
  margin-right:auto;
  padding:6px 10px;
}

/* ---- Card ---- */
.card{
  background:linear-gradient(180deg,var(--panel),var(--panel-2));
  border:1px solid var(--border);
  border-radius:var(--radius);
  box-shadow:var(--shadow);
  overflow:hidden;
}

.card-head{
  padding:18px 20px;
  border-bottom:1px solid var(--border);
}

.card-title{
  margin:0;
  font-size:22px;
  font-weight:800;
  letter-spacing:-.02em;
}

.card-subtitle{
  margin:6px 0 0;
  color:var(--muted);
  font-size:14px;
}

.card-body{
  padding:20px;
}

.hidden{display:none !important}

/* ---- Form ---- */
form{
  display:grid;
  gap:20px;
}

.section-title{
  margin:0 0 12px;
  font-size:14px;
  font-weight:800;
  color:#fff;
  text-transform:uppercase;
  letter-spacing:.08em;
}

.grid-2,.grid-3{
  display:grid;
  gap:14px;
}

.grid-2{grid-template-columns:repeat(2,minmax(0,1fr))}
.grid-3{grid-template-columns:repeat(3,minmax(0,1fr))}

@media (max-width:760px){
  .grid-2,.grid-3{grid-template-columns:1fr}
}

.field{
  display:grid;
  gap:7px;
}

.field label{
  font-size:13px;
  font-weight:700;
  color:#e5e7eb;
}

.field input,
.field select{
  width:100%;
  border:1px solid var(--border);
  border-radius:12px;
  background:#0f1115;
  color:var(--text);
  padding:12px 13px;
  font-size:14px;
  outline:none;
}

.field input:focus,
.field select:focus{
  border-color:var(--blue);
}

.field input[type="color"]{
  min-height:46px;
  padding:6px;
}

.hint{
  color:var(--muted);
  font-size:12px;
  line-height:1.4;
}

.color-row{
  display:grid;
  grid-template-columns:92px 1fr;
  gap:10px;
}

.advanced-toggle{
  width:100%;
  display:flex;
  align-items:center;
  justify-content:space-between;
  gap:12px;
  border:1px solid var(--border);
  border-radius:14px;
  background:#12141a;
  color:var(--text);
  padding:14px 16px;
  cursor:pointer;
  font-size:14px;
  font-weight:800;
}

.advanced-box{
  margin-top:12px;
  padding:16px;
  border:1px solid var(--border);
  border-radius:14px;
  background:rgba(255,255,255,.02);
}

/* ---- Buttons ---- */
.actions{
  display:flex;
  flex-wrap:wrap;
  gap:10px;
}

button{
  border:0;
  border-radius:12px;
  padding:12px 16px;
  font-size:14px;
  font-weight:800;
  cursor:pointer;
  transition:transform .05s ease, opacity .15s ease;
}

button:active{transform:translateY(1px)}
button:disabled{opacity:.55;cursor:not-allowed}

.btn-primary{
  background:linear-gradient(180deg,var(--red-2),var(--red));
  color:#fff;
}

.btn-secondary{
  background:#23262f;
  color:#fff;
  border:1px solid #353946;
}

.btn-ghost{
  background:transparent;
  color:#fff;
  border:1px solid var(--border);
}

/* ---- Status / Steps ---- */
.status-wrap{
  display:grid;
  gap:16px;
}

.status-banner{
  padding:14px 16px;
  border-radius:14px;
  border:1px solid rgba(59,130,246,.28);
  background:rgba(59,130,246,.10);
  color:#dbeafe;
  font-weight:700;
  white-space:pre-wrap;
}

.steps{
  display:grid;
  gap:12px;
}

.step{
  display:flex;
  align-items:center;
  gap:12px;
  padding:13px 14px;
  border-radius:14px;
  border:1px solid var(--border);
  background:rgba(255,255,255,.02);
}

.dot{
  width:14px;
  height:14px;
  border-radius:50%;
  border:2px solid #52525b;
  background:transparent;
  flex:0 0 14px;
}

.step.active .dot{
  border-color:var(--blue);
  background:var(--blue);
  box-shadow:0 0 0 4px rgba(59,130,246,.16);
}

.step.done .dot{
  border-color:var(--green);
  background:var(--green);
  box-shadow:0 0 0 4px rgba(34,197,94,.16);
}

.step.error .dot{
  border-color:var(--red-2);
  background:var(--red-2);
  box-shadow:0 0 0 4px rgba(239,68,68,.16);
}

.step-title{
  font-size:14px;
  font-weight:800;
  color:#fff;
}

.step-sub{
  margin-top:3px;
  font-size:12px;
  color:var(--muted);
}

.result{
  padding:14px 16px;
  border-radius:14px;
  border:1px solid var(--border);
  background:rgba(255,255,255,.03);
  white-space:pre-wrap;
  color:var(--muted);
}

.result.success{
  color:#d1fae5;
  border-color:rgba(34,197,94,.28);
  background:rgba(34,197,94,.10);
}

.result.error{
  color:#fee2e2;
  border-color:rgba(239,68,68,.28);
  background:rgba(239,68,68,.10);
}

/* ---- Reader ---- */
.tag-info{
  display:grid;
  gap:12px;
}

.tag-row{
  display:flex;
  justify-content:space-between;
  align-items:center;
  padding:10px 14px;
  border-radius:12px;
  border:1px solid var(--border);
  background:rgba(255,255,255,.02);
}

.tag-row .label{
  font-size:13px;
  font-weight:700;
  color:var(--muted);
}

.tag-row .value{
  font-size:14px;
  font-weight:800;
  color:var(--text);
}

.color-swatch{
  display:inline-block;
  width:20px;
  height:20px;
  border-radius:6px;
  border:2px solid var(--border);
  vertical-align:middle;
  margin-right:8px;
}

/* ---- Landing ---- */
.tool-grid{
  display:grid;
  grid-template-columns:repeat(auto-fit, minmax(220px, 1fr));
  gap:14px;
}

.tool-card{
  display:flex;
  flex-direction:column;
  align-items:center;
  gap:12px;
  padding:24px 18px;
  border:1px solid var(--border);
  border-radius:var(--radius);
  background:linear-gradient(180deg,var(--panel),var(--panel-2));
  text-decoration:none;
  color:var(--text);
  transition:border-color .15s, transform .1s;
}

.tool-card:hover{
  border-color:var(--red-2);
  transform:translateY(-2px);
}

.tool-card .tool-icon{
  font-size:32px;
}

.tool-card .tool-name{
  font-size:16px;
  font-weight:800;
}

.tool-card .tool-desc{
  font-size:13px;
  color:var(--muted);
  text-align:center;
}

/* ---- Write warning ---- */
.write-warning{
  padding:14px 16px;
  border-radius:14px;
  border:1px solid rgba(245,158,11,.28);
  background:rgba(245,158,11,.08);
  color:#fef3c7;
  font-weight:700;
  font-size:14px;
  text-align:center;
}

.footer-note{
  margin-top:10px;
  color:var(--muted);
  font-size:12px;
  text-align:center;
}

/* ---- Logo header ---- */
.page-logo{
  display:flex;
  align-items:center;
  justify-content:center;
  gap:14px;
  margin:0 0 20px;
}

.page-logo img{
  max-height:48px;
  width:auto;
}

.page-logo .logo-text{
  font-size:22px;
  font-weight:800;
  letter-spacing:-.02em;
}
)rawliteral";
