import numpy as np
import sys

if len(sys.argv) < 3:
    print("Uso: python convert_raw_to_h.py archivo.raw salida.h")
    sys.exit(1)

raw_file = sys.argv[1]
output_file = sys.argv[2]

pcm = np.fromfile(raw_file, dtype=np.int16)

with open(output_file, "w") as f:
    f.write("const int16_t sample[] = {\n")
    for i, val in enumerate(pcm):
        f.write(f"{val}, ")
        if (i + 1) % 8 == 0:
            f.write("\n")
    f.write("\n};\n")
    f.write(f"const unsigned int sample_len = {len(pcm)};\n")