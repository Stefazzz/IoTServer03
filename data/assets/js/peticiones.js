async function fetchToOut(path) {
  // Inicializar buffers de series si no existen
  if (!window._series) {
    window._series = { temp: [], hum: [], dist: [] };
    window._maxPoints = 120; // guarda ~10 min si refrescas cada 5s
  }

  // Función simple para dibujar líneas en canvas
  function drawLine(canvasId, data, options = {}) {
    const canvas = document.getElementById(canvasId);
    if (!canvas || !canvas.getContext) return;
    const ctx = canvas.getContext('2d');
    const w = canvas.width, h = canvas.height;
    ctx.clearRect(0, 0, w, h);
    if (!data || data.length === 0) return;
    const pad = 8;
    const xs = data.map((_, i) => i);
    const ymin = Math.min(...data);
    const ymax = Math.max(...data);
    const xMax = Math.max(1, data.length - 1);
    const yLo = (options.min !== undefined) ? Math.min(ymin, options.min) : ymin;
    const yHi = (options.max !== undefined) ? Math.max(ymax, options.max) : ymax;
    const ySpan = (yHi === yLo) ? 1 : (yHi - yLo);

    // Rejilla ligera
    ctx.strokeStyle = '#eee';
    ctx.lineWidth = 1;
    for (let i = 0; i <= 4; i++) {
      const y = pad + (h - 2 * pad) * (i / 4);
      ctx.beginPath(); ctx.moveTo(pad, y); ctx.lineTo(w - pad, y); ctx.stroke();
    }

    // Curva
    ctx.strokeStyle = options.color || '#1e88e5';
    ctx.lineWidth = 2;
    ctx.beginPath();
    data.forEach((v, i) => {
      const x = pad + (w - 2 * pad) * (i / xMax);
      const y = pad + (h - 2 * pad) * (1 - (v - yLo) / ySpan);
      if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
    });
    ctx.stroke();
  }
  const temp = document.getElementById('tempValue');

  const epsId = document.getElementById('espId');
  const epsName = document.getElementById('espName');
  const epsUser = document.getElementById('espUser');
  const epsPass = document.getElementById('espPass');

  const ssidEl = document.getElementById('ssidValue');
  const ipv4El = document.getElementById('ipv4Value');
  const subnet = document.getElementById('subnetValue');

  const mqttState = document.getElementById('mqttState');
  const mqttServer = document.getElementById('mqttServe');
  const mqttPort = document.getElementById('mqttPort');
  const mqttTopic = document.getElementById('mqttTopic');


  const digitalesList = document.getElementById('digitalesList');
  const analogosList = document.getElementById('analogosList');
  const lastEl = document.getElementById('last');

  const lastTemp = document.getElementById('lastTemp');
  const lastHum = document.getElementById('lastHum');
  const lastDistance = document.getElementById('lastDistance');

  const versionEl = document.getElementById('version');

  try {
    const res = await fetch(path);
    if (!res.ok) throw new Error('Error HTTP ' + res.status);
    const json = await res.json();

    console.log(json);

    if (json.sensors && json.sensors.dht11 && json.sensors.dht11.temperature !== undefined) {
      temp.textContent = `${json.sensors.dht11.temperature}°`;
    } else {
      temp.textContent = `0°`;
    }

    // Mostrar SSID actual si existe
    //---ESP Id ---
    if (json.data.device_id) {
      epsId.textContent = json.data.device_id;
    } else {
      epsId.textContent = "Desconocido";
    }
    //---ESP Name ---
    if (json.data.device_name) {
      epsName.textContent = json.data.device_name;
    } else {
      epsName.textContent = "Desconocido";
    }
    //---ESP User ---
    if (json.data.device_user) {
      epsUser.textContent = json.data.device_user;
    } else {
      epsUser.textContent = "Desconocido";
    }
    //---ESP Pass ---
    if (json.data.device_password) {
      epsPass.textContent = json.data.device_password;
    } else {
      epsPass.textContent = "Desconocido";
    }

    // --- WIFI ---
    if (json.data.wifi && json.data.wifi.stations && json.data.wifi.stations.length > 0) {
      ssidEl.textContent = json.data.wifi.stations[0].ssid || "Desconocido";
    } else {
      ssidEl.textContent = "No conectado";
    }
    // --- IPV4 ---
    if (json.data.wifi.defaults && json.data.wifi.defaults.ipv4) {
      ipv4El.textContent = json.data.wifi.defaults.ipv4;
    } else {
      ipv4El.textContent = "Sin IP";
    }
    // --- SUBNET ---
    if (json.data.wifi && json.data.wifi.defaults && json.data.wifi.defaults.subnet) {
      subnet.textContent = json.data.wifi.defaults.subnet;
    } else {
      subnet.textContent = "Sin máscara";
    }

    // --- MQTT Status---
    if (json.data.mqtt) {
      const status = json.data.mqtt.mqtt_enable ? "✅ Activo" : "❌ Inactivo";
      mqttState.textContent = status;
    } else {
      mqttState.textContent = "No hay datos MQTT";
    }
    // --- MQTT Server---
    if (json.data.mqtt) {
      const mqttServe = json.data.mqtt.mqtt_server ? json.data.mqtt.mqtt_server : "Desconocido";
      mqttServer.textContent = `${mqttServe}`;
    } else {
      mqttServer.textContent = "No hay datos MQTT";
    }
    // --- MQTT Port---
    if (json.data.mqtt) {
      const mqttPrt = json.data.mqtt.mqtt_port ? json.data.mqtt.mqtt_port : "Desconocido";
      mqttPort.textContent = `${mqttPrt}`;
    } else {
      mqttPort.textContent = "No hay datos MQTT";
    }
    // --- MQTT Topic---
    if (json.data.mqtt) {
      const mqttTpc = json.data.mqtt.mqtt_willTopic ? json.data.mqtt.mqtt_willTopic : "Desconocido";
      mqttTopic.textContent = `${mqttTpc}`;
    } else {
      mqttTopic.textContent = "No hay datos MQTT";
    }

    // --- ACTUADORES ---
    if (json.data.actuators) {
      // Digitales - crear un elemento por cada uno
      if (json.data.actuators.digital && json.data.actuators.digital.length > 0) {
        digitalesList.innerHTML = "";
        json.data.actuators.digital.forEach((d) => {
          const row = document.createElement('div');
          row.className = 'relay-row';


          const label = document.createElement('span');
          label.className = 'relay-label';
          label.textContent = d.name.replace('_', ' ').toUpperCase();

          // Toggle button
          const btn = document.createElement('button');
          btn.className = 'btn-toggle ' + (d.state ? 'on' : 'off');
          btn.textContent = d.state ? 'ON' : 'OFF';
          btn.setAttribute('aria-pressed', d.state ? 'true' : 'false');
          btn.addEventListener('click', async () => {
            btn.disabled = true;
            try {
              const resp = await fetch('/api/relay', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ name: d.name, toggle: true })
              });
              if (!resp.ok) throw new Error('HTTP ' + resp.status);
              // refrescar estado después del cambio
              await fetchToOut('/api/settings');
            } catch (e) {
              console.error('Error toggling relay', e);
            } finally {
              btn.disabled = false;
            }
          });

          row.appendChild(label);
          row.appendChild(btn);
          digitalesList.appendChild(row);
        });
      } else {
        digitalesList.textContent = "Sin datos digitales";
      }

      // Modos piscina 
      if (json.data.actuators.piscina_modes && json.data.actuators.active_pool_mode !== undefined) {
        analogosList.innerHTML = "";
        const activeMode = json.data.actuators.active_pool_mode;
        
        // Crear botón para cada modo (excepto "none" que es OFF)
        Object.keys(json.data.actuators.piscina_modes).forEach(modeName => {
          if (modeName === 'none') return; // Saltar el modo "none" (OFF)
          
          const row = document.createElement('div');
          row.className = 'mode-row';
          
          const btn = document.createElement('button');
          btn.className = 'btn-mode';
          btn.textContent = modeName.toUpperCase();
          
          // Marcar si es el modo activo
          if (activeMode === modeName) {
            btn.classList.add('active');
          }
          
          btn.addEventListener('click', async () => {
            // Deshabilitar todos los botones durante la petición
            const allBtns = analogosList.querySelectorAll('.btn-mode');
            allBtns.forEach(b => b.disabled = true);
            
            try {
              const resp = await fetch('/api/pool_mode', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ mode: modeName })
              });
              if (!resp.ok) throw new Error('HTTP ' + resp.status);
              await fetchToOut('/api/settings');
            } catch (e) {
              console.error('Error setting pool mode', e);
            } finally {
              allBtns.forEach(b => b.disabled = false);
            }
          });
          
          row.appendChild(btn);
          analogosList.appendChild(row);
        });
        
        // Agregar botón OFF
        const offRow = document.createElement('div');
        offRow.className = 'mode-row';
        const offBtn = document.createElement('button');
        offBtn.className = 'btn-mode btn-mode-off';
        offBtn.textContent = 'OFF';
        if (activeMode === 'none') {
          offBtn.classList.add('active');
        }
        offBtn.addEventListener('click', async () => {
          const allBtns = analogosList.querySelectorAll('.btn-mode');
          allBtns.forEach(b => b.disabled = true);
          try {
            const resp = await fetch('/api/pool_mode', {
              method: 'POST',
              headers: { 'Content-Type': 'application/json' },
              body: JSON.stringify({ mode: 'none' })
            });
            if (!resp.ok) throw new Error('HTTP ' + resp.status);
            await fetchToOut('/api/settings');
          } catch (e) {
            console.error('Error turning off pool mode', e);
          } finally {
            allBtns.forEach(b => b.disabled = false);
          }
        });
        offRow.appendChild(offBtn);
        analogosList.appendChild(offRow);
        
      } else {
        analogosList.textContent = "Sin modos piscina";
      }
    } else {
      digitalesList.textContent = "No hay datos de actuadores";
      analogosList.textContent = "No hay datos de actuadores";
    }

    // Actualizar últimos valores de sensores
    if (json.data.sensors) {
      // Temperatura
      if (json.data.sensors.dht11?.temperature !== undefined) {
        const t = Number(json.data.sensors.dht11.temperature);
        lastTemp.textContent = `${t} °C`;
        window._series.temp.push(t);
        if (window._series.temp.length > window._maxPoints) window._series.temp.shift();
        drawLine('chartTemp', window._series.temp, { color: '#e53935' });
      } else {
        lastTemp.textContent = `N/A`;
      }

      // Humedad
      if (json.data.sensors.dht11?.humidity !== undefined) {
        const h = Number(json.data.sensors.dht11.humidity);
        lastHum.textContent = `${h} %`;
        window._series.hum.push(h);
        if (window._series.hum.length > window._maxPoints) window._series.hum.shift();
        drawLine('chartHum', window._series.hum, { color: '#43a047' });
      } else {
        lastHum.textContent = `N/A`;
      }

      // Distancia (mm -> cm)
      if (json.data.sensors.vl53l0x?.distance_mm !== undefined) {
        const cm = Number(json.data.sensors.vl53l0x.distance_mm) / 10;
        lastDistance.textContent = `${cm.toFixed(1)} cm`;
        window._series.dist.push(cm);
        if (window._series.dist.length > window._maxPoints) window._series.dist.shift();
        drawLine('chartDist', window._series.dist, { color: '#1e88e5' });
      } else {
        lastDistance.textContent = `N/A`;
      }
    }

    // Actualizar indicadores de válvulas en el diagrama
    if (json.data.actuators && json.data.actuators.valves) {
      json.data.actuators.valves.forEach((valve) => {
        const indicator = document.querySelector(`.valve-indicator[data-valve="${valve.name}"]`);
        if (indicator) {
          if (valve.state === true || valve.state === 1) {
            indicator.classList.add('open');
          } else {
            indicator.classList.remove('open');
          }
        }
      });
    }

    // Actualizar timestamp
    const now = new Date();
    if (lastEl) {
      lastEl.textContent = 'Última actualización: ' + now.toLocaleString();
    }

    // Actualizar versión en el footer
    if (json.data.file_version) {
      versionEl.textContent = `Versión: ${json.data.file_version}`;
    } else {
      versionEl.textContent = `Versión desconocida`;
    }

  } catch (e) {
    console.error("Error en fetchToOut:", e);
  }
}

// Al abrir la página, cargar los settings actuales
window.addEventListener('load', () => {
  const API_URL = '/api/settings';
  fetchToOut(API_URL);
  // Auto-actualizar cada 5s
  setInterval(() => fetchToOut(API_URL), 5000);

  // Configurar botón de actualización si existe
  const refreshBtn = document.getElementById('btn-refresh');
  if (refreshBtn) {
    refreshBtn.addEventListener('click', () => fetchToOut(API_URL));
  }

});