from PIL import Image
import numpy as np
import sys

def rgb888_to_rgb565(r, g, b):
    """Convert from RGB888 to RGB565."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def convert_image_to_rgb565_a8_header(png_filepath, header_filepath):
    """Convert a PNG image to RGB565 with Alpha format and save as a C header file in uint8_t array format."""
    img = Image.open(png_filepath).convert('RGBA')
    pixels = np.array(img)
    height, width, _ = pixels.shape

    with open(header_filepath, 'w') as header_file:
        header_file.write("#ifndef IMAGE_H\n")
        header_file.write("#define IMAGE_H\n\n")
        header_file.write("#include <stdint.h>\n\n")
        header_file.write(f"static const uint32_t image_width = {width};\n")
        header_file.write(f"static const uint32_t image_height = {height};\n")
        header_file.write("static const uint8_t image_data[] = {\n")

        for y in range(height):
            for x in range(width):
                r, g, b, a = pixels[y, x]
                rgb565 = rgb888_to_rgb565(r, g, b)
                # Write RGB565 in two parts (high byte, low byte) and then Alpha
                header_file.write(f"    0x{rgb565 >> 8:02X}, 0x{rgb565 & 0xFF:02X}, 0x{a:02X},")
            header_file.write("\n")

        header_file.write("};\n\n")
        header_file.write("#endif // IMAGE_H\n")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python script.py input.png output_image.h")
        sys.exit(1)

    png_filepath = sys.argv[1]
    header_filepath = sys.argv[2]
    convert_image_to_rgb565_a8_header(png_filepath, header_filepath)
