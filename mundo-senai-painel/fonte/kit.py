"""Peças de apoio do painel: moldura de mão "Crachá do Futuro" (cabe em placa A1) e placa A3 com QR code.

Uso: python3 kit.py <pasta_saida>
"""
import os, sys
import qrcode
from lib import *

OUT = sys.argv[1] if len(sys.argv) > 1 else "."
IG = "sistemafieptelemacoborba"

# ------------------------------------------------------------------ crachá do futuro
BW, BH = 500, 840          # tamanho final do crachá (mm), cabe em placa A1 (594 x 841)
BLEED = 5
CW, CH = BW + 2 * BLEED, BH + 2 * BLEED
TAB = (170, 0, 160, 110)   # aba do cordão
SLOT = (205, 30, 90, 24)   # furo do cordão (corte)
BODY_Y = 80
WIN = (95, 275, 310, 335)  # janela do rosto (corte)

AREAS = [
    ("generico", "Indústria do Paraná"),
    ("automacao", "Automação Industrial"),
    ("eletrotecnica", "Eletrotécnica"),
    ("mecanica", "Mecânica"),
    ("celulose-papel", "Celulose e Papel"),
    ("seguranca", "Segurança do Trabalho"),
]


def badge_outline_d(dx=0, dy=0, grow=0):
    """contorno externo (aba + corpo) como um único caminho"""
    x0, y0, w, h, r = 0 - grow, BODY_Y - grow, BW + 2 * grow, BH - BODY_Y + 2 * grow, 36 + grow
    tx, ty, tw, th, tr = TAB[0] - grow, TAB[1] - grow, TAB[2] + 2 * grow, TAB[3], 30 + grow
    X = lambda v: v + dx
    Y = lambda v: v + dy
    return (f"M{X(x0 + r)} {Y(y0)} L{X(tx)} {Y(y0)} L{X(tx)} {Y(ty + tr)} Q{X(tx)} {Y(ty)} {X(tx + tr)} {Y(ty)} "
            f"L{X(tx + tw - tr)} {Y(ty)} Q{X(tx + tw)} {Y(ty)} {X(tx + tw)} {Y(ty + tr)} L{X(tx + tw)} {Y(y0)} "
            f"L{X(x0 + w - r)} {Y(y0)} Q{X(x0 + w)} {Y(y0)} {X(x0 + w)} {Y(y0 + r)} L{X(x0 + w)} {Y(y0 + h - r)} "
            f"Q{X(x0 + w)} {Y(y0 + h)} {X(x0 + w - r)} {Y(y0 + h)} L{X(x0 + r)} {Y(y0 + h)} Q{X(x0)} {Y(y0 + h)} {X(x0)} {Y(y0 + h - r)} "
            f"L{X(x0)} {Y(y0 + r)} Q{X(x0)} {Y(y0)} {X(x0 + r)} {Y(y0)} Z")


def rrect_d(x, y, w, h, r, dx=0, dy=0):
    x += dx
    y += dy
    return (f"M{x + r} {y} L{x + w - r} {y} Q{x + w} {y} {x + w} {y + r} L{x + w} {y + h - r} Q{x + w} {y + h} {x + w - r} {y + h} "
            f"L{x + r} {y + h} Q{x} {y + h} {x} {y + h - r} L{x} {y + r} Q{x} {y} {x + r} {y} Z")


def barcode(x, y, w, h, seed=11):
    import random
    rnd = random.Random(seed)
    o, cx = [], x
    while cx < x + w - 6:
        bw = rnd.choice((3, 3, 5, 7, 9))
        o.append(f'<rect x="{cx:.1f}" y="{y}" width="{bw}" height="{h}" fill="{NAVY}"/>')
        cx += bw + rnd.choice((3, 4, 6))
    return "".join(o)


