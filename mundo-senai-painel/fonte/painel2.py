"""Painel de fotos Mundo SENAI Telêmaco Borba 2026 — v2 "Portal do Futuro".

3,00 x 2,50 m. Unidades = mm. Origem no canto superior esquerdo; chão em y = 2500.
Uso: python3 painel2.py <pasta_saida>
"""
import math, os, sys
from lib import *

W, H = 3000, 2500
OUT = sys.argv[1] if len(sys.argv) > 1 else "."
FLOOR_TOP = 2240            # início da faixa de piso (h = 0,26 m)
CX, CY, R = 1500, 1560, 1000  # o "mundo": círculo central, topo em y=560 (h = 1,94 m)


def h(m):
    return H - m * 1000


# ------------------------------------------------------------------ fundo
def background():
    o = [f'<rect width="{W}" height="{H}" fill="{BLUE}"/>']
    # grade de desenho técnico (planta azul)
    g = []
    for x in range(0, W + 1, 100):
        g.append(f'<line x1="{x}" y1="0" x2="{x}" y2="{H}" stroke-width="{5 if x % 500 == 0 else 2.5}"/>')
    for y in range(0, H + 1, 100):
        g.append(f'<line x1="0" y1="{y}" x2="{W}" y2="{y}" stroke-width="{5 if y % 500 == 0 else 2.5}"/>')
    o.append(f'<g stroke="{BLUE_L}" opacity=".55">{"".join(g)}</g>')
    return "".join(o)


def sparkle(x, y, s, fill=WHITE, op=1):
    d = f"M{x} {y - s} Q{x + s * .18} {y - s * .18} {x + s} {y} Q{x + s * .18} {y + s * .18} {x} {y + s} Q{x - s * .18} {y + s * .18} {x - s} {y} Q{x - s * .18} {y - s * .18} {x} {y - s} Z"
    return f'<path d="{d}" fill="{fill}" opacity="{op}"/>'


def orbits():
    o = [f'<circle cx="{CX}" cy="{CY}" r="{R + 150}" fill="none" stroke="#fff" stroke-width="4" opacity=".28"/>',
         f'<circle cx="{CX}" cy="{CY}" r="{R + 75}" fill="none" stroke="{LIME}" stroke-width="9" stroke-dasharray="38 26"/>']
    for x, y, s in ((175, 330, 30), (2825, 330, 30), (95, 560, 18), (2905, 560, 18)):
        o.append(sparkle(x, y, s, WHITE, .9))
    return "".join(o)


# ------------------------------------------------------------------ o "mundo" (cena dentro do círculo)
def cloud(x, y, s=1.0, op=1):
    parts = [(0, 0, 90), (95, -40, 110), (210, -10, 95), (300, 20, 70), (150, 40, 90), (40, 40, 70)]
    c = "".join(f'<circle cx="{x + dx * s:.0f}" cy="{y + dy * s:.0f}" r="{r * s:.0f}"/>' for dx, dy, r in parts)
    return f'<g fill="#fff" opacity="{op}">{c}</g>'


def drone(x, y, s=1.0):
    o = [f'<g transform="translate({x} {y}) scale({s})">']
    o.append(f'<line x1="-95" y1="-10" x2="95" y2="-10" {stroke(9)}/>')
    for dx in (-95, 95):
        o.append(f'<line x1="{dx}" y1="-10" x2="{dx}" y2="-34" {stroke(8)}/>')
        o.append(f'<ellipse cx="{dx}" cy="-38" rx="55" ry="10" fill="#fff" {stroke(7)}/>')
    o.append(rr(-55, -30, 110, 48, 18, "#fff", 8))
    o.append(circ(0, 6, 14, NAVY, 0))
    o.append(circ(30, -8, 7, LIME, 0))
    o.append("</g>")
    return "".join(o)


def drone_banner(x, y):
    """drone carregando a faixa #MundoSENAI — fica ao lado do rosto de quem posa (entra na foto vertical)"""
    w, hh = 380, 112
    by = y + 115
    o = [f'<line x1="{x - 40}" y1="{y + 14}" x2="{x - w / 2 + 40}" y2="{by}" {stroke(6)}/>',
         f'<line x1="{x + 40}" y1="{y + 14}" x2="{x + w / 2 - 40}" y2="{by}" {stroke(6)}/>',
         f'<g transform="rotate(4 {x} {by + hh / 2})">',
         rr(x - w / 2 + 10, by + 12, w, hh, 22, NAVY, 0),
         rr(x - w / 2, by, w, hh, 22, LIME, 9)]
    sz = BLACK.fit("#MundoSENAI", 80, w - 50, 1)
    o.append(text(BLACK, "#MundoSENAI", sz, x, by + hh / 2 + BLACK.cap * sz / 2, anchor="middle", tracking=1))
    o.append("</g>")
    o.append(drone(x, y, 1.0))
    return "".join(o)


