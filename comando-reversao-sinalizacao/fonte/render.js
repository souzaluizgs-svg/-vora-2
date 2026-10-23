// node render.js <svg> <out.png|out.jpg|out.pdf> [larguraPx]
// PDF: vetorial no tamanho real (lê width/height em mm do SVG). PNG/JPG: largura em px.
const { chromium } = require('/opt/node22/lib/node_modules/playwright');
const fs = require('fs');

(async () => {
  const [svgPath, out, wpx = '1500'] = process.argv.slice(2);
  let svg = fs.readFileSync(svgPath, 'utf8');
  const vb = svg.match(/viewBox="0 0 ([\d.]+) ([\d.]+)"/);
  const [vw, vh] = [parseFloat(vb[1]), parseFloat(vb[2])];
  const browser = await chromium.launch();
  if (out.endsWith('.pdf')) {
    const mw = svg.match(/width="([\d.]+)mm"/)[1], mh = svg.match(/height="([\d.]+)mm"/)[1];
    const page = await browser.newPage();
    await page.setContent(`<html><head><style>@page{size:${mw}mm ${mh}mm;margin:0}html,body{margin:0;padding:0}svg{display:block}</style></head><body>${svg}</body></html>`);
    await page.pdf({ path: out, width: `${mw}mm`, height: `${mh}mm`, printBackground: true, pageRanges: '1' });
  } else {
    const w = parseInt(wpx, 10), hgt = Math.round(w * vh / vw);
    svg = svg.replace(/width="[^"]+" height="[^"]+" viewBox/, `width="${w}" height="${hgt}" viewBox`);
    const page = await browser.newPage({ viewport: { width: w, height: hgt } });
    await page.setContent(`<html><body style="margin:0">${svg}</body></html>`);
    const jpg = out.endsWith('.jpg');
    await page.screenshot({ path: out, type: jpg ? 'jpeg' : 'png', quality: jpg ? 92 : undefined });
  }
  await browser.close();
})();
