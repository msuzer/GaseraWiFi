#include "WebUI.h"

const char htmlPage[] PROGMEM = R"rawliteral(
<!-- Embedded HTML UI for ESP32 - Mobile Friendly -->
<!DOCTYPE html>
<html lang="en" data-bs-theme="light">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Gasera UI</title>
  <link href="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css" rel="stylesheet">
  <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
  <script src="https://cdn.jsdelivr.net/npm/chartjs-plugin-zoom@2.0.1/dist/chartjs-plugin-zoom.min.js"></script>
  <style>
    .card pre { background: #f8f9fa; padding: 1rem; border-radius: .25rem; max-height: 300px; overflow: auto;}
    #connectionStatus { position: fixed; top: 0; right: 0; margin: 1rem; padding: 0.5rem 1rem; font-weight: bold; border-radius: .25rem; }
    .connected { background-color: #d1e7dd; color: #0f5132; }
    .disconnected { background-color: #f8d7da; color: #842029; }
  </style>
</head>
<body class="bg-light">
  <div id="connectionStatus" class="disconnected">Disconnected</div>
  <div class="container py-4">
    <div class="d-flex justify-content-between align-items-center mb-4">
      <h2 class="mb-0">Gasera Monitoring Panel</h2>
      <button class="btn btn-outline-dark btn-sm" onclick="toggleTheme()">Toggle Theme</button>
    </div>

    <!-- Nav Tabs -->
    <ul class="nav nav-tabs mb-3" id="tabMenu">
      <li class="nav-item"><button class="nav-link active" data-bs-toggle="tab" data-bs-target="#status">Status</button></li>
	  <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#jsonTab">JSON</button></li>
      <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#graphTab">Graph</button></li>
      <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#results">Results</button></li>
      <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#errors">Errors</button></li>
      <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#device">Device Info</button></li>
      <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#network">Network</button></li>
      <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#control">Control</button></li>
      <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#components">Components</button></li>
      <li class="nav-item"><button class="nav-link" data-bs-toggle="tab" data-bs-target="#settings">System Settings</button></li>
    </ul>

    <div class="tab-content">
	
      <div class="tab-pane fade show active" id="status">
        <div class="card mb-3">
          <div class="card-header">Device Status</div>
          <div class="card-body">
            <pre id="statusBox">Loading...</pre>
            <button class="btn btn-primary btn-sm" onclick="loadStatus()">Refresh</button>
          </div>
        </div>
      </div>

      <div class="tab-pane fade" id="jsonTab" role="tabpanel">
        <div class="card mb-3">
          <div class="card-header d-flex justify-content-between align-items-center">
            <span>Last Results (JSON)</span>
            <div>
              <button class="btn btn-sm btn-outline-secondary me-2" onclick="downloadResults('json')">Download JSON</button>
              <button class="btn btn-sm btn-outline-secondary" onclick="downloadResults('csv')">Download CSV</button>
            </div>
          </div>
          <div class="card-body">
            <input type="text" class="form-control mb-2" placeholder="Filter results..." oninput="filterResults(this.value)">
            <pre id="resultsBox">Loading...</pre>
            <button class="btn btn-primary btn-sm mt-2" onclick="loadResults()">Refresh</button>
          </div>
        </div>
      </div>

      <div class="tab-pane fade" id="graphTab" role="tabpanel">
        <div class="card mb-3">
          <div class="card-header d-flex justify-content-between align-items-center">
            <span>Results Chart</span>
            <div>
              <button class="btn btn-sm btn-outline-secondary me-2" onclick="saveChartImage()">Save as Image</button>
              <button class="btn btn-sm btn-outline-secondary" onclick="resetZoom()">Reset Zoom</button>
            </div>
          </div>
          <div class="card-body">
            <canvas id="resultsChart" height="200"></canvas>
          </div>
        </div>
      </div>

      <div class="tab-pane fade" id="results">
        <div class="card mb-3">
          <div class="card-header">Last Results</div>
          <div class="card-body">
            <pre id="resultsBox">Loading...</pre>
            <button class="btn btn-primary btn-sm" onclick="loadResults()">Refresh</button>
          </div>
        </div>
      </div>

      <div class="tab-pane fade" id="errors">
        <div class="card mb-3">
          <div class="card-header">Active Errors</div>
          <div class="card-body">
            <pre id="errorsBox">Loading...</pre>
            <button class="btn btn-primary btn-sm" onclick="loadErrors()">Refresh</button>
          </div>
        </div>
      </div>

      <div class="tab-pane fade" id="device">
        <div class="card mb-3">
          <div class="card-header">Device Information</div>
          <div class="card-body">
            <pre id="deviceBox">Loading...</pre>
            <button class="btn btn-primary btn-sm" onclick="loadDevice()">Refresh</button>
          </div>
        </div>
      </div>

      <div class="tab-pane fade" id="network">
        <div class="card mb-3">
          <div class="card-header">Network Configuration</div>
          <div class="card-body">
            <pre id="networkBox">Loading...</pre>
            <button class="btn btn-primary btn-sm" onclick="loadNetwork()">Refresh</button>
          </div>
        </div>
      </div>

      <div class="tab-pane fade" id="control">
        <div class="card mb-3">
          <div class="card-header">Measurement Control</div>
          <div class="card-body">
            <div class="input-group mb-3">
              <input type="text" id="taskIdInput" class="form-control" placeholder="Enter Task ID">
              <button class="btn btn-success" onclick="startMeasurement()">Start</button>
            </div>
            <button class="btn btn-danger" onclick="stopMeasurement()">Stop Measurement</button>
          </div>
        </div>
      </div>

      <div class="tab-pane fade" id="components">
        <div class="card mb-3">
          <div class="card-header">Component Order</div>
          <div class="card-body">
            <label for="componentOrderInput">Set Component Order (comma-separated):</label>
            <div class="input-group mb-3">
              <input type="text" id="componentOrderInput" class="form-control" placeholder="e.g., CAS1,CAS2,CAS3">
              <button class="btn btn-secondary" onclick="setComponentOrder()">Apply</button>
            </div>
          </div>
        </div>
      </div>

      <div class="tab-pane fade" id="settings">
        <div class="card mb-3">
          <div class="card-header">System Settings</div>
          <div class="card-body">
            <div class="form-group mb-3">
              <label for="datetimeBox">Current Device Time:</label>
              <pre id="datetimeBox">Loading...</pre>
              <button class="btn btn-primary btn-sm" onclick="loadDateTime()">Refresh</button>
            </div>
            <div class="form-group mb-3">
              <label for="systemParamsBox">System Parameters:</label>
              <pre id="systemParamsBox">Loading...</pre>
              <button class="btn btn-primary btn-sm" onclick="loadSystemParams()">Refresh</button>
            </div>
          </div>
        </div>
      </div>	
    </div>
  </div>

  <script>
    let resultsData = {};
    let chart = null;

	function toggleTheme() {
      const html = document.documentElement;
      html.dataset.bsTheme = html.dataset.bsTheme === 'dark' ? 'light' : 'dark';
    }

    function updateConnectionStatus(ok) {
      const el = document.getElementById('connectionStatus');
      if (ok) {
        el.textContent = 'Connected';
        el.classList.remove('disconnected');
        el.classList.add('connected');
      } else {
        el.textContent = 'Disconnected';
        el.classList.remove('connected');
        el.classList.add('disconnected');
      }
    }

    async function fetchWithStatus(url) {
      try {
        const res = await fetch(url);
        updateConnectionStatus(true);
        return res;
      } catch (e) {
        updateConnectionStatus(false);
        throw e;
      }
    }

    function filterResults(keyword) {
      if (!resultsData) return;
      const filtered = JSON.stringify(resultsData, null, 2)
        .split('\n')
        .filter(line => line.toLowerCase().includes(keyword.toLowerCase()))
        .join('\n');
      document.getElementById('resultsBox').textContent = filtered;
    }

    function updateChart(data) {
      const ctx = document.getElementById('resultsChart').getContext('2d');
      if (!Array.isArray(data)) return;

      const labels = data.map((item, i) => item.timestamp || i);
      const keys = Object.keys(data[0] || {}).filter(k => typeof data[0][k] === 'number');

      const datasets = keys.map(key => ({
        label: key,
        data: data.map(d => d[key]),
        fill: false,
        borderColor: '#' + Math.floor(Math.random()*16777215).toString(16),
        tension: 0.1
      }));

      if (chart) chart.destroy();
      chart = new Chart(ctx, {
        type: 'line',
        data: {
          labels,
          datasets
        },
        options: {
          responsive: true,
          plugins: {
            legend: { position: 'top' },
            title: { display: true, text: 'Results Graph' },
            zoom: {
              pan: { enabled: true, mode: 'x' },
              zoom: { wheel: { enabled: true }, pinch: { enabled: true }, mode: 'x' }
            }
          }
        }
      });
    }

	function resetZoom() {
      if (chart) chart.resetZoom();
    }

    function saveChartImage() {
      const a = document.createElement('a');
      a.href = chart.toBase64Image();
      a.download = 'results_chart.png';
      a.click();
    }

    function downloadResults(format) {
      let content = '';
      if (format === 'json') {
        content = JSON.stringify(resultsData, null, 2);
        downloadFile(content, 'results.json', 'application/json');
      } else if (format === 'csv') {
        const rows = [];
        if (Array.isArray(resultsData)) {
          const keys = Object.keys(resultsData[0] || {});
          rows.push(keys.join(','));
          resultsData.forEach(row => {
            rows.push(keys.map(k => JSON.stringify(row[k] || '')).join(','));
          });
        } else {
          const keys = Object.keys(resultsData);
          rows.push('Key,Value');
          keys.forEach(k => rows.push(`${k},${JSON.stringify(resultsData[k])}`));
        }
        content = rows.join('\n');
        downloadFile(content, 'results.csv', 'text/csv');
      }
    }

    function downloadFile(content, filename, type) {
      const blob = new Blob([content], { type });
      const a = document.createElement('a');
      a.href = URL.createObjectURL(blob);
      a.download = filename;
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
    }

    function loadResults() {
      fetchWithStatus('/results')
        .then(r => r.json())
        .then(data => {
          resultsData = data;
          document.getElementById('resultsBox').textContent = JSON.stringify(data, null, 2);
          updateChart(data);
        });
    }

    function loadStatus() {
      fetchWithStatus('/status').then(r => r.json()).then(data => {
        document.getElementById('statusBox').textContent = JSON.stringify(data, null, 2);
      });
    }

    function loadErrors() {
      fetchWithStatus('/errors').then(r => r.json()).then(data => {
        document.getElementById('errorsBox').textContent = JSON.stringify(data, null, 2);
      });
    }

    function loadDevice() {
      fetchWithStatus('/device').then(r => r.json()).then(data => {
        document.getElementById('deviceBox').textContent = JSON.stringify(data, null, 2);
      });
    }

    function loadNetwork() {
      fetchWithStatus('/network').then(r => r.json()).then(data => {
        document.getElementById('networkBox').textContent = JSON.stringify(data, null, 2);
      });
    }

    function loadDateTime() {
      fetchWithStatus('/datetime').then(r => r.json()).then(data => {
        document.getElementById('datetimeBox').textContent = JSON.stringify(data, null, 2);
      });
    }

    function loadSystemParams() {
      fetchWithStatus('/system').then(r => r.json()).then(data => {
        document.getElementById('systemParamsBox').textContent = JSON.stringify(data, null, 2);
      });
    }

    function setComponentOrder() {
      const val = document.getElementById('componentOrderInput').value.trim();
      if (!val) return alert("Please enter a component order");
      fetchWithStatus(`/componentOrder?cas=${encodeURIComponent(val)}`, { method: 'POST' })
        .then(r => r.json()).then(data => alert(JSON.stringify(data, null, 2)));
    }

    function startMeasurement() {
      const taskIdInput = document.getElementById('taskIdInput');
      const taskId = taskIdInput.value.trim();
      if (!taskId) return alert("Please enter a task ID");
      localStorage.setItem("lastTaskId", taskId);
      fetchWithStatus(`/startMeasurement?taskId=${taskId}`, { method: 'POST' })
        .then(r => r.json()).then(data => alert(JSON.stringify(data, null, 2)));
    }

    function stopMeasurement() {
      fetchWithStatus('/stopMeasurement', { method: 'POST' })
        .then(r => r.json()).then(data => alert(JSON.stringify(data, null, 2)));
    }

    window.addEventListener('load', () => {
      const lastTaskId = localStorage.getItem("lastTaskId");
      if (lastTaskId) document.getElementById('taskIdInput').value = lastTaskId;
      loadStatus();
      loadResults();
      loadErrors();
      loadDevice();
      loadNetwork();
      loadDateTime();
      loadSystemParams();
      setInterval(loadStatus, 10000);
    });
  </script>
  <script src="https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/js/bootstrap.bundle.min.js"></script>
</body>
</html>
)rawliteral";