def euc_forest():
    """faixa de eucalipto plantado no horizonte (silhuetas sem contorno = profundidade)"""
    o = []
    import random
    rnd = random.Random(7)
    for row, (base, tone, hmin, hmax, step) in enumerate(((1975, "#4DBB80", 260, 420, 62), (2010, "#2F9C66", 200, 330, 78))):
        x = 520 + rnd.uniform(0, step)
        while x < 2490:
            ht = rnd.uniform(hmin, hmax)
            rx = rnd.uniform(38, 54)
            top = base - ht
            o.append(f'<line x1="{x:.0f}" y1="{base}" x2="{x:.0f}" y2="{top + ht * .55:.0f}" stroke="#EFE7D5" stroke-width="9"/>')
            o.append(f'<ellipse cx="{x:.0f}" cy="{top + ht * .33:.0f}" rx="{rx:.0f}" ry="{ht * .36:.0f}" fill="{tone}"/>')
            o.append(f'<ellipse cx="{x - rx * .3:.0f}" cy="{top + ht * .28:.0f}" rx="{rx * .45:.0f}" ry="{ht * .22:.0f}" fill="#fff" opacity=".12"/>')
            x += step * rnd.uniform(.75, 1.15)
    return "".join(o)


def factory():
    x0, x1, top, base = 1600, 2290, 1945, 2190
    o = []
    # torre do digestor (celulose) — baixa, fica atrás do tronco de quem posa
    o.append(rr(2300, 1660, 95, base - 1660, 22, "#fff", 8))
    o.append(f'<rect x="2300" y="1720" width="95" height="34" fill="{LIME}" {stroke(7)}/>')
    o.append(f'<path d="M2347 1660 L2347 1620" {stroke(8)}/>')
    # shed: vidro na face vertical
    n = 6
    tw = (x1 - x0) / n
    for i in range(n):
        a = x0 + i * tw
        o.append(path(f"M{a:.0f} {top} L{a + tw:.0f} {top - 115} L{a + tw:.0f} {top} Z", ROYAL, 8))
        o.append(f'<rect x="{a + tw - 26:.0f}" y="{top - 95}" width="20" height="90" fill="#A9DEFF"/>')
    o.append(rr(x0, top, x1 - x0, base - top, 6, "#F5F7FF", 8))
    o.append(f'<rect x="{x0 + 4}" y="{top + 58}" width="{x1 - x0 - 8}" height="20" fill="{LIME}"/>')
    for i in range(5):
        wx = x0 + 40 + i * 100
        o.append(rr(wx, top + 105, 70, 55, 8, "#A9DEFF", 7))
    o.append(rr(x1 - 150, top + 95, 110, base - top - 95, 4, NAVY, 0))
    for k in range(3):
        o.append(f'<rect x="{x1 - 140}" y="{top + 115 + k * 38}" width="90" height="12" rx="6" fill="{LIME}"/>')
    return "".join(o)


def paper_roll(cx, cy, r, depth=(70, -28)):
    dx, dy = depth
    bx, by = cx + dx, cy + dy
    o = [circ(bx, by, r, "#C7D0E4", 8),
         path(f"M{cx} {cy - r} L{bx} {by - r} L{bx} {by + r} L{cx} {cy + r} Z", "#DCE3F1", 0),
         f'<line x1="{cx}" y1="{cy - r}" x2="{bx}" y2="{by - r}" {stroke(8)}/>',
         f'<line x1="{cx}" y1="{cy + r}" x2="{bx}" y2="{by + r}" {stroke(8)}/>',
         circ(cx, cy, r, "#fff", 9)]
    for k in (0.78, 0.58):
        o.append(f'<circle cx="{cx}" cy="{cy}" r="{r * k:.1f}" fill="none" stroke="#C9D3E6" stroke-width="6"/>')
    o.append(circ(cx, cy, r * 0.24, NAVY, 0))
    o.append(circ(cx, cy, r * 0.10, "#C9D3E6", 0))
    return "".join(o)


