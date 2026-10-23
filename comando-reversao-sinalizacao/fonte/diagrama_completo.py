"""Redesenho do diagrama da foto (reversão com botão único e pausa temporizada) + sinalização.

Preto/vermelho = igual à foto. Azul = acrescentado (LED 1/2, LED 3/4, LED 5/6, buzzer 7/8).
Uso: python3 diagrama_completo.py <saida.svg>
"""
import sys
from lib import Font

OUT = sys.argv[1]
W, H = 2500, 1770
F = Font("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")
FB = Font("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf")

OLD = "#8A2424"   # fiação existente (como na foto)
NEW = "#1565C0"   # acrescentado
INK = "#111111"
o = []


# ------------------------------------------------------------------ primitivas
def txt(s, size, x, y, color=INK, anchor="start", bold=False):
    f = FB if bold else F
    o.append(f'<path d="{f.d(s, size, x, y, anchor)}" fill="{color}"/>')


def wire(pts, c=OLD, w=3):
    d = "M" + " L".join(f"{x} {y}" for x, y in pts)
    o.append(f'<path d="{d}" fill="none" stroke="{c}" stroke-width="{w}" stroke-linejoin="miter"/>')


def seg(x1, y1, x2, y2, c=INK, w=3, dash=None):
    da = f' stroke-dasharray="{dash}"' if dash else ""
    o.append(f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{c}" stroke-width="{w}" stroke-linecap="round"{da}/>')


def dot(x, y, c=OLD):
    o.append(f'<circle cx="{x}" cy="{y}" r="6" fill="{c}"/>')


def term(x, y, label=None, c=INK, lc=INK, side="right", size=19):
    o.append(f'<circle cx="{x}" cy="{y}" r="6.5" fill="#fff" stroke="{c}" stroke-width="2.4"/>')
    seg(x - 5, y + 5, x + 5, y - 5, c, 2)
    if label:
        dx = 13 if side == "right" else -13
        txt(label, size, x + dx, y + 7, lc, "start" if side == "right" else "end", bold=(lc == NEW))


def contact(x, y1, y2, tag, a, b, nc=False, c=INK, lc=INK, timed=False, push=False):
    """contato vertical entre os terminais (x,y1) e (x,y2)"""
    L = 28
    term(x, y1, a, c, lc)
    term(x, y2, b, c, lc)
    seg(x, y1 + 7, x, y1 + L, c)
    seg(x, y2 - 7, x, y2 - L, c)
    if nc:
        seg(x, y1 + L, x + 16, y1 + L, c)
        bx, by = x + 22, y1 + L - 6
    else:
        bx, by = x - 24, y1 + L + 2
    seg(x, y2 - L, bx, by, c)
    mx, my = (x + bx) / 2, (y2 - L + by) / 2
    if timed:
        seg(mx, my, mx - 40, my, c, 2.4, "7 5")
        o.append(f'<path d="M{mx - 40} {my - 13} A 13 13 0 0 0 {mx - 40} {my + 13}" fill="none" stroke="{c}" stroke-width="2.6"/>')
    if push:
        seg(mx, my, mx - 34, my, c, 2.4, "7 5")
        wire([(mx - 30, my - 12), (mx - 42, my - 12), (mx - 42, my + 12), (mx - 30, my + 12)], c, 2.6)
    txt(tag, 21, x - (64 if (timed or push) else 34), (y1 + y2) / 2 + 7, lc, "end", bold=(lc == NEW))


def coil(x, y1, y2, tag, timer=False):
    cy = (y1 + y2) / 2
    term(x, y1, "A1")
    term(x, y2, "A2")
    seg(x, y1 + 7, x, cy - 22)
    seg(x, y2 - 7, x, cy + 22)
    o.append(f'<rect x="{x - 30}" y="{cy - 22}" width="60" height="44" fill="#fff" stroke="{INK}" stroke-width="3"/>')
    if timer:
        o.append(f'<rect x="{x - 58}" y="{cy - 22}" width="28" height="44" fill="#fff" stroke="{INK}" stroke-width="3"/>')
        seg(x - 58, cy - 22, x - 30, cy + 22, INK, 2.4)
        seg(x - 58, cy + 22, x - 30, cy - 22, INK, 2.4)
    txt(tag, 21, x - (68 if timer else 40), cy + 7, INK, "end")


def lamp(x, y1, y2, tag, a, b, sub):
    cy = (y1 + y2) / 2
    term(x, y1, a, NEW, NEW)
    term(x, y2, b, NEW, NEW)
    seg(x, y1 + 7, x, cy - 26, NEW)
    seg(x, y2 - 7, x, cy + 26, NEW)
    o.append(f'<circle cx="{x}" cy="{cy}" r="26" fill="#fff" stroke="{NEW}" stroke-width="3"/>')
    seg(x - 18, cy - 18, x + 18, cy + 18, NEW, 3)
    seg(x - 18, cy + 18, x + 18, cy - 18, NEW, 3)
    txt(tag, 21, x - 36, cy + 1, NEW, "end", bold=True)
    txt(sub, 16, x - 36, cy + 22, NEW, "end")


def buzzer(x, y1, y2, tag, a, b, sub):
    cy = (y1 + y2) / 2
    term(x, y1, a, NEW, NEW)
    term(x, y2, b, NEW, NEW)
    seg(x, y1 + 7, x, cy - 24, NEW)
    seg(x, y2 - 7, x, cy + 14, NEW)
    o.append(f'<path d="M{x - 26} {cy + 14} L{x + 26} {cy + 14} L{x + 26} {cy + 6} A 26 26 0 0 0 {x - 26} {cy + 6} Z" fill="#fff" stroke="{NEW}" stroke-width="3"/>')
    txt(tag, 21, x - 36, cy + 1, NEW, "end", bold=True)
    txt(sub, 16, x - 36, cy + 22, NEW, "end")


def breaker(xs, y_in, y_out):
    """disjuntor-motor (térmico + magnético) com manopla, como na foto"""
    for x in xs:
        term(x, y_in)
        seg(x, y_in + 7, x, y_in + 22)
        seg(x, y_in + 82, x - 22, y_in + 30)
        seg(x - 14, y_in + 44, x - 4, y_in + 44)
        seg(x, y_in + 82, x, y_in + 96)
        term(x, y_out)
    x0 = xs[0] - 26
    x1 = xs[-1] + 26
    by = y_in + 96
    o.append(f'<rect x="{x0}" y="{by}" width="{x1 - x0}" height="56" fill="#fff" stroke="{INK}" stroke-width="3"/>')
    seg(x0, by + 28, x1, by + 28, INK, 2)
    for x in xs:
        seg(x - 26, by, x - 26, by + 56, INK, 2)
        wire([(x - 12, by + 22), (x - 12, by + 8), (x + 2, by + 8), (x + 2, by + 22)], INK, 2.4)
        txt("I>", 15, x - 12, by + 49, INK)
        seg(x, by + 56, x, y_out - 7)
    # manopla rotativa e ligação mecânica
    hx, hy = xs[0] - 70, y_in + 44
    o.append(f'<rect x="{hx - 24}" y="{hy - 18}" width="36" height="36" fill="#fff" stroke="{INK}" stroke-width="2.6"/>')
    seg(hx - 6, hy - 18, hx - 6, hy + 18, INK, 2.2)
    seg(hx - 24, hy, hx + 12, hy, INK, 2.2)
    seg(hx - 36, hy - 18, hx - 36, hy + 18, INK, 2.6)
    seg(hx - 36, hy, hx - 24, hy, INK, 2.6)
    seg(hx + 12, hy, xs[0] - 12, hy, INK, 2.2, "7 5")
    seg(hx - 6, hy + 18, hx - 6, by + 28, INK, 2.2, "7 5")
    seg(hx - 6, by + 14, x0, by + 14, INK, 2.2, "7 5")
    seg(hx - 6, by + 42, x0, by + 42, INK, 2.2, "7 5")


def x_terms(xs, names, y_lab, y1, y2, show_tag=True):
    for x, n in zip(xs, names):
        txt(n, 19, x, y_lab, INK, "middle")
        o.append(f'<circle cx="{x}" cy="{y1}" r="7" fill="#fff" stroke="{INK}" stroke-width="2.4"/>')
        term(x, y2)
        seg(x, y1 + 7, x, y2 - 7)
    if show_tag:
        txt("-X", 19, xs[0] - 14, y1 + 7, INK, "end")


def power_contacts(xs, y1, y2, tag, top, bot):
    for x, a, b in zip(xs, top, bot):
        term(x, y1)
        term(x, y2)
        txt(a, 17, x - 10, y1 + 6, INK, "end")
        txt(b, 17, x - 10, y2 + 6, INK, "end")
        seg(x, y1 + 7, x, y1 + 22)
        seg(x, y2 - 7, x, y2 - 22)
        seg(x, y2 - 22, x - 22, y1 + 24)
        seg(x - 14, y1 + 22, x - 2, y1 + 22)
    txt(tag, 21, xs[0] - 50, (y1 + y2) / 2 + 7, INK, "end")


# ------------------------------------------------------------------ coordenadas
XL, XM, XR = 110, 250, 310                 # trilho esquerdo, vertical principal, subida p/ barramento
XS1, XB1, XB2 = 400, 540, 650
XK2, XBZ, XLOOP = 790, 915, 1010
XC1, XC2, XD, XE = 1110, 1230, 1395, 1540
XH1, XH2, XH3 = 1685, 1810, 1935
BUSA, BUSB = 540, 580
R1A, R1B, J1Y = 620, 720, 760              # fileira 1 de contatos e junção
R2A, R2B, J2Y = 800, 900, 940              # fileira 2
LOOPY, R3A, R3B, NXY, NYY = 965, 1000, 1100, 1140, 1180
CA1, CA2, BOT = 1210, 1310, 1350

# ------------------------------------------------------------------ COMANDO (igual à foto)
x_terms([200, 250, 300], ["L1", "L2", "L3"], 62, 88, 122)
wire([(200, 129), (200, 193)])
wire([(250, 129), (250, 193)])
breaker([200, 250], 200, 360)
wire([(200, 367), (200, 420), (XL, 420), (XL, BOT), (XE, BOT)])
wire([(250, 367), (250, 423)])
contact(XM, 430, 530, "-S0", "9", "10", nc=True, push=True)
wire([(XM, 537), (XM, R1A - 30), (XM, J1Y)])
dot(XM, 590)
wire([(XM, 590), (XR, 590), (XR, BUSA), (XE, BUSA)])
dot(XM, J1Y)
# S1 e barramento B (só tem tensão com o S1 apertado)
contact(XS1, R1A, R1B, "-S1", "15", "16", push=True)
wire([(XS1, R1A - 7), (XS1, BUSB), (XC2, BUSB), (XC2, R1A - 7)])
wire([(XS1, R1B + 7), (XS1, J1Y), (XM, J1Y)])
dot(XS1, J1Y)
# ramal do K1
contact(XM, R2A, R2B, "-Ka1", "23", "24")
contact(XS1, R2A, R2B, "-K1", "53", "54")
wire([(XM, J1Y), (XM, R2A - 7)])
wire([(XS1, J1Y), (XS1, R2A - 7)])
wire([(XS1, R2B + 7), (XS1, J2Y), (XM, J2Y)])
wire([(XM, R2B + 7), (XM, R3A - 7)])
dot(XM, J2Y)
contact(XM, R3A, R3B, "-Ka2", "61", "62", nc=True)
wire([(XM, R3B + 7), (XM, CA1 - 7)])
coil(XM, CA1, CA2, "-K1")
wire([(XM, CA2 + 7), (XM, BOT)])
dot(XM, BOT)
# ramal do Ka1
dot(XB1, BUSB)
contact(XB1, R1A, R1B, "-K1", "61", "62", nc=True)
wire([(XB1, BUSB), (XB1, R1A - 7)])
dot(XB2, BUSB)
contact(XB2, R1A, R1B, "-Ka1", "73", "74")
wire([(XB2, BUSB), (XB2, R1A - 7)])
wire([(XB2, R1B + 7), (XB2, J1Y), (XB1, J1Y)])
wire([(XB1, R1B + 7), (XB1, R2A - 7)])
dot(XB1, J1Y)
contact(XB1, R2A, R2B, "-Ka2", "71", "72", nc=True)
wire([(XB1, R2B + 7), (XB1, CA1 - 7)])
coil(XB1, CA1, CA2, "-Ka1")
wire([(XB1, CA2 + 7), (XB1, BOT)])
dot(XB1, BOT)
# ramal do Ka2
dot(XC1, BUSB)
contact(XC1, R1A, R1B, "-K1", "73", "74")
wire([(XC1, BUSB), (XC1, R1A - 7)])
contact(XC2, R1A, R1B, "-Ka2", "53", "54")
wire([(XC1, R1B + 7), (XC1, J1Y), (XC2, J1Y)])
wire([(XC2, R1B + 7), (XC2, R3A - 7)])
dot(XC2, J1Y)
contact(XC2, R3A, R3B, "-Ka1", "51", "52", nc=True)
wire([(XC2, R3B + 7), (XC2, CA1 - 7)])
dot(XC2, NXY)
coil(XC2, CA1, CA2, "-Ka2")
wire([(XC2, CA2 + 7), (XC2, BOT)])
dot(XC2, BOT)
# Kt1 (agora comutador 15-16-18) e K2
wire([(XK2, R3A - 7), (XK2, LOOPY), (XLOOP, LOOPY), (XLOOP, NXY), (XD, NXY), (XD, R3B + 7)])
term(XK2, R3A, "15")
term(XK2, R3B, "18", side="left")
seg(XK2, R3A + 7, XK2, R3A + 26)
seg(XK2, R3B - 7, XK2, R3B - 30)
seg(XK2, R3A + 26, XK2 + 34, R3B - 36)                    # lâmina em repouso no NF (16)
o.append(f'<circle cx="{XK2}" cy="{R3A + 26}" r="3" fill="{INK}"/>')
X16 = XK2 + 50
term(X16, R3B, "16", NEW, NEW)
seg(X16, R3B - 7, X16, R3B - 30, NEW)
seg(X16, R3B - 30, XK2 + 30, R3B - 30, NEW)
mx, my = XK2 + 17, R3A + 47
seg(mx, my, mx - 54, my, INK, 2.4, "7 5")
o.append(f'<path d="M{mx - 54} {my - 13} A 13 13 0 0 0 {mx - 54} {my + 13}" fill="none" stroke="{INK}" stroke-width="2.6"/>')
txt("-Kt1", 21, mx - 74, my + 7, INK, "end")
wire([(XK2, R3B + 7), (XK2, CA1 - 7)])
coil(XK2, CA1, CA2, "-K2")
wire([(XK2, CA2 + 7), (XK2, BOT)])
dot(XK2, BOT)
# Ka2 83-84 e Ka3 / Kt1
contact(XD, R3A, R3B, "-Ka2", "83", "84")
wire([(XD, R3A - 7), (XD, LOOPY), (XE, LOOPY)])
dot(XE, LOOPY)
dot(XE, BUSA)
contact(XE, R1A, R1B, "-Ka3", "13", "14")
wire([(XE, BUSA), (XE, R1A - 7)])
wire([(XE, R1B + 7), (XE, CA1 - 7)])
dot(XE, NYY)
wire([(XE, NYY), (XD, NYY), (XD, CA1 - 7)])
coil(XD, CA1, CA2, "-Kt1", timer=True)
coil(XE, CA1, CA2, "-Ka3")
wire([(XD, CA2 + 7), (XD, BOT)])
dot(XD, BOT)
wire([(XE, CA2 + 7), (XE, BOT)])
dot(XE, BOT)

# ------------------------------------------------------------------ SINALIZAÇÃO (acrescentado)
# buzzer 7/8: pega o 16 do Kt1 (NF temporizado; o comum 15 já está no nó do Ka2)
wire([(X16, R3B + 7), (X16, R3B + 20), (XBZ, R3B + 20), (XBZ, CA1 - 7)], NEW)
buzzer(XBZ, CA1, CA2, "-H4", "7", "8", "buzzer")
wire([(XBZ, CA2 + 7), (XBZ, BOT)], NEW)
dot(XBZ, BOT, NEW)
# barramentos estendidos
wire([(XE, BUSA), (XH3, BUSA), (XH3, R1A - 7)], NEW)
wire([(XE, BOT), (XH3, BOT), (XH3, CA2 + 7)], NEW)
# LED 1/2: pronto / parado
dot(XH1, BUSA, NEW)
contact(XH1, R1A, R1B, "-K1", "21", "22", nc=True, c=NEW, lc=NEW)
wire([(XH1, BUSA), (XH1, R1A - 7)], NEW)
wire([(XH1, R1B + 7), (XH1, R2A - 7)], NEW)
contact(XH1, R2A, R2B, "-Ka3", "21", "22", nc=True, c=NEW, lc=NEW)
wire([(XH1, R2B + 7), (XH1, CA1 - 7)], NEW)
lamp(XH1, CA1, CA2, "-H1", "1", "2", "LED")
wire([(XH1, CA2 + 7), (XH1, BOT)], NEW)
dot(XH1, BOT, NEW)
# LED 3/4: sentido 2 (K2)
dot(XH2, BUSA, NEW)
contact(XH2, R1A, R1B, "-K2", "13", "14", c=NEW, lc=NEW)
wire([(XH2, BUSA), (XH2, R1A - 7)], NEW)
wire([(XH2, R1B + 7), (XH2, CA1 - 7)], NEW)
lamp(XH2, CA1, CA2, "-H2", "3", "4", "LED")
wire([(XH2, CA2 + 7), (XH2, BOT)], NEW)
dot(XH2, BOT, NEW)
# LED 5/6: sentido 1 (K1)
contact(XH3, R1A, R1B, "-K1", "83", "84", c=NEW, lc=NEW)
wire([(XH3, R1B + 7), (XH3, CA1 - 7)], NEW)
lamp(XH3, CA1, CA2, "-H3", "5", "6", "LED")

# função de cada sinal (embaixo das lâmpadas)
for x, a in ((XBZ, "pausa"), (XH1, "pronto"), (XH2, "sentido 2"), (XH3, "sentido 1")):
    txt(a, 17, x, BOT + 32, NEW, "middle", bold=True)

# ------------------------------------------------------------------ FORÇA (igual à foto)
P = [2090, 2140, 2190]
K2X = [2340, 2390, 2440]
x_terms(P, ["L1", "L2", "L3"], 62, 88, 122)
for x in P:
    wire([(x, 129), (x, 193)])
breaker(P, 200, 360)
for x, ty, kx in zip(P[::-1], (420, 455, 490), K2X[::-1]):
    dot(x, ty)
    wire([(x, ty), (kx, ty), (kx, 563)])
for x in P:
    wire([(x, 367), (x, 563)])
power_contacts(P, 570, 650, "-K1", ["L1", "L2", "L3"], ["T1", "T2", "T3"])
power_contacts(K2X, 570, 650, "-K2", ["L1", "L2", "L3"], ["T1", "T2", "T3"])
# K2 com inversão de fases (T1 -> fase 3, T3 -> fase 1)
for kx, ty, px in zip(K2X, (700, 740, 780), P[::-1]):
    wire([(kx, 657), (kx, ty), (px, ty)])
    dot(px, ty)
for x in P:
    wire([(x, 657), (x, 1103)])
    term(x, 1110)
MX, MY = 2155, 1235
PEX = MX + 120
term(PEX, 1110)
for x in P:
    seg(x, 1117, x, MY - 92 + abs(x - MX) * 0.35, OLD)
o.append(f'<circle cx="{MX}" cy="{MY}" r="95" fill="#fff" stroke="{INK}" stroke-width="3"/>')
txt("M", 26, MX, MY - 14, INK, "middle")
txt("3", 22, MX - 26, MY + 30, INK, "middle")
o.append(f'<path d="M{MX + 2} {MY + 22} q 12 -18 24 0 t 24 0" fill="none" stroke="{INK}" stroke-width="3"/>')
txt("-M", 21, MX - 108, MY + 7, INK, "end")
wire([(PEX, 1117), (PEX, MY + 10), (MX + 106, MY + 10)])
seg(MX + 106, MY - 8, MX + 106, MY + 28, INK, 3)
seg(MX + 98, MY - 4, MX + 98, MY + 24, INK, 3)

# ------------------------------------------------------------------ título, sequência e notas
o.append(f'<rect x="40" y="1405" width="{W - 80}" height="{H - 1430}" fill="none" stroke="{INK}" stroke-width="2.5"/>')
txt("Reversão com botão único e pausa temporizada + sinalização", 30, 70, 1450, INK, bold=True)
txt("Diagrama da foto redesenhado sem alterar a lógica. Em azul: o que foi acrescentado.", 21, 70, 1484, "#333")
o.append(f'<rect x="1010" y="1477" width="26" height="6" fill="{NEW}"/>')
txt("acrescentado", 19, 1044, 1486, NEW, bold=True)
o.append(f'<rect x="1200" y="1477" width="26" height="6" fill="{OLD}"/>')
txt("existente", 19, 1234, 1486, OLD, bold=True)

# tabela de sequência
cols = [("Situação", 330), ("Contatores ligados", 290), ("LED 1/2", 120), ("LED 5/6", 120), ("Buzzer 7/8", 140), ("LED 3/4", 120)]
rows = [("Painel energizado", "nenhum", 1, 0, 0, 0),
        ("1º toque no S1", "K1 (sentido 1)", 0, 1, 0, 0),
        ("2º toque no S1 (pausa)", "Ka2, Ka3, Kt1", 0, 0, 1, 0),
        ("Fim do tempo do Kt1", "Ka2, Ka3, Kt1, K2", 0, 0, 0, 1),
        ("Apertou e soltou o S0", "nenhum", 1, 0, 0, 0)]
tx, ty, rh = 70, 1508, 34
x = tx
for name, w in cols:
    o.append(f'<rect x="{x}" y="{ty}" width="{w}" height="{rh}" fill="#E8EEF7" stroke="{INK}" stroke-width="1.5"/>')
    txt(name, 18, x + 10, ty + 23, INK, bold=True)
    x += w
for i, r in enumerate(rows):
    y = ty + rh * (i + 1)
    x = tx
    for j, (name, w) in enumerate(cols):
        o.append(f'<rect x="{x}" y="{y}" width="{w}" height="{rh}" fill="#fff" stroke="{INK}" stroke-width="1.5"/>')
        v = r[j]
        if j < 2:
            txt(v, 18, x + 10, y + 23, INK)
        elif v:
            on = "toca" if j == 4 else "acesa"
            o.append(f'<rect x="{x + 6}" y="{y + 5}" width="{w - 12}" height="{rh - 10}" rx="5" fill="{NEW}"/>')
            txt(on, 17, x + w / 2, y + 23, "#fff", "middle", bold=True)
        else:
            txt("–", 18, x + w / 2, y + 23, "#999", "middle")
        x += w

# notas
nx = 1250
notes = [
    ("Contatos novos (confira se estão livres nos seus componentes):", True),
    ("K1 21-22 (NF) e 83-84 (NA) · K2 13-14 (NA) · Ka3 21-22 (NF) · Kt1 16 (NF temporizado).", False),
    ("Se o número do contato livre for outro, troque só o número.", False),
    ("O Kt1 precisa ter contato comutador 15-16-18. O 15 já está ligado; só sai um fio do 16.", False),
    ("Não ligue nada em paralelo com o 16 do Kt1: a corrente volta pelo 15 e liga o Ka2 sozinho.", False),
    ("LEDs e buzzer na tensão do comando (entre L1 e L2 = tensão de linha da rede).", False),
    ("Contato do K1 no ramal do K1 lido como 53/54 (na foto estava rasurado).", False),
    ("Recomendado (não incluído): intertravamento direto K1/K2 com contatos NF.", False),
]
for i, (s, b) in enumerate(notes):
    txt(s, 18.5, nx, 1530 + i * 24, INK if b else "#333", bold=b)

open(OUT, "w").write(
    f'<svg xmlns="http://www.w3.org/2000/svg" width="420mm" height="297.4mm" viewBox="0 0 {W} {H}">'
    f'<title>Reversão com sinalização</title><rect width="{W}" height="{H}" fill="#fff"/>{"".join(o)}</svg>')
