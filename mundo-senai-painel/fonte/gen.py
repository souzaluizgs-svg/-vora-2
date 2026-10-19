"""Gera o painel de fotos Mundo SENAI Telêmaco Borba em SVG (unidades = mm).

Todo texto é convertido em curvas, então o arquivo não depende de fontes instaladas.
Uso: FONT_DIR=<pasta com Poppins-Black/BlackItalic/ExtraBold/Bold.ttf> python3 gen.py <pasta_saida>
Fontes (OFL): https://github.com/google/fonts/tree/main/ofl/poppins
PDF/PNG: node render.js <arquivo.svg> <saida.pdf|saida.png> [larguraPx]
"""
import sys, os, math
import uharfbuzz as hb
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen

FONT_DIR = os.environ.get("FONT_DIR", os.path.join(os.path.dirname(__file__), "..", "fonts"))
OUT = sys.argv[1] if len(sys.argv) > 1 else "."

W, H = 3000, 2500  # mm  (painel 3,00 x 2,50 m)
FLOOR = H


def h(m):
    """altura a partir do chão (em metros) -> coordenada y do SVG"""
    return FLOOR - m * 1000


# ---------------------------------------------------------------- texto -> curvas
class Font:
    def __init__(self, fname):
        path = os.path.join(FONT_DIR, fname)
        self.tt = TTFont(path)
        self.gs = self.tt.getGlyphSet()
        self.upem = self.tt["head"].unitsPerEm
        self.cap = self.tt["OS/2"].sCapHeight / self.upem
        self.order = self.tt.getGlyphOrder()
        self.hb = hb.Font(hb.Face(hb.Blob.from_file_path(path)))

    def _shape(self, text):
        buf = hb.Buffer()
        buf.add_str(text)
        buf.guess_segment_properties()
        hb.shape(self.hb, buf, {"kern": True, "liga": True})
        return buf.glyph_infos, buf.glyph_positions

    def width(self, text, size, tracking=0):
        infos, pos = self._shape(text)
        return sum(p.x_advance for p in pos) * size / self.upem + tracking * (len(infos) - 1)

    def d(self, text, size, x, y, anchor="start", tracking=0):
        s = size / self.upem
        infos, pos = self._shape(text)
        total = self.width(text, size, tracking)
        x0 = {"start": x, "middle": x - total / 2, "end": x - total}[anchor]
        pen = SVGPathPen(self.gs, ntos=lambda v: f"{v:.2f}".rstrip("0").rstrip("."))
        cx = 0.0
        for info, p in zip(infos, pos):
            tp = TransformPen(pen, (s, 0, 0, -s, x0 + cx + p.x_offset * s, y - p.y_offset * s))
            self.gs[self.order[info.codepoint]].draw(tp)
            cx += p.x_advance * s + tracking
        return pen.getCommands()


BLACK = Font("Poppins-Black.ttf")
BLACK_IT = Font("Poppins-BlackItalic.ttf")
XBOLD = Font("Poppins-ExtraBold.ttf")
BOLD = Font("Poppins-Bold.ttf")

# ---------------------------------------------------------------- paleta
NAVY = "#15214F"
SKY_T, SKY_B = "#6FBDF0", "#B9E3FB"
LIME = "#D4F046"
ROYAL = "#3448E0"
UNIF = "#23308F"
PANTS = "#3550E6"
BUILD = "#F3F6FF"
WIN = "#8FD0F7"
HILL = "#97DDB2"
HILL_TREE = "#7DD0A0"
GRASS = "#4FC285"
TREE = "#3BAF72"
TREE_D = "#2A9460"
BARK = "#EFE8D8"
RED = "#F2665E"
ROAD = "#CDD4E6"
TANK = "#C9D3F5"
SKIN1, SKIN2 = "#F3BC94", "#A8673F"
HAIR = "#5B3420"
YEL = "#FFCC33"
STRIPE = "#F4F6FA"
SW = 10  # espessura padrão do contorno (mm)


def stroke(w=SW):
    return f'stroke="{NAVY}" stroke-width="{w}" stroke-linejoin="round" stroke-linecap="round"'


def rr(x, y, w, hh, r, fill, sw=SW, extra=""):
    st = stroke(sw) if sw else 'stroke="none"'
    return f'<rect x="{x}" y="{y}" width="{w}" height="{hh}" rx="{r}" fill="{fill}" {st} {extra}/>'