def robot_arm():
    o = []
    o.append(rr(1180, 2148, 170, 46, 14, NAVY, 0))
    o.append(rr(1212, 2080, 106, 76, 24, ORANGE, 9))
    o.append(tube("M1265 2085 L1330 1905", ORANGE, 60, 9))
    o.append(circ(1265, 2085, 30, "#fff", 8))
    o.append(tube("M1330 1905 L1110 1838", ORANGE, 48, 9))
    o.append(circ(1330, 1905, 30, "#fff", 8))
    # garra
    o.append(rr(1078, 1830, 64, 42, 10, NAVY, 0))
    o.append(path("M1084 1868 L1070 1915 L1090 1918 L1100 1872 Z", NAVY, 0))
    o.append(path("M1136 1868 L1150 1915 L1130 1918 L1120 1872 Z", NAVY, 0))
    o.append(circ(1110, 1838, 10, LIME, 0))
    return "".join(o)


def world():
    defs = f'''
<clipPath id="worldClip"><path d="M{CX - R} {CY} A {R} {R} 0 0 1 {CX + R} {CY} A {R} {R} 0 0 1 {CX - R} {CY} Z"/></clipPath>
<clipPath id="worldFloor"><rect x="0" y="0" width="{W}" height="{FLOOR_TOP}"/></clipPath>
<linearGradient id="wsky" gradientUnits="userSpaceOnUse" x1="0" y1="{CY - R}" x2="0" y2="2000">
  <stop offset="0" stop-color="#7FC9FF"/><stop offset=".55" stop-color="#CDEBFF"/><stop offset="1" stop-color="#FFE2B3"/>
</linearGradient>'''
    o = [f'<g clip-path="url(#worldFloor)"><g clip-path="url(#worldClip)">']
    o.append(f'<rect x="{CX - R}" y="{CY - R}" width="{2 * R}" height="{2 * R}" fill="url(#wsky)"/>')
    # raios de sol saindo do horizonte
    sx, sy = CX, 1860
    rays = []
    n = 26
    for i in range(0, n, 2):
        a0 = math.pi + i * math.pi / n
        a1 = math.pi + (i + 1) * math.pi / n
        rays.append(f"M{sx} {sy} L{sx + 1600 * math.cos(a0):.0f} {sy + 1600 * math.sin(a0):.0f} "
                    f"L{sx + 1600 * math.cos(a1):.0f} {sy + 1600 * math.sin(a1):.0f} Z")
    o.append(f'<path d="{" ".join(rays)}" fill="#fff" opacity=".16"/>')
    o.append(f'<circle cx="{sx}" cy="{sy}" r="430" fill="#FFF3CF" opacity=".55"/>')
    o.append(f'<circle cx="{sx}" cy="{sy}" r="340" fill="#FFE59A" opacity=".75"/>')
    o.append(f'<circle cx="{sx}" cy="{sy}" r="250" fill="#FFCF4D"/>')
    o.append(cloud(760, 1120, .75, .95) + cloud(1980, 980, .85, .95) + cloud(2160, 1420, .55, .9))
    o.append(drone_banner(1875, 785))
    o.append(f'<path d="M{CX - R} 1900 C 800 1790 1050 1770 1300 1840 S 1800 1905 2050 1810 S 2400 1770 {CX + R} 1830 L{CX + R} 2300 L{CX - R} 2300 Z" fill="#A6E1C0"/>')
    o.append(euc_forest())
    o.append(f'<path d="M{CX - R} 2030 C 900 1975 1300 2015 1500 2005 S 2200 1975 {CX + R} 2015 L{CX + R} 2300 L{CX - R} 2300 Z" fill="#5FC78C"/>')
    o.append(f'<path d="M{CX - R} 2110 C 1000 2080 1400 2100 1600 2095 S 2300 2075 {CX + R} 2100 L{CX + R} 2300 L{CX - R} 2300 Z" fill="#4DB77C"/>')
    o.append(factory())
    o.append(paper_roll(870, 2135, 82) + paper_roll(1045, 2135, 82) + paper_roll(958, 1985, 82))
    o.append(robot_arm())
    o.append("</g></g>")
    # contorno do mundo
    o.append(f'<g clip-path="url(#worldFloor)"><circle cx="{CX}" cy="{CY}" r="{R}" fill="none" stroke="{NAVY}" stroke-width="16"/></g>')
    return defs, "".join(o)