def cracha(area, preview=False):
    d = BLEED
    o = []
    defs = f'<clipPath id="bodyClip"><path d="{badge_outline_d(d, d, BLEED)}"/></clipPath>'
    # fundo com sangria
    o.append(f'<g clip-path="url(#bodyClip)">')
    o.append(f'<rect x="0" y="0" width="{CW}" height="{CH}" fill="#fff"/>')
    o.append(f'<rect x="0" y="0" width="{CW}" height="{d + 262}" fill="{BLUE}"/>')
    g = "".join(f'<line x1="{x}" y1="0" x2="{x}" y2="{d + 262}"/>' for x in range(d, CW, 40)) + \
        "".join(f'<line x1="0" y1="{y}" x2="{CW}" y2="{y}"/>' for y in range(d, d + 262, 40))
    o.append(f'<g stroke="{BLUE_L}" stroke-width="1.6" opacity=".6">{g}</g>')
    # faixa zebrada no pé
    o.append(f'<rect x="0" y="{d + BH - 46}" width="{CW}" height="{46 + d}" fill="{NAVY}"/>')
    o.append(f'<clipPath id="hzc"><rect x="0" y="{d + BH - 46}" width="{CW}" height="16"/></clipPath>')
    o.append(f'<g clip-path="url(#hzc)" fill="{LIME}">' + "".join(
        f'<path d="M{x} {d + BH - 30} L{x + 16} {d + BH - 46} L{x + 30} {d + BH - 46} L{x + 14} {d + BH - 30} Z"/>' for x in range(-20, CW + 20, 28)) + "</g>")
    o.append("</g>")
    # furo do cordão (visual)
    sx, sy, sw, sh = SLOT
    o.append(f'<rect x="{sx + d}" y="{sy + d}" width="{sw}" height="{sh}" rx="12" fill="{NAVY}"/>')
    # cabeçalho
    pill_t = "CRACHÁ DO FUTURO"
    ps = 22
    pw = BOLD.width(pill_t, ps, 2) + 40
    o.append(rr(d + BW / 2 - pw / 2, d + 120, pw, 40, 20, LIME, 4)
             + text(BOLD, pill_t, ps, d + BW / 2, d + 140 + BOLD.cap * ps / 2, anchor="middle", tracking=2))
    tsize = BLACK.fit("MUNDO SENAI", 90, 420, 2)
    tw1 = BLACK.width("MUNDO ", tsize, 2)
    tw = BLACK.width("MUNDO SENAI", tsize, 2)
    tx = d + BW / 2 - tw / 2
    ty = d + 238
    d1, d2 = BLACK.d("MUNDO ", tsize, tx, ty, tracking=2), BLACK.d("SENAI", tsize, tx + tw1, ty, tracking=2)
    o.append(f'<g transform="translate(4 5)" fill="{NAVY}" stroke="{NAVY}" stroke-width="10" stroke-linejoin="round"><path d="{d1}"/><path d="{d2}"/></g>'
             f'<g stroke="{NAVY}" stroke-width="10" stroke-linejoin="round" paint-order="stroke"><path d="{d1}" fill="#fff"/><path d="{d2}" fill="{LIME}"/></g>')
    # janela do rosto
    wx, wy, ww, wh = WIN
    o.append(f'<rect x="{wx + d - 12}" y="{wy + d - 12}" width="{ww + 24}" height="{wh + 24}" rx="40" fill="{LIME}" {stroke(6)}/>')
    if preview:
        o.append(f'<rect x="{wx + d}" y="{wy + d}" width="{ww}" height="{wh}" rx="28" fill="#DCE6F5"/>')
        o.append(f'<g fill="#AFC0DD"><circle cx="{wx + d + ww / 2}" cy="{wy + d + 140}" r="70"/>'
                 f'<path d="M{wx + d + 45} {wy + d + wh} Q{wx + d + 45} {wy + d + 228} {wx + d + ww / 2} {wy + d + 228} Q{wx + d + ww - 45} {wy + d + 228} {wx + d + ww - 45} {wy + d + wh} Z"/></g>')
        o.append(text(BOLD, "SEU ROSTO AQUI", 20, wx + d + ww / 2, wy + d + 40, fill="#7D90B5", anchor="middle", tracking=2))
    else:
        o.append(f'<rect x="{wx + d}" y="{wy + d}" width="{ww}" height="{wh}" rx="28" fill="#fff" {stroke(4)}/>')
    o.append(f'<rect x="{wx + d}" y="{wy + d}" width="{ww}" height="{wh}" rx="28" fill="none" {stroke(6)}/>')
    # campos
    fy = d + 655
    o.append(text(BOLD, "CARGO", 17, d + 40, fy, fill=BLUE, tracking=2))
    v1 = "Profissional do Futuro"
    s1 = XBOLD.fit(v1, 38, BW - 80)
    o.append(text(XBOLD, v1, s1, d + 40, fy + 44))
    o.append(text(BOLD, "ÁREA", 17, d + 40, fy + 82, fill=BLUE, tracking=2))
    s2 = XBOLD.fit(area, 30, 260)
    o.append(text(XBOLD, area, s2, d + 40, fy + 116))
    o.append(barcode(d + 330, fy + 66, 130, 46))
    o.append(text(SEMI, "#MundoSENAI", 15, d + 395, fy + 130, anchor="middle"))
    o.append(text(SEMI, "SEU FUTURO COMEÇA AQUI · TELÊMACO BORBA 2026", 13.5, d + BW / 2, d + BH - 13, fill="#fff", anchor="middle", tracking=1))
    # contorno guia (só na prévia)
    if preview:
        o.append(f'<path d="{badge_outline_d(d, d)}" fill="none" {stroke(5)}/>')
    return defs, "".join(o)


def faca():
    """arquivo de corte (faca): magenta, sem preenchimento"""
    d = BLEED
    sx, sy, sw, sh = SLOT
    wx, wy, ww, wh = WIN
    st = 'fill="none" stroke="#FF00FF" stroke-width="0.5"'
    return (f'<path d="{badge_outline_d(d, d)}" {st}/>'
            f'<path d="{rrect_d(sx, sy, sw, sh, 12, d, d)}" {st}/>'
            f'<path d="{rrect_d(wx, wy, ww, wh, 28, d, d)}" {st}/>')


