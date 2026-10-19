// node render.js <svg> <out.png|out.pdf> [larguraPx]
const { chromium } = require('/opt/node22/lib/node_modules/playwright');
const fs = require('fs');

(async () => {
  const [svgPath, out, wpx = '1500'] = process.argv.slice(2);
  let svg = fs.readFileSync(svgPath, 'utf8');
  const browser = await chromium.launch();
  if (out.endsWith('.pdf')) {
    // PDF vetorial em tamanho real (3000 x 2500 mm)
    const page = await browser.newPage();
    await page.setContent(`<html><head><style>@page{size:3000mm 2500mm;margin:0}html,body{margin:0;padding:0}svg{display:block}</style></head><body>${svg}</body></html>`);
    await page.pdf({ path: out, width: '3000mm', height: '2500mm', printBackground: true, pageRanges: '1' });
  } else {
    const w = parseInt(wpx, 10), hgt = Math.round(w * 2500 / 3000);
    svg = svg.replace(/width="[^"]+" height="[^"]+" viewBox/, `width="${w}" height="${hgt}" viewBox`);
    const page = await browser.newPage({ viewport: { width: w, height: hgt } });
    await page.setContent(`<html><body style="margin:0">${svg}</body></html>`);
    await page.screenshot({ path: out, type: out.endsWith('.jpg') ? 'jpeg' : 'png', quality: out.endsWith('.jpg') ? 92 : undefined });
  }
  await browser.close();
})();