# ------------------------------------------------------------------ piso
def floor():
    o = [f'<rect x="0" y="{FLOOR_TOP}" width="{W}" height="{H - FLOOR_TOP}" fill="{NAVY}"/>']
    # faixa zebrada de segurança (piso de fábrica)
    band = 46
    o.append(f'<clipPath id="hz"><rect x="0" y="{FLOOR_TOP}" width="{W}" height="{band}"/></clipPath>')
    stripes = "".join(f'<path d="M{x} {FLOOR_TOP + band} L{x + band} {FLOOR_TOP} L{x + band + 40} {FLOOR_TOP} L{x + 40} {FLOOR_TOP + band} Z"/>'
                      for x in range(-60, W + 60, 80))
    o.append(f'<rect x="0" y="{FLOOR_TOP}" width="{W}" height="{band}" fill="{NAVY}"/>')
    o.append(f'<g clip-path="url(#hz)" fill="{LIME}">{stripes}</g>')
    o.append(f'<line x1="0" y1="{FLOOR_TOP}" x2="{W}" y2="{FLOOR_TOP}" {stroke(8)}/>')
    return "".join(o)


def floor_handle():
    handle = "@sistemafieptelemacoborba"
    size = 62
    s = size * .9
    tw = SEMI.width(handle, size, 1) + s + 24
    x = W / 2 - tw / 2
    cy = FLOOR_TOP + 46 + (H - FLOOR_TOP - 46) / 2
    return ig_icon(x, cy - s / 2, s, "#fff") + text(SEMI, handle, size, x + s + 24, cy + SEMI.cap * size / 2, fill="#fff", tracking=1)


# ------------------------------------------------------------------ personagens (origem = pés)
def glasses(y, sep=50):
    """óculos de proteção sobre os olhos (EPI correto), lentes translúcidas"""
    o = []
    for sx in (-1, 1):
        o.append(rr(sx * sep - 44, y - 30, 88, 58, 24, "#DDF2FF", 7, 'fill-opacity=".45"'))
    o.append(f'<path d="M-6 {y - 6} Q0 {y - 14} 6 {y - 6}" fill="none" {stroke(7)}/>')
    return "".join(o)


def mitten(x, y, r, skin, skin_d, ang=0):
    return (f'<g transform="rotate({ang} {x} {y})">'
            f'<ellipse cx="{x - r * .82}" cy="{y + r * .15}" rx="{r * .38}" ry="{r * .55}" fill="{skin}" {stroke(8)}/>'
            f'<ellipse cx="{x}" cy="{y}" rx="{r * .88}" ry="{r}" fill="{skin}" {stroke(8)}/>'
            f'</g>')


