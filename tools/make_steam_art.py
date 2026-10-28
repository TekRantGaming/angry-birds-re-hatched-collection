"""Makes the Add to Steam artwork (launcher steam-art/: cover.png 600x900, wide.png 920x430, hero.png 1920x620,
logo.png, icon.png 512x512), and the launcher's Home wordmark (art/logo.png, src/logo_png.h), from Angry Birds Trilogy's SteamGridDB artwork (owner's choice, 28 Oct 2026:
https://www.steamgriddb.com/game/33881), with the collection's name in place of "Trilogy".

usage: python -I tools/make_steam_art.py   (from the launcher folder)

Sources in art/sgdb (downloaded from cdn2.steamgriddb.com, named by their address there):
  logo_204045dad4a10a8689414c7f83f56adb.png  the official Trilogy logo: its gold TRILOGY is replaced by
                                             THE RE-HATCHED COLLECTION in the same gold
  grid_bb9b8f636da01545487dd1da1f916492.png  cover: its logo is painted over with its own sky, the new logo goes on
  hero_f340c2f9d9a79c6243ea596349704822.png  hero (as it is, no text) and the wide capsule's background
  icon_d576556711ce6a6cf8bb14f698ef66d4.png  the icon (the red bird)
They are third party artwork, so they and the result stay out of any repository until the owner decides.
"""
import os
import subprocess
import sys

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, 'art', 'sgdb')
OUT = os.path.join(ROOT, 'art', 'steam')
FONT = r'C:\Windows\Fonts\impact.ttf'
NAME = 'THE RE-HATCHED COLLECTION'


def src(name):
    return Image.open(os.path.join(SRC, name)).convert('RGBA')


def gold_text(text, width):
    """The name in Trilogy's gold: a light yellow to orange gradient, a dark brown outline and a soft shadow."""
    size = 200
    font = ImageFont.truetype(FONT, size)
    while font.getlength(text) > width:
        size -= 2
        font = ImageFont.truetype(FONT, size)
    tw = int(font.getlength(text))
    pad = 30
    w, h = tw + 2 * pad, int(size * 1.35) + 2 * pad
    mask = Image.new('L', (w, h))
    ImageDraw.Draw(mask).text((pad, pad), text, font=font, fill=255)
    outline = Image.new('L', (w, h))
    ImageDraw.Draw(outline).text((pad, pad), text, font=font, fill=255, stroke_width=max(6, size // 14),
                                 stroke_fill=255)
    box = mask.getbbox()
    grad = Image.new('RGBA', (w, h))
    gd = ImageDraw.Draw(grad)
    for y in range(h):
        t = min(max((y - box[1]) / max(1, box[3] - box[1]), 0.0), 1.0)
        top, mid, bottom = (255, 246, 170), (255, 200, 40), (225, 120, 10)
        c = [round(top[i] + (mid[i] - top[i]) * t * 2) if t < 0.5 else round(mid[i] + (bottom[i] - mid[i]) * (t - 0.5) * 2)
             for i in range(3)]
        gd.line([(0, y), (w, y)], fill=(*c, 255))
    layer = Image.new('RGBA', (w, h))
    shadow = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    shadow.paste((0, 0, 0, 140), (0, 0), outline.filter(ImageFilter.GaussianBlur(6)))
    layer.alpha_composite(shadow, (0, 0))
    edge = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    edge.paste((60, 30, 5, 255), (0, 0), outline)
    layer.alpha_composite(edge)
    fill = Image.new('RGBA', (w, h), (0, 0, 0, 0))
    fill.paste(grad, (0, 0), mask)
    layer.alpha_composite(fill)
    return layer.crop(layer.getbbox())


def make_logo():
    logo = src('logo_204045dad4a10a8689414c7f83f56adb.png')  # 1762 x 921
    d = ImageDraw.Draw(logo)
    d.rectangle((480, 585, 1290, 790), fill=(0, 0, 0, 0))  # TRILOGY and the sparkles around it
    name = gold_text(NAME, 1560)
    logo.alpha_composite(name, ((logo.width - name.width) // 2, 600))
    return logo.crop(logo.getbbox())


def fit(img, width):
    return img.resize((width, round(img.height * width / img.width)), Image.LANCZOS)


def cover_to(img, w, h, cx=0.5, cy=0.5):
    scale = max(w / img.width, h / img.height)
    img = img.resize((round(img.width * scale), round(img.height * scale)), Image.LANCZOS)
    x = min(max(round(img.width * cx - w / 2), 0), img.width - w)
    y = min(max(round(img.height * cy - h / 2), 0), img.height - h)
    return img.crop((x, y, x + w, y + h))


def main():
    os.makedirs(OUT, exist_ok=True)
    logo = make_logo()
    logo.save(os.path.join(OUT, 'logo.png'))
    # The launcher's Home wordmark is the same logo (owner, 28 Oct 2026), 1400 wide, compiled in as src/logo_png.h.
    home = os.path.join(ROOT, 'art', 'logo.png')
    fit(logo, 1400).save(home)
    tool = os.path.join(ROOT, 'trg-launcher', 'launcher2', 'tools', 'png_to_header.py')
    subprocess.check_call([sys.executable, '-I', tool, home, os.path.join(ROOT, 'src', 'logo_png.h'), 'kAngryBirdsLogoPng'])

    # Cover: the sky and birds of the Trilogy cover, its logo painted over with the sky above it, the new logo on top.
    cover = src('grid_bb9b8f636da01545487dd1da1f916492.png').resize((600, 900), Image.LANCZOS)
    top = 340  # the old logo, TRILOGY and its sparkles end above this line
    sky = cover.crop((0, 0, 600, 8)).resize((600, 1))  # the sky colour along the top edge
    below = cover.crop((0, top, 600, top + 8)).resize((600, 1))
    fill = Image.new('RGBA', (600, top))
    for y in range(top):  # top edge colour fading into the colour under the old logo
        t = y / (top - 1)
        row = Image.blend(sky, below, t)
        fill.paste(row, (0, y))
    cover.alpha_composite(fill, (0, 0))
    lg = fit(logo, 560)
    cover.alpha_composite(lg, (20, 40))
    cover.convert('RGB').save(os.path.join(OUT, 'cover.png'))

    # Hero: the Trilogy header with the birds in the sky, no text (Steam draws logo.png over it).
    hero = src('hero_f340c2f9d9a79c6243ea596349704822.png')
    hero.convert('RGB').save(os.path.join(OUT, 'hero.png'))

    # Wide capsule: the same birds and sky, the new logo in the open sky in the middle.
    wide = cover_to(hero, 920, 430, 0.5, 0.5)
    lg = fit(logo, 560)
    wide.alpha_composite(lg, ((920 - lg.width) // 2, (430 - lg.height) // 2))
    wide.convert('RGB').save(os.path.join(OUT, 'wide.png'))

    # Icon: the red bird.
    src('icon_d576556711ce6a6cf8bb14f698ef66d4.png').resize((512, 512), Image.LANCZOS).save(os.path.join(OUT, 'icon.png'))
    print('wrote', OUT)


main()
