"""Render a font/material-palette study; this is not a game screenshot."""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


def main() -> None:
    directory = Path(__file__).resolve().parent
    rajdhani = directory / "rajdhani" / "Rajdhani-SemiBold.ttf"
    source = directory / "source-sans" / "SourceSans3-Regular.ttf"
    image = Image.new("RGB", (1440, 900), (6, 11, 17))
    draw = ImageDraw.Draw(image)
    silver, cyan, muted = (202, 219, 232), (56, 217, 240), (133, 163, 184)
    draw.line((60, 72, 1380, 72), fill=cyan, width=2)
    draw.text((60, 95), "CRUCIBLE", font=ImageFont.truetype(str(rajdhani), 136), fill=silver)
    draw.text((65, 275), "TYPOGRAPHY STUDY / ORIGINAL LICENSED FONTS",
              font=ImageFont.truetype(str(source), 26), fill=cyan)
    draw.rounded_rectangle((60, 345, 650, 810), radius=12, fill=(9, 18, 25), outline=(54, 82, 99), width=2)
    draw.text((90, 372), "Operations", font=ImageFont.truetype(str(rajdhani), 64), fill=silver)
    draw.text((92, 470), "MISSION-BOUNDARY PROGRESS", font=ImageFont.truetype(str(source), 25), fill=cyan)
    draw.rounded_rectangle((90, 530, 620, 620), radius=6, fill=(19, 36, 46), outline=cyan, width=2)
    draw.text((115, 548), "Continue", font=ImageFont.truetype(str(rajdhani), 48), fill=silver)
    draw.text((90, 665), "Reclaim the front", font=ImageFont.truetype(str(rajdhani), 42), fill=silver)
    draw.text((90, 725), "Recover biomass before the tick deadline.",
              font=ImageFont.truetype(str(source), 27), fill=muted)
    draw.text((740, 365), "Rajdhani SemiBold", font=ImageFont.truetype(str(rajdhani), 44), fill=silver)
    draw.text((740, 432), "Source Sans 3 Regular", font=ImageFont.truetype(str(source), 36), fill=silver)
    draw.text((740, 510), "Readable numeric objectives", font=ImageFont.truetype(str(source), 30), fill=muted)
    draw.text((740, 566), "1780 biomass / 900 ticks", font=ImageFont.truetype(str(rajdhani), 44), fill=cyan)
    draw.text((740, 648), "Silver  /  Cyan  /  Amber  /  Failure", font=ImageFont.truetype(str(source), 27), fill=silver)
    for index, color in enumerate((silver, cyan, (250, 176, 82), (250, 99, 148))):
        draw.rectangle((740 + index * 150, 710, 860 + index * 150, 755), fill=color)
    draw.text((60, 847), "Static font study. Functional UI is authored in ImGui; native Filament receiving remains pending.",
              font=ImageFont.truetype(str(source), 24), fill=muted)
    image.save(directory / "font-specimen.png")


if __name__ == "__main__":
    main()
