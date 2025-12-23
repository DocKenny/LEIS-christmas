import tkinter as tk
from tkinter import filedialog
from PIL import Image
import numpy as np


def load_images_to_packet(brightness=50, fps=20, chunk=0, is_last_packet=False):
    # Hide root window
    root = tk.Tk()
    root.withdraw()

    # Open file explorer (multiple selection)
    file_paths = filedialog.askopenfilenames(
        title="Select images",
        filetypes=[("Image files", "*.png *.jpg *.jpeg *.bmp")]
    )

    images_array = []

    for path in file_paths:
        img = Image.open(path).convert("RGB")
        img_array = np.array(img)

        # Convert to Python list: [[[R,G,B], ...], ...]
        images_array.append(img_array.tolist())

    packet = {
        "brightness": brightness,
        "fps": fps,
        "isLastPacket": is_last_packet,
        "chunk": chunk,
        "images": images_array
    }

    return packet

if __name__ == "__main__":
    packet = load_images_to_packet(
    brightness=50,
    fps=1,
    chunk=0,
    is_last_packet=False
    )
    with open("image_packet.json", "w") as f:
        import json
        json.dump(packet, f)

    print(packet)
