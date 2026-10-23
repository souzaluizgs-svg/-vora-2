"""Utilitários comuns: texto convertido em curvas (HarfBuzz + fontTools), paleta e primitivas SVG."""
import os
import uharfbuzz as hb
from fontTools.ttLib import TTFont
from fontTools.pens.svgPathPen import SVGPathPen
from fontTools.pens.transformPen import TransformPen

FONT_DIR = os.environ.get("FONT_DIR", os.path.join(os.path.dirname(__file__), "..", "fonts"))


class Font:
    def __init__(self, fname):
        path = os.path.join(FONT_DIR, fname)
        self.tt = TTFont(path)
        self.gs = self.tt.getGlyphSet()
        self.upem = self.tt["head"].unitsPerEm
        os2 = self.tt["OS/2"]
        self.cap = (getattr(os2, "sCapHeight", 0) or 0.72 * self.upem) / self.upem
        self.xh = (getattr(os2, "sxHeight", 0) or 0.53 * self.upem) / self.upem
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

    def fit(self, text, size, maxw, tracking=0):
        """maior tamanho <= size que cabe em maxw"""
        w = self.width(text, size, tracking)
        return size if w <= maxw else size * maxw / w


BLACK = Font("Poppins-Black.ttf")
BLACK_IT = Font("Poppins-BlackItalic.ttf")
XBOLD = Font("Poppins-ExtraBold.ttf")
BOLD = Font("Poppins-Bold.ttf")
SEMI = Font("Poppins-SemiBold.ttf")
MED = Font("Poppins-Medium.ttf")

# paleta (cores de impressão mais seguras que o neon original)
NAVY = "#121C47"
BLUE = "#2C46DB"
BLUE_L = "#4A63EE"
ROYAL = "#3448E0"
LIME = "#D2EE3E"
LIME_D = "#A9C722"
WHITE = "#FFFFFF"
ORANGE = "#FF8A3D"
ORANGE_D = "#E06A1F"
SKIN1, SKIN1_D = "#F4BE98", "#E3A17A"
SKIN2, SKIN2_D = "#9C5E3A", "#814A2C"
HAIR1 = "#4A2A19"
HAIR2 = "#1E1410"
YEL, YEL_L = "#FFC935", "#FFE07A"
STRIPE = "#E9EEF7"
SW = 10


def stroke(w=SW, color=NAVY):
    return f'stroke="{color}" stroke-width="{w}" stroke-linejoin="round" stroke-linecap="round"'


def rr(x, y, w, h, r, fill, sw=SW, extra=""):
    st = stroke(sw) if sw else 'stroke="none"'
    return f'<rect x="{x:.1f}" y="{y:.1f}" width="{w:.1f}" height="{h:.1f}" rx="{r}" fill="{fill}" {st} {extra}/>'


def circ(cx, cy, r, fill, sw=SW, extra=""):
    st = stroke(sw) if sw else 'stroke="none"'
    return f'<circle cx="{cx:.1f}" cy="{cy:.1f}" r="{r:.1f}" fill="{fill}" {st} {extra}/>'


def path(d, fill, sw=SW, extra=""):
    st = stroke(sw) if sw else 'stroke="none"'
    return f'<path d="{d}" fill="{fill}" {st} {extra}/>'


def tube(d, color, w=100, sw=SW):
    """braço/perna: traço grosso com contorno navy"""
    return (f'<path d="{d}" fill="none" stroke="{NAVY}" stroke-width="{w + 2 * sw}" stroke-linecap="round" stroke-linejoin="round"/>'
            f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{w}" stroke-linecap="round" stroke-linejoin="round"/>')


def text(font, s, size, x, y, fill=NAVY, anchor="start", tracking=0, extra=""):
    return f'<path d="{font.d(s, size, x, y, anchor, tracking)}" fill="{fill}" {extra}/>'


def ig_icon(x, y, s, color):
    return (f'<rect x="{x:.0f}" y="{y:.0f}" width="{s:.0f}" height="{s:.0f}" rx="{s * .28:.0f}" fill="none" stroke="{color}" stroke-width="{s * .1:.0f}"/>'
            f'<circle cx="{x + s / 2:.0f}" cy="{y + s / 2:.0f}" r="{s * .22:.0f}" fill="none" stroke="{color}" stroke-width="{s * .1:.0f}"/>'
            f'<circle cx="{x + s * .76:.0f}" cy="{y + s * .24:.0f}" r="{s * .065:.0f}" fill="{color}"/>')


def svg_doc(w, h, body, defs="", title="", w_attr=None, h_attr=None):
    w_attr = w_attr or f"{w}mm"
    h_attr = h_attr or f"{h}mm"
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{w_attr}" height="{h_attr}" viewBox="0 0 {w} {h}">'
            f'<title>{title}</title><defs>{defs}</defs>{body}</svg>')
