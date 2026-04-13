#ifndef LogViewerHTML_h
#define LogViewerHTML_h

#include <Arduino.h>

const char LOG_VIEWER_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Serial Log &mdash; SpoolSense</title>
    <link rel="stylesheet" href="/css/shared.css?v=)=====" FIRMWARE_VERSION R"=====(" />
    <style>
        .subtitle {
            color: var(--muted);
            font-size: 14px;
            margin-bottom: 16px;
        }
        .controls {
            display: flex;
            align-items: center;
            gap: 14px;
            margin-bottom: 12px;
            font-size: 13px;
            color: var(--muted);
        }
        .controls label { cursor: pointer; }
        .log-btn {
            font-size: 11px;
            padding: 4px 10px;
            background: rgba(99,102,241,.2);
            color: #a5b4fc;
            border: 1px solid rgba(99,102,241,.4);
            border-radius: 6px;
            cursor: pointer;
        }
        .log-btn:hover { background: rgba(99,102,241,.35); }
        pre {
            background: var(--panel);
            padding: 14px;
            border-radius: var(--radius);
            overflow: auto;
            max-height: 70vh;
            font-family: 'SF Mono', 'Fira Code', 'Cascadia Code', monospace;
            font-size: 12px;
            line-height: 1.5;
            white-space: pre-wrap;
            word-wrap: break-word;
            border: 1px solid var(--border);
            color: var(--muted);
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
            <a href="/writer/openspool">OpenSpool</a>
            <a href="/register/uid">NFC+</a>
            <a href="/update">Update</a>
            <a href="/troubleshooting">Troubleshooting</a>
            <a href="/logs" class="active">Logs</a>
            <a href="/config">Config</a>
        </nav>

        <h2 style="margin-bottom:6px">Serial Log</h2>
        <p class="subtitle">Live scanner output for debugging. Auto-refreshes every 2 seconds.</p>

        <div class="controls">
            <label><input type="checkbox" id="autoscroll" checked> Auto-scroll</label>
            <button class="log-btn" onclick="copyLogs()">Copy</button>
            <button class="log-btn" onclick="clearLogs()">Clear</button>
        </div>
        <pre id="logOutput">(empty)</pre>
    </div>
    <script>
        var logEl = document.getElementById('logOutput');
        var autoEl = document.getElementById('autoscroll');
        function fetchLogs() {
            fetch('/api/logs')
                .then(function(r) { return r.text(); })
                .then(function(data) {
                    logEl.textContent = data || '(empty)';
                    if (autoEl.checked) logEl.scrollTop = logEl.scrollHeight;
                })
                .catch(function(e) { logEl.textContent = 'Error: ' + e.message; });
        }
        function copyLogs() {
            var text = logEl.textContent;
            if (navigator.clipboard) {
                navigator.clipboard.writeText(text).then(function() {
                    var btns = document.querySelectorAll('.log-btn');
                    btns[0].textContent = 'Copied!';
                    setTimeout(function() { btns[0].textContent = 'Copy'; }, 1500);
                });
            } else {
                var ta = document.createElement('textarea');
                ta.value = text;
                document.body.appendChild(ta);
                ta.select();
                document.execCommand('copy');
                document.body.removeChild(ta);
            }
        }
        function clearLogs() {
            fetch('/api/logs/clear', { method: 'POST' })
                .then(function() { fetchLogs(); });
        }
        fetchLogs();
        setInterval(fetchLogs, 2000);
    </script>
</body>
</html>
)=====";

#endif
