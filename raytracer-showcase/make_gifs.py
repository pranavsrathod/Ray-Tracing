import os
from PIL import Image, ImageDraw, ImageFont

base_dir = "images"

stages = [
    ("01-flat-shading", "Flat"),
    ("02-sphere-shading", "Sphere"),
    ("03-triangle-shading", "Triangle"),
    ("04-shadows", "Shadows"),
    ("05-antialiasing", "AA"),
    ("06-recursive-reflections/01", "R1"),
    ("06-recursive-reflections/02", "R2"),
    ("06-recursive-reflections/03", "R3"),
]

scenes = [
    "SIGGRAPH",
    "snow",
    "spheres",
    "table",
    "test1",
    "test2",
    "toy"
]

output_dir = "comparisons"
os.makedirs(output_dir, exist_ok=True)

font = ImageFont.load_default()

for scene in scenes:

    images = []
    labels = []

    for folder, label in stages:
        path = os.path.join(base_dir, folder, f"{scene}.jpg")

        if os.path.exists(path):
            img = Image.open(path)
            images.append(img)
            labels.append(label)

    if not images:
        continue

    w, h = images[0].size
    label_height = 30

    # Grid layout
    cols = 4
    rows = (len(images) + cols - 1) // cols

    canvas = Image.new(
        "RGB",
        (w * cols, rows * (h + label_height)),
        "black"
    )

    draw = ImageDraw.Draw(canvas)

    for i, img in enumerate(images):
        row = i // cols
        col = i % cols

        x = col * w
        y = row * (h + label_height)

        canvas.paste(img, (x, y))

        text = labels[i]
        text_width = draw.textlength(text, font=font)

        draw.text(
            (x + (w - text_width) / 2, y + h + 5),
            text,
            fill="white",
            font=font
        )

    out_path = os.path.join(output_dir, f"{scene}_comparison.jpg")
    canvas.save(out_path, quality=95)

    print("Created", out_path)