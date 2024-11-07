from PIL import Image
import os

# os.chdir("./docs")

fov60_paths = [
    "pinhole60.png",
    "thinlens60.png",
    "panini60.png",
    "fisheye60.png"
]

fov90_paths = [
    "pinhole90.png",
    "thinlens90.png",
    "panini90.png",
    "fisheye90.png"
]

orth_compare_paths = ["pinhole_fov90.png", "orthographic_6.png"]

def combine2x2(image_paths: list[str], outpath: str):
    # Open images
    images = [Image.open(img) for img in image_paths]

    # Calculate the size of the 2x2 grid
    max_width = max(img.width for img in images)
    max_height = max(img.height for img in images)

    # Create a new blank image with the calculated size
    combined_image = Image.new("RGB", (max_width * 2, max_height * 2))

    # Paste images into the 2x2 grid
    positions = [(0, 0), (max_width, 0), (0, max_height), (max_width, max_height)]

    for img, pos in zip(images, positions):
        combined_image.paste(img, pos)

    # Save the combined image
    combined_image.save(outpath)

def combine1x3(image_paths: list[str], outpath: str):
    # Open images
    images = [Image.open(img) for img in image_paths]

    # Calculate total width and max height for a 1x3 grid
    total_width = sum(img.width for img in images)
    max_height = max(img.height for img in images)

    # Create a new blank image with the calculated size
    combined_image = Image.new("RGB", (total_width, max_height))

    # Paste images side-by-side into the new image
    x_offset = 0
    for img in images:
        combined_image.paste(img, (x_offset, 0))
        x_offset += img.width

    # Save the combined image
    combined_image.save(outpath)

def combine1x2(image_paths: list[str], outpath: str):
    # Open images
    images = [Image.open(img) for img in image_paths]

    # Calculate total width and max height for a 1x3 grid
    total_width = sum(img.width for img in images)
    max_height = max(img.height for img in images)

    # Create a new blank image with the calculated size
    combined_image = Image.new("RGB", (total_width, max_height))

    # Paste images side-by-side into the new image
    x_offset = 0
    for img in images:
        combined_image.paste(img, (x_offset, 0))
        x_offset += img.width

    # Save the combined image
    combined_image.save(outpath)

if __name__ == "__main__":
    combine2x2(fov60_paths, "fov60compare.png")
    combine2x2(fov90_paths, "fov90compare.png")
    combine1x2(orth_compare_paths, "orthographic_compare.png")