def woman():
    o = [f'<ellipse cx="0" cy="0" rx="270" ry="28" fill="#000" opacity=".25"/>']
    # pernas e botinas
    o.append(path("M-165 -735 L-35 -735 L-55 -75 L-145 -75 Z", "#24318F"))
    o.append(path("M35 -735 L165 -735 L145 -75 L55 -75 Z", "#24318F"))
    o.append(path("M-185 -4 Q-190 -96 -120 -96 L-60 -96 Q-40 -96 -40 -66 L-40 -4 Z", "#7A4A2A", 8))
    o.append(path("M185 -4 Q190 -96 120 -96 L60 -96 Q40 -96 40 -66 L40 -4 Z", "#7A4A2A", 8))
    o.append(f'<rect x="-188" y="-26" width="150" height="22" rx="8" fill="#D8B48A"/><rect x="38" y="-26" width="150" height="22" rx="8" fill="#D8B48A"/>')
    o.append('<g transform="translate(0 70)">')
    # cabelo (atrás)
    o.append(path("M-152 -1345 C-182 -1185 -158 -1112 -98 -1108 L98 -1108 C158 -1112 182 -1185 152 -1345 Z", HAIR1, 8))
    # pescoço
    o.append(rr(-44, -1290, 88, 80, 14, SKIN1_D, 8))
    # tronco: camisa + cinto + colete
    o.append(path("M-200 -1172 Q-200 -1248 -128 -1248 L128 -1248 Q200 -1248 200 -1172 L190 -790 L-190 -790 Z", "#F4F6FB"))
    o.append(rr(-188, -838, 376, 46, 6, NAVY, 0))
    o.append('<clipPath id="vestW"><path d="M-196 -1182 Q-196 -1236 -150 -1241 L-78 -1246 L0 -1092 L78 -1246 L150 -1241 Q196 -1236 196 -1182 L188 -838 L-188 -838 Z"/></clipPath>')
    o.append(path("M-196 -1182 Q-196 -1236 -150 -1241 L-78 -1246 L0 -1092 L78 -1246 L150 -1241 Q196 -1236 196 -1182 L188 -838 L-188 -838 Z", LIME))
    o.append(f'<g clip-path="url(#vestW)">'
             f'<rect x="-210" y="-1000" width="420" height="44" fill="{STRIPE}" {stroke(6)}/>'
             f'<rect x="-210" y="-918" width="420" height="44" fill="{STRIPE}" {stroke(6)}/></g>')
    o.append(f'<line x1="0" y1="-1092" x2="0" y2="-838" {stroke(6)}/>')
    # crachá no colete
    o.append(rr(58, -1085, 74, 98, 10, "#fff", 6) + f'<rect x="58" y="-1085" width="74" height="22" rx="8" fill="{BLUE}"/>'
             + circ(95, -1040, 13, SKIN1_D, 0) + f'<rect x="72" y="-1018" width="46" height="8" rx="4" fill="{NAVY}"/>')
    # braço do tablet
    o.append(tube("M-178 -1195 L-238 -1010 L-120 -935", "#F4F6FB", 92))
    o.append(f'<g transform="rotate(-9 -165 -960)">{rr(-305, -1068, 265, 188, 24, NAVY, 0)}{rr(-288, -1051, 231, 154, 12, "#A9DEFF", 0)}'
             f'<rect x="-268" y="-950" width="36" height="40" rx="4" fill="{LIME}"/><rect x="-222" y="-985" width="36" height="75" rx="4" fill="{LIME}"/>'
             f'<rect x="-176" y="-1022" width="36" height="112" rx="4" fill="{LIME}"/><rect x="-130" y="-1000" width="36" height="90" rx="4" fill="#fff"/></g>')
    o.append(mitten(-112, -932, 50, SKIN1, SKIN1_D, -20))
    # braço acenando (mão aberta)
    o.append(tube("M175 -1195 L300 -1268 L322 -1440", "#F4F6FB", 92))
    fingers = "".join(f'<rect x="{fx}" y="{fy}" width="24" height="{fl}" rx="12" fill="{SKIN1}" {stroke(7)}/>'
                      for fx, fy, fl in ((284, -1592, 70), (309, -1606, 82), (334, -1604, 80), (359, -1590, 66)))
    o.append(f'<g transform="rotate(10 328 -1505)">'
             f'<ellipse cx="280" cy="-1484" rx="20" ry="40" transform="rotate(-38 280 -1484)" fill="{SKIN1}" {stroke(7)}/>'
             f'{fingers}{rr(280, -1550, 106, 98, 38, SKIN1, 8)}'
             f'<path d="M296 -1548 L370 -1548" stroke="{SKIN1}" stroke-width="14"/></g>')
    # cabeça
    o.append(circ(-136, -1385, 27, SKIN1, 8) + circ(136, -1385, 27, SKIN1, 8))
    o.append(circ(0, -1392, 138, SKIN1))
    o.append(path("M-138 -1418 Q-118 -1480 -40 -1474 Q40 -1505 140 -1428 L140 -1450 L-140 -1450 Z", HAIR1, 0))
    o.append(f'<ellipse cx="-50" cy="-1385" rx="13" ry="17" fill="{NAVY}"/><ellipse cx="50" cy="-1385" rx="13" ry="17" fill="{NAVY}"/>')
    o.append(f'<path d="M-78 -1428 Q-52 -1442 -26 -1428 M26 -1428 Q52 -1442 78 -1428" fill="none" {stroke(8)}/>')
    o.append(f'<ellipse cx="-92" cy="-1336" rx="26" ry="16" fill="#F27D92" opacity=".5"/><ellipse cx="92" cy="-1336" rx="26" ry="16" fill="#F27D92" opacity=".5"/>')
    o.append(f'<path d="M-9 -1352 Q0 -1342 9 -1352" fill="none" {stroke(7)}/>')
    o.append(path("M-46 -1318 Q0 -1262 46 -1318 Z", NAVY, 6) + f'<ellipse cx="0" cy="-1296" rx="17" ry="9" fill="#F27D92"/>')
    o.append(glasses(-1385))
    # capacete amarelo
    o.append(path("M-150 -1446 C-150 -1566 -88 -1620 0 -1620 C88 -1620 150 -1566 150 -1446 Z", YEL))
    o.append(path("M-26 -1617 Q0 -1621 26 -1617 L30 -1450 L-30 -1450 Z", YEL_L, 0))
    o.append(path("M-150 -1446 C-150 -1566 -88 -1620 0 -1620 C88 -1620 150 -1566 150 -1446", "none"))
    o.append(f'<ellipse cx="0" cy="-1446" rx="205" ry="30" fill="{YEL}" {stroke()}/>')
    o.append('</g>')
    return "".join(o)


