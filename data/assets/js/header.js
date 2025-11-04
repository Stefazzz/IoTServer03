// header.js
// Actualiza la hora y la fecha en español y escribe en los elementos con ids timeNow y dateNow
(function () {
  const timeEl = document.getElementById('timeNow');
  const dateEl = document.getElementById('dateNow');

  // Formatea la fecha en español como: Lunes, 11 de Noviembre del 2025
  function formatDateSpanish(d) {
    try {
      const opts = { weekday: 'long', day: 'numeric', month: 'long', year: 'numeric' };
      // Usamos Intl para obtener partes y luego ajustamos el texto 'del'
      const parts = new Intl.DateTimeFormat('es-ES', opts).formatToParts(d);
      // parts contiene objetos con type: weekday, day, month, year, literal, etc.
      const wk = parts.find(p => p.type === 'weekday')?.value || '';
      const day = parts.find(p => p.type === 'day')?.value || '';
      const month = parts.find(p => p.type === 'month')?.value || '';
      const year = parts.find(p => p.type === 'year')?.value || '';
      // Construimos: "Lunes, 11 de Noviembre del 2025"
      // Nota: capitalizamos la primera letra del weekday y month
      const cap = s => s.charAt(0).toUpperCase() + s.slice(1);
      return `${cap(wk)}, ${day} de ${cap(month)} del ${year}`;
    } catch (e) {
      // Fallback sencillo
      return d.toLocaleDateString('es-ES');
    }
  }

  // Formatea la hora en 12h con AM/PM, e.g. 6:22:32 PM
  function formatTime12(d) {
    try {
      const opts = { hour: 'numeric', minute: '2-digit', second: '2-digit', hour12: true };
      return new Intl.DateTimeFormat('es-ES', opts).format(d);
    } catch (e) {
      // Fallback
      let h = d.getHours();
      const ampm = h >= 12 ? 'PM' : 'AM';
      h = h % 12;
      if (h === 0) h = 12;
      const mm = String(d.getMinutes()).padStart(2, '0');
      const ss = String(d.getSeconds()).padStart(2, '0');
      return `${h}:${mm}:${ss} ${ampm}`;
    }
  }

  function tick() {
    const now = new Date();
    if (timeEl) timeEl.textContent = formatTime12(now);
    if (dateEl) dateEl.textContent = formatDateSpanish(now);
  }

  // Start immediately and update each second
  window.addEventListener('load', () => {
    tick();
    setInterval(tick, 1000);
  });
})();