# ------------------------------------------------------------------ placa A3 com QR code
def qr_paths(data, x, y, size):
    q = qrcode.QRCode(error_correction=qrcode.constants.ERROR_CORRECT_M, border=0)
    q.add_data(data)
    q.make(fit=True)
    m = q.get_matrix()
    n = len(m)
    c = size / n
    rects = "".join(f"M{x + j * c:.2f} {y + i * c:.2f}h{c:.2f}v{c:.2f}h-{c:.2f}Z"
                    for i, row in enumerate(m) for j, v in enumerate(row) if v)
    return f'<path d="{rects}" fill="{NAVY}"/>'


def placa():
    w, h = 297, 420
    o = [f'<rect width="{w}" height="{h}" fill="{BLUE}"/>']
    g = "".join(f'<line x1="{x}" y1="0" x2="{x}" y2="{h}"/>' for x in range(0, w + 1, 20)) + \
        "".join(f'<line x1="0" y1="{y}" x2="{w}" y2="{y}"/>' for y in range(0, h + 1, 20))
    o.append(f'<g stroke="{BLUE_L}" stroke-width=".8" opacity=".6">{g}</g>')
    # título
    ts = BLACK.fit("MUNDO SENAI", 60, 250, 1)
    tw1 = BLACK.width("MUNDO ", ts, 1)
    tw = BLACK.width("MUNDO SENAI", ts, 1)
    tx, ty = w / 2 - tw / 2, 62
    d1, d2 = BLACK.d("MUNDO ", ts, tx, ty, tracking=1), BLACK.d("SENAI", ts, tx + tw1, ty, tracking=1)
    o.append(f'<g transform="translate(2.5 3)" fill="{NAVY}" stroke="{NAVY}" stroke-width="6" stroke-linejoin="round"><path d="{d1}"/><path d="{d2}"/></g>'
             f'<g stroke="{NAVY}" stroke-width="6" stroke-linejoin="round" paint-order="stroke"><path d="{d1}" fill="#fff"/><path d="{d2}" fill="{LIME}"/></g>')
    # chamada
    s1 = XBOLD.fit("Tirou foto?", 26, 250)
    o.append(text(XBOLD, "Tirou foto?", s1, w / 2, 102, fill="#fff", anchor="middle"))
    s2 = XBOLD.fit("Posta e marca a gente!", 22, 250)
    o.append(text(XBOLD, "Posta e marca a gente!", s2, w / 2, 130, fill=LIME, anchor="middle"))
    # cartão do QR
    cs = 140
    cx, cy = w / 2 - cs / 2, 146
    ch = cs + 46
    o.append(rr(cx + 4, cy + 5, cs, ch, 14, NAVY, 0))
    o.append(rr(cx, cy, cs, ch, 14, "#fff", 3))
    o.append(qr_paths(f"https://www.instagram.com/{IG}/", cx + 15, cy + 15, cs - 30))
    hs = SEMI.fit("@" + IG, 11, cs - 34)
    hw = SEMI.width("@" + IG, hs) + 15
    hx = w / 2 - hw / 2
    o.append(ig_icon(hx, cy + cs - 2, 11, NAVY))
    o.append(text(SEMI, "@" + IG, hs, hx + 15, cy + cs + 7))
    o.append(text(BLACK, "#MundoSENAI", BLACK.fit("#MundoSENAI", 19, cs - 24), w / 2, cy + cs + 33, anchor="middle"))
    # dicas
    tips = ["Pegue um capacete e um crachá do futuro", "Fique em cima da marca no chão", "Depois, devolva os acessórios aqui :)"]
    for i, t in enumerate(tips):
        yy = cy + ch + 26 + i * 16
        o.append(circ(44, yy - 3.6, 4.5, LIME, 0))
        o.append(text(SEMI, t, SEMI.fit(t, 11, 210), 55, yy, fill="#fff"))
    # rodapé
    o.append(f'<rect x="0" y="{h - 22}" width="{w}" height="22" fill="{NAVY}"/>')
    ft = "05 E 06 DE NOVEMBRO · 2026 · TELÊMACO BORBA · PR"
    o.append(text(SEMI, ft, SEMI.fit(ft, 9, 260, 1), w / 2, h - 8, fill="#fff", anchor="middle", tracking=1))
    return w, h, "".join(o)


def main():
    os.makedirs(OUT, exist_ok=True)
    for slug, area in AREAS:
        defs, body = cracha(area)
        open(os.path.join(OUT, f"cracha-{slug}.svg"), "w").write(
            svg_doc(CW, CH, body, defs, f"Crachá do Futuro — {area} (50 x 84 cm + sangria 5 mm)"))
        defs, body = cracha(area, preview=True)
        open(os.path.join(OUT, f"cracha-{slug}-previa.svg"), "w").write(
            svg_doc(CW, CH, body, defs, f"Crachá do Futuro — {area} (prévia)"))
    open(os.path.join(OUT, "cracha-faca.svg"), "w").write(svg_doc(CW, CH, faca(), "", "Crachá do Futuro — faca de corte"))
    w, h, body = placa()
    open(os.path.join(OUT, "placa-a3.svg"), "w").write(svg_doc(w, h, body, "", "Placa A3 — Posta e marca"))


if __name__ == "__main__":
    main()