def man(sign_svg):
    o = [f'<ellipse cx="0" cy="0" rx="285" ry="28" fill="#000" opacity=".25"/>']
    U = "#24318F"
    o.append(path("M-175 -750 L-38 -750 L-58 -85 L-158 -85 Z", U))
    o.append(path("M38 -750 L175 -750 L158 -85 L58 -85 Z", U))
    o.append(path("M-205 -4 Q-210 -104 -132 -104 L-62 -104 Q-40 -104 -40 -70 L-40 -4 Z", "#6B3E26", 8))
    o.append(path("M205 -4 Q210 -104 132 -104 L62 -104 Q40 -104 40 -70 L40 -4 Z", "#6B3E26", 8))
    o.append(f'<rect x="-208" y="-26" width="168" height="22" rx="8" fill="#D8B48A"/><rect x="40" y="-26" width="168" height="22" rx="8" fill="#D8B48A"/>')
    o.append('<g transform="translate(0 70)">')
    o.append(rr(-46, -1318, 92, 84, 14, SKIN2_D, 8))
    o.append(path("M-212 -1196 Q-212 -1276 -136 -1276 L136 -1276 Q212 -1276 212 -1196 L198 -800 L-198 -800 Z", U))
    o.append('<clipPath id="vestM"><path d="M-208 -1206 Q-208 -1266 -160 -1270 L-80 -1272 L-70 -846 L-198 -846 Z M208 -1206 Q208 -1266 160 -1270 L80 -1272 L70 -846 L198 -846 Z"/></clipPath>')
    o.append(path("M-208 -1206 Q-208 -1266 -160 -1270 L-80 -1272 L-70 -846 L-198 -846 Z", LIME))
    o.append(path("M208 -1206 Q208 -1266 160 -1270 L80 -1272 L70 -846 L198 -846 Z", LIME))
    o.append(f'<g clip-path="url(#vestM)"><rect x="-220" y="-1020" width="440" height="46" fill="{STRIPE}" {stroke(6)}/>'
             f'<rect x="-220" y="-935" width="440" height="46" fill="{STRIPE}" {stroke(6)}/></g>')
    o.append(rr(-150, -1110, 74, 98, 10, "#fff", 6) + f'<rect x="-150" y="-1110" width="74" height="22" rx="8" fill="{BLUE}"/>'
             + circ(-113, -1065, 13, SKIN2_D, 0) + f'<rect x="-136" y="-1043" width="46" height="8" rx="4" fill="{NAVY}"/>')
    # placa erguida com as duas mãos
    o.append(tube("M-185 -1225 L-318 -1415 L-292 -1622", U, 96))
    o.append(tube("M185 -1225 L318 -1415 L292 -1622", U, 96))
    o.append(sign_svg)
    o.append(mitten(-292, -1652, 54, SKIN2, SKIN2_D, 15))
    o.append(mitten(292, -1652, 54, SKIN2, SKIN2_D, 165))
    # cabeça
    o.append(circ(-140, -1412, 28, SKIN2, 8) + circ(140, -1412, 28, SKIN2, 8))
    o.append(circ(0, -1420, 142, SKIN2))
    o.append(path("M-128 -1392 Q-122 -1290 -40 -1268 Q0 -1258 40 -1268 Q122 -1290 128 -1392 Q118 -1330 70 -1318 Q0 -1300 -70 -1318 Q-118 -1330 -128 -1392 Z", HAIR2, 0))
    o.append(path("M-142 -1440 Q-140 -1490 -110 -1500 L110 -1500 Q140 -1490 142 -1440 L120 -1440 L120 -1475 L-120 -1475 L-120 -1440 Z", HAIR2, 0))
    o.append(f'<ellipse cx="-52" cy="-1418" rx="13" ry="17" fill="{NAVY}"/><ellipse cx="52" cy="-1418" rx="13" ry="17" fill="{NAVY}"/>')
    o.append(f'<path d="M-80 -1462 Q-54 -1476 -28 -1462 M28 -1462 Q54 -1476 80 -1462" fill="none" {stroke(9)}/>')
    o.append(f'<path d="M-10 -1384 Q0 -1372 10 -1384" fill="none" {stroke(7)}/>')
    o.append(path("M-48 -1346 Q0 -1290 48 -1346 Z", "#fff", 7))
    o.append(glasses(-1418, 52))
    o.append(path("M-155 -1478 C-155 -1600 -92 -1655 0 -1655 C92 -1655 155 -1600 155 -1478 Z", "#fff"))
    o.append(path("M-28 -1652 Q0 -1656 28 -1652 L32 -1482 L-32 -1482 Z", ROYAL, 0))
    o.append(path("M-155 -1478 C-155 -1600 -92 -1655 0 -1655 C92 -1655 155 -1600 155 -1478", "none"))
    o.append(f'<ellipse cx="0" cy="-1478" rx="212" ry="31" fill="#fff" {stroke()}/>')
    o.append('</g>')
    return "".join(o)


