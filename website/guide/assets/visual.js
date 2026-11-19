(() => {
  const dialog = document.querySelector('.guide-lightbox');
  if (!dialog) return;
  const body = dialog.querySelector('.lightbox-body');
  let opener;
  document.querySelectorAll('.shot-button').forEach(button => {
    button.addEventListener('click', () => {
      opener = button;
      body.replaceChildren();
      const figure = button.closest('figure');
      const svg = button.querySelector('svg').cloneNode(true);
      // Keep duplicated SVG marker references scoped to this copy.
      svg.querySelectorAll('[id]').forEach(el => {
        const old = el.id, next = old + '-zoom';
        svg.querySelectorAll('[marker-end]').forEach(line => {
          if (line.getAttribute('marker-end') === `url(#${old})`) line.setAttribute('marker-end', `url(#${next})`);
        });
        el.id = next;
      });
      body.append(svg, figure.querySelector('.callout-legend').cloneNode(true), figure.querySelector('figcaption').cloneNode(true));
      dialog.showModal();
    });
  });
  dialog.querySelector('.lightbox-close').addEventListener('click', () => dialog.close());
  dialog.addEventListener('click', event => { if(event.target === dialog) dialog.close(); });
  dialog.addEventListener('close', () => { body.replaceChildren(); opener?.focus(); });
})();