def circ(cx, cy, r, fill, sw=SW, extra=""):
    st = stroke(sw) if sw else 'stroke="none"'
    return f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="{fill}" {st} {extra}/>'


def limb(d, color, w=100, sw=SW):
    """braço/perna com contorno: traço grosso navy + traço colorido por cima"""
    return (f'<path d="{d}" fill="none" stroke="{NAVY}" stroke-width="{w + 2 * sw}" stroke-linecap="round" stroke-linejoin="round"/>'
            f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{w}" stroke-linecap="round" stroke-linejoin="round"/>')


# ---------------------------------------------------------------- cenário
def sky():
    return f'''
  <linearGradient id="sky" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="{SKY_T}"/><stop offset="1" stop-color="{SKY_B}"/>
  </linearGradient>
  <radialGradient id="halo" cx="0.5" cy="0.5" r="0.5">
    <stop offset="0" stop-color="#fff" stop-opacity=".45"/><stop offset="1" stop-color="#fff" stop-opacity="0"/>
  </radialGradient>''', f'''
<rect width="{W}" height="{H}" fill="url(#sky)"/>
<ellipse cx="{W/2}" cy="420" rx="1650" ry="420" fill="url(#halo)"/>'''


def cloud(x, y, s=1.0, op=1):
    parts = [(0, 0, 90), (95, -40, 110), (210, -10, 95), (300, 20, 70), (150, 40, 90), (40, 40, 70)]
    c = "".join(f'<circle cx="{x + dx * s:.0f}" cy="{y + dy * s:.0f}" r="{r * s:.0f}"/>' for dx, dy, r in parts)
    return f'<g fill="#fff" opacity="{op}">{c}</g>'


def hills():
    d = (f"M0 {h(0.78)} C 300 {h(0.92)} 600 {h(0.98)} 950 {h(0.86)} "
         f"S 1500 {h(0.70)} 1900 {h(0.84)} S 2600 {h(1.0)} {W} {h(0.82)} L {W} {h(0.2)} L 0 {h(0.2)} Z")
    out = f'<path d="{d}" fill="{HILL}"/>'
    # fileiras de eucalipto plantado no horizonte (floresta plantada típica da região)
    trees = []
    for i in range(0, W + 1, 30):
        x = i
        # aproxima a linha do morro (amostragem simples da curva acima)
        t = x / W
        base = 0.78 + 0.16 * math.sin(math.pi * t * 1.1) + 0.05 * math.sin(6 * math.pi * t)
        y = h(base) + 18
        trees.append(f'<ellipse cx="{x}" cy="{y:.0f}" rx="9" ry="22"/>')
    out += f'<g fill="{HILL_TREE}">{"".join(trees)}</g>'
    return out


def ground():
    g_top = h(0.36)
    road_t, road_b = h(0.27), h(0.11)
    d = f"M0 {g_top} C 700 {g_top - 50} 1300 {g_top + 20} 1900 {g_top - 30} S 2700 {g_top - 10} {W} {g_top - 40} L {W} {road_t} L 0 {road_t} Z"
    out = f'<path d="{d}" fill="{GRASS}"/>'
    out += f'<rect x="0" y="{road_t}" width="{W}" height="{road_b - road_t}" fill="{ROAD}"/>'
    out += f'<line x1="0" y1="{road_t}" x2="{W}" y2="{road_t}" {stroke(8)}/>'
    mid = (road_t + road_b) / 2
    out += "".join(f'<rect x="{x}" y="{mid - 9}" width="130" height="18" rx="9" fill="#fff"/>' for x in range(40, W, 260))
    return out


def eucalyptus(x, top, base, w=1.0, dark_side=1):
    trunk = f'<rect x="{x - 14 * w}" y="{top + 200 * w}" width="{28 * w}" height="{base - top - 200 * w}" rx="10" fill="{BARK}" {stroke(7)}/>'
    cy = top + 290 * w
    crown = (f'<ellipse cx="{x}" cy="{cy}" rx="{120 * w}" ry="{290 * w}" fill="{TREE}" {stroke(8)}/>'
             f'<ellipse cx="{x + dark_side * 45 * w}" cy="{cy + 40 * w}" rx="{60 * w}" ry="{215 * w}" fill="{TREE_D}" opacity=".55"/>')
    return trunk + crown


def factory():
    x0, x1 = 760, 2240
    top, base = h(0.80), h(0.27)
    out = []
    # chaminés curtas nas pontas (nada alto atrás/ao lado das cabeças de quem posa)
    for sx in (800, 2120):
        st_top = h(1.08)
        out.append(rr(sx, st_top, 80, top - st_top + 20, 8, "#fff"))
        out.append(f'<rect x="{sx}" y="{st_top + 35}" width="80" height="40" fill="{RED}" {stroke(8)}/>')
    # vapor saindo para fora
    for sx, dirx in ((840, -1), (2160, 1)):
        y = h(1.08)
        for dx, dy, r in [(0, -50, 48), (dirx * 55, -120, 62), (dirx * 150, -165, 66)]:
            out.append(f'<circle cx="{sx + dx}" cy="{y + dy}" r="{r}" fill="#fff"/>')
    # telhado em shed
    teeth = 7
    tw = (x1 - x0) / teeth
    roof_t = top - 150
    for i in range(teeth):
        a = x0 + i * tw
        out.append(f'<path d="M{a:.0f} {top} L{a + tw:.0f} {roof_t} L{a + tw:.0f} {top} Z" fill="{ROYAL}" {stroke(SW)}/>')
    out.append(rr(x0, top, x1 - x0, base - top, 6, BUILD))
    # janelas
    n = 8
    gap = (x1 - x0 - n * 120) / (n + 1)
    for i in range(n):
        wx = x0 + gap + i * (120 + gap)
        out.append(rr(round(wx), top + 70, 120, 95, 8, WIN, 8))
    # portão
    out.append(rr(1390, h(0.62), 220, h(0.27) - h(0.62), 4, NAVY, 0))
    for k in range(3):
        out.append(f'<rect x="1410" y="{h(0.56) + k * 85}" width="180" height="22" rx="11" fill="{LIME}"/>')
    # tanque
    out.append(rr(2250, h(0.78), 140, h(0.27) - h(0.78), 50, TANK))
    out.append(f'<line x1="2250" y1="{h(0.60)}" x2="2390" y2="{h(0.60)}" {stroke(8)}/>')
    out.append(f'<rect x="2238" y="{h(0.46)}" width="22" height="30" fill="{RED}" {stroke(6)}/>')
    return "".join(out)


def paper_rolls():
    out = []
    r = 78
    for cx, cy in ((690, h(0.27) - r), (850, h(0.27) - r), (770, h(0.27) - 3 * r + 18)):
        out.append(circ(cx, cy, r, "#fff"))
        out.append(circ(cx, cy, r * 0.55, "#DDE6FF", 8))
        out.append(circ(cx, cy, r * 0.18, NAVY, 0))
    return "".join(out)


# ---------------------------------------------------------------- personagens
def worker_woman():
    """origem = centro dos pés; y negativo para cima. ~1,62 m"""
    o = []
    o.append(f'<ellipse cx="0" cy="0" rx="280" ry="32" fill="{NAVY}" opacity=".15"/>')
    o.append(rr(-150, -62, 140, 62, 28, NAVY, 0))
    o.append(rr(10, -62, 140, 62, 28, NAVY, 0))
    o.append(rr(-118, -720, 108, 680, 34, PANTS))
    o.append(rr(10, -720, 108, 680, 34, PANTS))
    # cabelo atrás
    o.append(rr(-168, -1480, 336, 330, 130, HAIR))
    # braço do tablet (atrás do tronco)
    o.append(limb("M-150 -1150 Q -260 -1000 -150 -930", ROYAL))
    # tronco + colete
    o.append('<clipPath id="torsoW"><rect x="-190" y="-1235" width="380" height="575" rx="75"/></clipPath>')
    o.append(rr(-190, -1235, 380, 575, 75, LIME))
    o.append(f'<g clip-path="url(#torsoW)">'
             f'<rect x="-200" y="-955" width="400" height="48" fill="{STRIPE}" {stroke(6)}/>'
             f'<rect x="-200" y="-860" width="400" height="48" fill="{STRIPE}" {stroke(6)}/>'
             f'<path d="M-80 -1240 L80 -1240 L0 -1110 Z" fill="{ROYAL}" {stroke(8)}/>'
             f'</g>')
    o.append(f'<line x1="0" y1="-1110" x2="0" y2="-680" {stroke(8)}/>')
    o.append(rr(-190, -1235, 380, 575, 75, "none"))
    # tablet
    o.append(f'<g transform="rotate(-10 -150 -960)">'
             f'{rr(-290, -1060, 250, 180, 20, NAVY, 0)}'
             f'{rr(-272, -1043, 214, 146, 10, WIN, 0)}'
             f'<rect x="-250" y="-950" width="34" height="38" fill="{LIME}"/>'
             f'<rect x="-205" y="-985" width="34" height="73" fill="{LIME}"/>'
             f'<rect x="-160" y="-1020" width="34" height="108" fill="{LIME}"/>'
             f'<rect x="-115" y="-1000" width="34" height="88" fill="#fff"/></g>')
    o.append(circ(-150, -930, 52, SKIN1))
    # pescoço e cabeça
    o.append(rr(-44, -1295, 88, 90, 10, SKIN1))
    o.append(circ(0, -1395, 152, SKIN1))
    o.append(f'<circle cx="-52" cy="-1380" r="16" fill="{NAVY}"/><circle cx="52" cy="-1380" r="16" fill="{NAVY}"/>')
    o.append(f'<path d="M-48 -1330 Q 0 -1288 48 -1330" fill="none" {stroke(10)}/>')
    o.append(f'<ellipse cx="-95" cy="-1338" rx="30" ry="22" fill="#F48FA0" opacity=".6"/>'
             f'<ellipse cx="95" cy="-1338" rx="30" ry="22" fill="#F48FA0" opacity=".6"/>')
    # óculos de proteção na testa
    o.append(rr(-112, -1478, 224, 62, 28, "#BFE6FF", 9))
    o.append(f'<line x1="0" y1="-1474" x2="0" y2="-1420" {stroke(7)}/>')
    # capacete
    o.append(f'<path d="M-175 -1490 A 175 175 0 0 1 175 -1490 Z" fill="{YEL}" {stroke()}/>')
    o.append(f'<path d="M-30 -1660 Q 0 -1668 30 -1660 L 30 -1492 L -30 -1492 Z" fill="#FFE27A"/>')
    o.append(f'<path d="M-175 -1490 A 175 175 0 0 1 175 -1490" fill="none" {stroke()}/>')
    o.append(rr(-205, -1512, 410, 44, 22, YEL))
    # braço acenando (para o centro)
    o.append(limb("M150 -1160 Q 300 -1200 345 -1420", ROYAL))
    o.append(circ(352, -1465, 56, SKIN1))
    return "".join(o)


def worker_man():
    """origem = centro dos pés; ~1,66 m"""
    o = []
    o.append(f'<ellipse cx="0" cy="0" rx="290" ry="32" fill="{NAVY}" opacity=".15"/>')
    o.append(rr(-160, -64, 150, 64, 28, "#6B3E26", 0))
    o.append(rr(10, -64, 150, 64, 28, "#6B3E26", 0))
    o.append(rr(-125, -740, 115, 700, 34, UNIF))
    o.append(rr(10, -740, 115, 700, 34, UNIF))
    # braço na cintura (atrás do tronco)
    o.append(limb("M160 -1170 Q 290 -1010 205 -880", UNIF))
    o.append('<clipPath id="torsoM"><rect x="-200" y="-1265" width="400" height="600" rx="80"/></clipPath>')
    o.append(rr(-200, -1265, 400, 600, 80, UNIF))
    o.append(f'<g clip-path="url(#torsoM)">'
             f'<path d="M-200 -1265 L-70 -1265 L-70 -650 L-200 -650 Z" fill="{LIME}" {stroke(8)}/>'
             f'<path d="M200 -1265 L70 -1265 L70 -650 L200 -650 Z" fill="{LIME}" {stroke(8)}/>'
             f'<rect x="-210" y="-975" width="420" height="50" fill="{STRIPE}" {stroke(6)}/>'
             f'<rect x="-210" y="-880" width="420" height="50" fill="{STRIPE}" {stroke(6)}/>'
             f'</g>')
    o.append(rr(-200, -1265, 400, 600, 80, "none"))
    o.append(circ(205, -880, 54, SKIN2))
    # pescoço e cabeça
    o.append(rr(-46, -1325, 92, 90, 10, SKIN2))
    o.append(circ(0, -1425, 155, SKIN2))
    o.append(f'<circle cx="-54" cy="-1405" r="16" fill="{NAVY}"/><circle cx="54" cy="-1405" r="16" fill="{NAVY}"/>')
    o.append(f'<path d="M-50 -1355 Q 0 -1312 50 -1355" fill="none" {stroke(10)}/>')
    o.append(f'<ellipse cx="-100" cy="-1362" rx="30" ry="22" fill="#E07A6A" opacity=".45"/>'
             f'<ellipse cx="100" cy="-1362" rx="30" ry="22" fill="#E07A6A" opacity=".45"/>')
    o.append(rr(-116, -1508, 232, 64, 28, "#BFE6FF", 9))
    o.append(f'<line x1="0" y1="-1504" x2="0" y2="-1448" {stroke(7)}/>')
    # capacete branco com faixa azul
    o.append(f'<path d="M-180 -1520 A 180 180 0 0 1 180 -1520 Z" fill="#fff" {stroke()}/>')
    o.append(f'<path d="M-32 -1696 Q 0 -1702 32 -1696 L 32 -1522 L -32 -1522 Z" fill="{ROYAL}"/>')
    o.append(f'<path d="M-180 -1520 A 180 180 0 0 1 180 -1520" fill="none" {stroke()}/>')
    o.append(rr(-212, -1544, 424, 46, 23, "#fff"))
    # braço erguido com chave (para o centro)
    o.append(limb("M-160 -1190 Q -305 -1220 -340 -1345", UNIF))
    o.append(f'<g transform="rotate(-4 -352 -1390)">'
             f'{rr(-372, -1610, 40, 230, 18, "#D9DEE8")}'
             f'<path d="M-352 -1660 m -66 0 a 66 66 0 1 0 132 0 a 66 66 0 1 0 -132 0 Z M -378 -1730 L -378 -1665 L -326 -1665 L -326 -1730 Z" '
             f'fill="#D9DEE8" fill-rule="evenodd" {stroke()}/></g>')
    o.append(circ(-350, -1390, 58, SKIN2))
    return "".join(o)


# ---------------------------------------------------------------- tipografia / marca
def title():
    """MUNDO SENAI em uma linha, acima da altura das cabeças."""
    size = 380
    trk = 6
    w1 = BLACK.width("MUNDO ", size, trk)
    w2 = BLACK.width("SENAI", size, trk)
    total = w1 + w2
    maxw = 2560
    if total > maxw:
        k = maxw / total
        size *= k
        w1 *= k
        w2 *= k
        total = maxw
    base = 300 + BLACK.cap * size
    x0 = W / 2 - total / 2
    d1 = BLACK.d("MUNDO ", size, x0, base, tracking=trk)
    d2 = BLACK.d("SENAI", size, x0 + w1, base, tracking=trk)
    sh = 16
    out = (f'<g transform="translate({sh} {sh + 4})" fill="{NAVY}" stroke="{NAVY}" stroke-width="34" stroke-linejoin="round">'
           f'<path d="{d1}"/><path d="{d2}"/></g>'
           f'<g stroke="{NAVY}" stroke-width="34" stroke-linejoin="round" paint-order="stroke">'
           f'<path d="{d1}" fill="#fff"/><path d="{d2}" fill="{LIME}"/></g>')
    return out, base


def pill(cx, cy, text, font, size, fill=LIME, color=NAVY, padx=60, hh=120, icon=None):
    tw = font.width(text, size, 2)
    iw = 0 if icon is None else size * 0.95 + 24
    w = tw + iw + 2 * padx
    x = cx - w / 2
    base = cy + font.cap * size / 2
    o = rr(round(x), cy - hh / 2, round(w), hh, hh / 2, fill, 9)
    tx = x + padx + iw
    if icon == "ig":
        s = size * 0.95
        ix, iy = x + padx, cy - s / 2
        o += (f'<rect x="{ix:.0f}" y="{iy:.0f}" width="{s:.0f}" height="{s:.0f}" rx="{s * .28:.0f}" fill="none" stroke="{color}" stroke-width="{s * .1:.0f}"/>'
              f'<circle cx="{ix + s / 2:.0f}" cy="{iy + s / 2:.0f}" r="{s * .22:.0f}" fill="none" stroke="{color}" stroke-width="{s * .1:.0f}"/>'
              f'<circle cx="{ix + s * .76:.0f}" cy="{iy + s * .24:.0f}" r="{s * .06:.0f}" fill="{color}"/>')
    o += f'<path d="{font.d(text, size, tx, base, tracking=2)}" fill="{color}"/>'
    return o


def logo_placeholder(cx, cy):
    """Marca provisória — SUBSTITUIR pelo arquivo vetorial oficial do SENAI (manual de marca)."""
    w, hh = 560, 200
    x, y = cx - w / 2, cy - hh / 2
    size = 175
    base = cy + BLACK_IT.cap * size / 2
    d = BLACK_IT.d("SENAI", size, cx, base, anchor="middle", tracking=4)
    lines = "".join(f'<rect x="{x + 40}" y="{cy - 60 + k * 40}" width="{w - 80}" height="9" fill="#fff"/>' for k in range(4))
    return (f'<g id="LOGO_SENAI_PROVISORIO_substituir_pelo_oficial">'
            f'{rr(x, y, w, hh, 34, "#fff", 0)}'
            f'<path d="{d}" fill="#1A4FBF"/>{lines}</g>')


def sticker(cx, cy, r, lines, rot, font, size, fill=LIME):
    """selo redondo; o tamanho do texto encolhe até caber dentro do círculo"""
    n = len(lines)
    inner = r - 38
    while True:
        lh = size * 0.94
        ys = [(i - (n - 1) / 2) * lh for i in range(n)]
        ok = all(font.width(t, size, 1) <= 2 * math.sqrt(max(inner ** 2 - (abs(dy) + font.cap * size / 2) ** 2, 0))
                 for t, dy in zip(lines, ys))
        if ok:
            break
        size -= 2
    o = [f'<g transform="rotate({rot} {cx} {cy})">',
         circ(cx + 14, cy + 18, r, NAVY, 0),
         circ(cx, cy, r, fill, 12)]
    for t, dy in zip(lines, ys):
        o.append(f'<path d="{font.d(t, size, cx, cy + dy + font.cap * size / 2, anchor="middle", tracking=1)}" fill="{NAVY}"/>')
    o.append("</g>")
    return "".join(o)


def bottom_bar():
    y = h(0.11)
    hh = H - y
    cy = y + hh / 2
    size = 64
    o = f'<rect x="0" y="{y}" width="{W}" height="{hh}" fill="{LIME}"/><line x1="0" y1="{y}" x2="{W}" y2="{y}" {stroke(8)}/>'
    base = cy + BOLD.cap * size / 2
    s = size * 0.95
    ix, iy = 110, cy - s / 2
    o += (f'<rect x="{ix}" y="{iy:.0f}" width="{s:.0f}" height="{s:.0f}" rx="{s * .28:.0f}" fill="none" stroke="{NAVY}" stroke-width="{s * .1:.0f}"/>'
          f'<circle cx="{ix + s / 2:.0f}" cy="{iy + s / 2:.0f}" r="{s * .22:.0f}" fill="none" stroke="{NAVY}" stroke-width="{s * .1:.0f}"/>'
          f'<circle cx="{ix + s * .76:.0f}" cy="{iy + s * .24:.0f}" r="{s * .06:.0f}" fill="{NAVY}"/>')
    o += f'<path d="{BOLD.d("@sistemafieptelemacoborba", size, ix + s + 24, base, tracking=2)}" fill="{NAVY}"/>'
    o += f'<path d="{BLACK.d("#MundoSENAI", size, W - 110, base, anchor="end", tracking=2)}" fill="{NAVY}"/>'
    return o


# ---------------------------------------------------------------- montagem
def panel_body():
    defs, bg = sky()
    parts = [bg]
    parts += [cloud(150, 760, 1.0), cloud(2560, 720, 1.05), cloud(1260, 1180, 0.55, .9), cloud(1580, 1110, 0.6, .9),
              cloud(-60, 1150, 0.8), cloud(2780, 1180, 0.75)]
    parts.append(hills())
    parts.append(ground())
    parts.append(factory())
    # eucaliptos nas laterais (atrás dos personagens)
    for x, top, w, ds in ((70, h(1.62), 1.0, 1), (285, h(1.35), 0.85, -1), (650, h(1.22), 0.72, 1),
                          (2930, h(1.62), 1.0, -1), (2720, h(1.35), 0.85, 1), (2420, h(1.20), 0.7, -1)):
        parts.append(eucalyptus(x, top, h(0.30), w, ds))
    parts.append(paper_rolls())
    parts.append(bottom_bar())
    parts.append(f'<g id="personagem_esquerda" transform="translate(380 {h(0.16)})">{worker_woman()}</g>')
    parts.append(f'<g id="personagem_direita" transform="translate(2640 {h(0.16)})">{worker_man()}</g>')
    t, base = title()
    parts.append(f'<g id="titulo">{t}</g>')
    # linha superior: data | logo | cidade
    parts.append(pill(590, 155, "05 E 06 DE NOVEMBRO · 2026", BOLD, 60))
    parts.append(logo_placeholder(W / 2, 155))
    parts.append(pill(2410, 155, "TELÊMACO BORBA · PR", BOLD, 60))
    # selos na altura do rosto, ao lado de quem posa (aparecem no recorte vertical do celular)
    parts.append(sticker(1100, h(1.55), 225, ["Seu futuro", "começa", "aqui!"], -8, XBOLD, 80, "#fff"))
    parts.append(sticker(1900, h(1.55), 225, ["#Mundo", "SENAI"], 8, BLACK, 110))
    return defs, "".join(parts), base


def svg(defs, body, extra="", w_attr=f"{W}mm", h_attr=f"{H}mm"):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w_attr}" height="{h_attr}" viewBox="0 0 {W} {H}">'
            f'<title>Painel de fotos Mundo SENAI Telêmaco Borba 2026 (3,00 x 2,50 m)</title>'
            f'<defs>{defs}</defs>{body}{extra}</svg>')


