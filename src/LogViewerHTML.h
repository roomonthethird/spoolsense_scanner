#ifndef LogViewerHTML_h
#define LogViewerHTML_h

#include <Arduino.h>

const char LOG_VIEWER_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Serial Log</title>
    <style>
        :root {
            --bg: #0b0b0d;
            --panel: #141519;
            --panel-2: #1a1c21;
            --border: #2a2e36;
            --text: #f4f4f5;
            --muted: #a1a1aa;
            --blue: #3b82f6;
            --radius: 16px;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; }
        body {
            background: var(--bg);
            color: var(--text);
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
        }
        .wrap {
            max-width: 800px;
            margin: 0 auto;
            padding: 20px 16px;
        }
        nav {
            display: flex;
            flex-wrap: wrap;
            gap: 8px 14px;
            margin-bottom: 24px;
            font-size: 13px;
            font-weight: 600;
        }
        nav a {
            color: var(--muted);
            text-decoration: none;
        }
        nav a:hover, nav a.active {
            color: var(--text);
        }
        .nav-brand {
            color: var(--blue);
            font-weight: 800;
            margin-right: 8px;
        }
        h2 { font-size: 1.3em; margin-bottom: 6px; }
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
        .btn {
            background: var(--blue);
            color: #fff;
            border: none;
            padding: 6px 16px;
            cursor: pointer;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
        }
        .btn:hover { opacity: 0.88; }
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

        <h2>Serial Log</h2>
        <p class="subtitle">Live scanner output for debugging. Auto-refreshes every 2 seconds.</p>

        <div class="controls">
            <label><input type="checkbox" id="autoscroll" checked> Auto-scroll</label>
            <button class="btn" onclick="clearLogs()">Clear</button>
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
