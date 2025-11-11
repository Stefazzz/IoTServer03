async function fetchToOut(path) {
  const outEl = document.getElementById('uplinkData');
  const lastEl = document.getElementById('last');
  try {
    const res = await fetch(path);
    if (!res.ok) {
      outEl.textContent = '❌ Error: ' + res.status;
      return;
    }
    const json = await res.json();

// Mostrar SSID actual si existe
const ssidEl = document.getElementById('ssidValue');
if (json.data.wifi && json.data.wifi.stations && json.data.wifi.stations.length > 0) {
  ssidEl.textContent = json.data.wifi.stations[0].ssid;
} else {
  ssidEl.textContent = "No conectado";
}

/*
// Mostrar IPV4 actual si existe
const ipv4 = document.getElementById('ipv4Value');
if (json.wifi.defaults) {
  ipv4.textContent = json.wifi.defaults.ipv4;
} else {
  ipv4.textContent ="No IPV4";
}*/


   let display = '';
    if (json.wifi && json.wifi.wifi_mode) {
      const wifiInfo = json.wifi.wifi_mode;
      display += `=== Estado de Conexión ===\n`;
      display += `🌐 Red WiFi: ${wifi.ssid}\n`;
      display += `📡 Dirección IP: ${wifiInfo.ip}\n`;
      if (wifiInfo.rssi) {
        const signal = wifiInfo.rssi;
        let signalIcon = '📶';
        if (signal < -70) signalIcon = '▂▁';
        else if (signal < -60) signalIcon = '▃▂▁';
        else if (signal < -50) signalIcon = '▅▃▂▁';
        else signalIcon = '▇▅▃▂▁';
        display += `${signalIcon} Señal: ${signal} dBm\n`;
      }
      if (json.wifi.status === "ap_mode") {
        display += "⚠️ Modo Punto de Acceso Activo\n";
      }
      display += '\n';
    }

    if (json.actuators) {
      display += "=== Actuadores ===\n";

      if (json.actuators.digital) {
        display += "➡️ Digitales:\n";
        json.actuators.digital.forEach(d => {
          display += `- ${d.name}: ${d.state ? "ON" : "OFF"}\n`;
        });
      }

      if (json.actuators.analog) {
        const a = json.actuators.analog;
        display += `➡️ Analógico (${a.name}): ${a.value_percent}%\n`;
      }
    }

    // Procesar datos del último uplink si existe
    if (json.last_uplink_json) {
      display += '=== Último Mensaje ===\n';
      display += JSON.stringify(json.last_uplink_json, null, 2);
    } else if (json.last_uplink_raw) {
      display += '=== Último Mensaje (Raw) ===\n';
      display += json.last_uplink_raw;
    }

    // Si no hay datos específicos, mostrar todo el JSON
    if (!display && json) {
      display = JSON.stringify(json, null, 2);
    }

    outEl.textContent = display;

    // Actualizar timestamp
    const now = new Date();
    if (lastEl) {
      lastEl.textContent = '🕒 Última actualización: ' + now.toLocaleString();
    }
  } catch (e) {
    outEl.textContent = '❌ Error en la petición: ' + e.message;
    console.error('Error fetching data:', e);
  }
}

// Al abrir la página, cargar los settings actuales
window.addEventListener('load', () => {
  fetchToOut('/api/settings');
  // Actualizar cada 10 segundos (10000 ms)
  setInterval(() => fetchToOut('/api/settings'), 10000);

  // Configurar botón de actualización si existe
  const refreshBtn = document.getElementById('btn-refresh');
  if (refreshBtn) {
    refreshBtn.addEventListener('click', () => fetchToOut('/api/settings'));
  }
});