# ---------------------------------------------------------------- simulação (pessoas + recorte celular)
def person(cx, real_h, color="#0B1230", op=0.82, factor=1.35, cam=1.45):
    """silhueta projetada no plano do painel (pessoa ~0,8 m à frente, câmera a ~3 m)."""
    proj = lambda m: cam + (m - cam) * factor
    top = h(proj(real_h))
    head_ry = 0.13 * factor * 1000
    head_rx = 0.095 * factor * 1000
    hcy = top + head_ry
    sh_y = hcy + head_ry + 0.06 * 1000 * factor
    sh_w = 0.20 * factor * 1000
    d = (f"M{cx - 50} {hcy + head_ry - 10} L{cx - 50} {sh_y - 40} "
         f"Q{cx - sh_w} {sh_y - 40} {cx - sh_w} {sh_y + 80} L{cx - sh_w + 30} {H} L{cx + sh_w - 30} {H} "
         f"L{cx + sh_w} {sh_y + 80} Q{cx + sh_w} {sh_y - 40} {cx + 50} {sh_y - 40} L{cx + 50} {hcy + head_ry - 10} Z")
    return (f'<g fill="{color}" opacity="{op}"><ellipse cx="{cx}" cy="{hcy}" rx="{head_rx}" ry="{head_ry}"/>'
            f'<path d="{d}"/></g>')