def man_sign():
    """placa '#MundoSENAI' (coordenadas locais do personagem)"""
    w, hh, x, y = 640, 196, -320, -1858
    o = [f'<g transform="rotate(-3 0 {y + hh / 2})">',
         rr(x + 14, y + 16, w, hh, 30, NAVY, 0),
         rr(x, y, w, hh, 30, LIME, 10)]
    s1 = BLACK.fit("#MundoSENAI", 130, w - 80, 1)
    o.append(text(BLACK, "#MundoSENAI", s1, 0, y + hh / 2 + BLACK.cap * s1 / 2 - 6, anchor="middle", tracking=1))
    o.append("</g>")
    return "".join(o)


def bubble():
    """balão da personagem: 'Seu futuro começa aqui!' (contorno único com a ponta)"""
    l1, l2 = "Seu futuro", "começa aqui!"
    size = 86
    tw = max(XBOLD.width(l1, size), XBOLD.width(l2, size))
    padx, pady = 56, 44
    lh = size * 1.04
    w = tw + 2 * padx
    hh = lh * 2 + 2 * pady - size * .25
    x, y, r = 520, 612, 56
    b = y + hh
    d = (f"M{x + r} {y} L{x + w - r} {y} Q{x + w} {y} {x + w} {y + r} L{x + w} {b - r} Q{x + w} {b} {x + w - r} {b} "
         f"L{x + 235} {b} L{x + 40} {b + 100} L{x + 125} {b} "
         f"L{x + r} {b} Q{x} {b} {x} {b - r} L{x} {y + r} Q{x} {y} {x + r} {y} Z")
    return "".join([f'<g transform="rotate(-3 {x + w / 2} {y + hh / 2})">',
                    f'<path d="{d}" fill="{NAVY}" transform="translate(14 16)"/>',
                    path(d, "#fff", 10),
                    text(XBOLD, l1, size, x + w / 2, y + pady + XBOLD.cap * size, anchor="middle"),
                    text(XBOLD, l2, size, x + w / 2, y + pady + XBOLD.cap * size + lh, anchor="middle", fill=BLUE),
                    "</g>"])


# ------------------------------------------------------------------ marca e título
def title():
    size, trk = 380, 6
    w1 = BLACK.width("MUNDO ", size, trk)
    w2 = BLACK.width("SENAI", size, trk)
    k = min(1, 2600 / (w1 + w2))
    size, w1, w2 = size * k, w1 * k, w2 * k
    base = 520
    x0 = W / 2 - (w1 + w2) / 2
    d1 = BLACK.d("MUNDO ", size, x0, base, tracking=trk)
    d2 = BLACK.d("SENAI", size, x0 + w1, base, tracking=trk)
    return (f'<g transform="translate(16 20)" fill="{NAVY}" stroke="{NAVY}" stroke-width="34" stroke-linejoin="round"><path d="{d1}"/><path d="{d2}"/></g>'
            f'<g stroke="{NAVY}" stroke-width="34" stroke-linejoin="round" paint-order="stroke"><path d="{d1}" fill="#fff"/><path d="{d2}" fill="{LIME}"/></g>')


def pill(cx, cy, s, size=56, hh=118):
    tw = BOLD.width(s, size, 2)
    w = tw + 120
    return (rr(cx - w / 2, cy - hh / 2, w, hh, hh / 2, LIME, 9)
            + text(BOLD, s, size, cx, cy + BOLD.cap * size / 2, anchor="middle", tracking=2))


