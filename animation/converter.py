import numpy as np
import cv2
import glob

WIDTH = 44
HEIGHT = 2 * int(0.43 * WIDTH)

POS = "\033[{};{}H"
FG = "\033[38;2;{};{};{}m"
BG = "\033[48;2;{};{};{}m"
CR = "\033[0m"
HB = "▄"

is_transparent = lambda clr1, clr2: clr1[3] == 0 and clr2[3] == 0

def bgr_to_console(clr):
    result = np.array((0, 0, 0))
    alpha = clr[3] / 255.0
    result = (alpha * clr[:3] + (1 - alpha) * result).astype(np.uint8)
    return result[::-1]

def convert_frame(frame_path):
    image = cv2.imread(frame_path, cv2.IMREAD_UNCHANGED)
    image = cv2.resize(image, dsize=(WIDTH, HEIGHT), interpolation=cv2.INTER_CUBIC)
    result = ""
    lc1, lc2 = None, None
    for y in range(0, HEIGHT, 2):
        result += POS.format(y // 2 + 1, 1)
        for x in range(WIDTH):
            if is_transparent(image[y + 1, x], image[y, x]):
                if lc1 is not None or lc2 is not None:
                    lc1, lc2 = None, None
                    result += CR
                result += " "
                continue
            c1 = bgr_to_console(image[y + 1, x])
            c2 = bgr_to_console(image[y, x])
            if lc1 is None or not (c1 == lc1).all():
                result += FG.format(*c1)
                lc1 = c1
            if lc2 is None or not (c2 == lc2).all():
                result += BG.format(*c2)
                lc2 = c2
            result += HB
    return result


"""
print(convert_frame("./images/frame01.png"))
#"""

#"""
frame_paths = sorted(glob.glob("./images/frame*.png"))
cfile = open("../jorb.c", "w")

cfile.write('#include "jfetch.h"\n\n')
cfile.write('static animation_object jorb = {\n')
cfile.write('    .current_frame = 0,\n')
cfile.write(f'    .frame_count = {len(frame_paths)},\n')
cfile.write('    .frames = {\n')

max_len = 0
for fp in frame_paths:
    frame = repr(convert_frame(fp)).replace("'", '"')
    cfile.write(8 * ' ' + frame + ',\n')
    max_len = max(max_len, len(frame))

cfile.write('    }\n')
cfile.write('};\n')
cfile.close()

print(f"{max_len=}")
#"""