def height_marks():
    o = []
    for m in (1.0, 1.5, 2.0):
        y = h(m)
        o.append(f'<line x1="0" y1="{y}" x2="{W}" y2="{y}" stroke="#E11D48" stroke-width="5" stroke-dasharray="30 20" opacity=".8"/>')
        o.append(f'<path d="{BOLD.d(f"{m:.1f} m".replace(".", ","), 52, 20, y - 14)}" fill="#E11D48"/>')
    return "".join(o)


def crop_frame(cx, top, bottom):
    hh = bottom - top
    w = hh * 9 / 16
    x = cx - w / 2
    lbl = BOLD.d("foto vertical (celular em pé)", 54, x + 20, bottom - 26)
    return (f'<rect x="{x}" y="{top}" width="{w}" height="{hh}" fill="none" stroke="#E11D48" stroke-width="14" stroke-dasharray="50 26"/>'
            f'<rect x="{x}" y="{bottom - 96}" width="{BOLD.width("foto vertical (celular em pé)", 54) + 40}" height="96" fill="#E11D48"/>'
            f'<path d="{lbl}" fill="#fff"/>')


def main():
    defs, body, base = panel_body()
    os.makedirs(OUT, exist_ok=True)
    with open(os.path.join(OUT, "painel.svg"), "w") as f:
        f.write(svg(defs, body))
    group = person(1180, 1.62) + person(1500, 1.74) + person(1830, 1.83)
    with open(os.path.join(OUT, "sim_grupo.svg"), "w") as f:
        f.write(svg(defs, body, group + height_marks(), "1500px", "1250px"))
    solo = person(1500, 1.72) + crop_frame(1500, 20, h(0.45))
    with open(os.path.join(OUT, "sim_stories.svg"), "w") as f:
        f.write(svg(defs, body, solo, "1500px", "1250px"))
    print("titulo: base y =", round(base), "-> altura da base do título =", round((FLOOR - base) / 1000, 2), "m")


main()