def logo_slot(cx, cy):
    """PROVISÓRIO: substituir pelo arquivo vetorial oficial do SENAI."""
    w, hh = 540, 190
    size = 160
    return (f'<g id="LOGO_SENAI_PROVISORIO_substituir_pelo_oficial">'
            + rr(cx - w / 2, cy - hh / 2, w, hh, 30, "#fff", 0)
            + text(BLACK_IT, "SENAI", size, cx, cy + BLACK_IT.cap * size / 2, fill="#1747B5", anchor="middle", tracking=6)
            + "</g>")


# ------------------------------------------------------------------ montagem
def panel():
    wdefs, wbody = world()
    body = [background(), orbits(), wbody, floor(),
            f'<g id="personagem_esquerda" transform="translate(380 2452) scale(.97)">{woman()}</g>',
            f'<g id="personagem_direita" transform="translate(2565 2456) scale(.97)">{man(man_sign())}</g>',
            floor_handle(),
            bubble(),
            f'<g id="titulo">{title()}</g>',
            pill(590, 128, "05 E 06 DE NOVEMBRO · 2026"),
            logo_slot(W / 2, 128),
            pill(2410, 128, "TELÊMACO BORBA · PR")]
    return wdefs, "".join(body)


# ------------------------------------------------------------------ simulação
def person(cx, real_h, factor=1.35, cam=1.45, op=.85):
    proj = lambda m: cam + (m - cam) * factor
    top = h(proj(real_h))
    hr = 0.12 * factor * 1000
    hw = 0.085 * factor * 1000
    hcy = top + hr
    sh = 0.205 * factor * 1000
    ny = hcy + hr - 15
    sy = ny + 60
    d = (f"M{cx - 45} {ny} L{cx - 45} {sy} C{cx - sh * .7} {sy} {cx - sh} {sy + 10} {cx - sh} {sy + 120} "
         f"L{cx - sh + 25} {H} L{cx + sh - 25} {H} L{cx + sh} {sy + 120} C{cx + sh} {sy + 10} {cx + sh * .7} {sy} {cx + 45} {sy} L{cx + 45} {ny} Z")
    return f'<g fill="#0A1030" opacity="{op}"><ellipse cx="{cx}" cy="{hcy}" rx="{hw}" ry="{hr}"/><path d="{d}"/></g>'


def marks():
    o = []
    for m in (1.0, 1.5, 2.0):
        y = h(m)
        o.append(f'<line x1="0" y1="{y}" x2="{W}" y2="{y}" stroke="#FF3B6B" stroke-width="6" stroke-dasharray="30 20"/>')
        o.append(rr(8, y - 78, 170, 66, 10, "#FF3B6B", 0) + text(BOLD, f"{m:.1f} m".replace(".", ","), 50, 93, y - 28, fill="#fff", anchor="middle"))
    return "".join(o)


def crop(cx, top, bottom, label):
    hh = bottom - top
    w = hh * 9 / 16
    x = cx - w / 2
    lw = BOLD.width(label, 52) + 40
    return (f'<rect x="{x:.0f}" y="{top}" width="{w:.0f}" height="{hh}" fill="none" stroke="#FF3B6B" stroke-width="14" stroke-dasharray="50 26"/>'
            f'<rect x="{x:.0f}" y="{bottom - 92}" width="{lw:.0f}" height="92" fill="#FF3B6B"/>'
            + text(BOLD, label, 52, x + 20, bottom - 28, fill="#fff"))


def main():
    os.makedirs(OUT, exist_ok=True)
    defs, body = panel()
    t = "Painel de fotos Mundo SENAI Telêmaco Borba 2026 (3,00 x 2,50 m)"
    open(os.path.join(OUT, "painel.svg"), "w").write(svg_doc(W, H, body, defs, t))
    grp = person(1150, 1.62) + person(1500, 1.74) + person(1850, 1.83)
    open(os.path.join(OUT, "sim_grupo.svg"), "w").write(svg_doc(W, H, body + grp + marks(), defs, t, "1500px", "1250px"))
    solo = person(1500, 1.72) + crop(1500, 20, h(0.42), "foto vertical (celular em pé)")
    open(os.path.join(OUT, "sim_vertical.svg"), "w").write(svg_doc(W, H, body + solo, defs, t, "1500px", "1250px"))


if __name__ == "__main__":
    main()
