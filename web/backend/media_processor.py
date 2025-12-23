import math
import cv2
from typing import List, Tuple
from PIL import Image

def gamma_correct(value: int, gamma: float = 2.2) -> int:
    """Apply gamma correction to a single 0-255 value."""
    corrected = 255 * ((value / 255) ** gamma)
    return max(0, min(255, round(corrected)))

def downscale_image(input_path: str, output_path: str = "./downscaled_image.png", gamma: float = 2.2) -> List[Tuple[int, int, int]]:
    # Read the image
    image = cv2.imread(input_path)
    if image is None:
        raise ValueError(f"Could not read the image from {input_path}")
    
    # Downscale the image to 8x8
    downscaled_image = cv2.resize(image, (8, 8), interpolation=cv2.INTER_AREA)
    
    # Convert BGR (OpenCV) to RGB
    rgb_image = cv2.cvtColor(downscaled_image, cv2.COLOR_BGR2RGB)
    
    # Flatten the pixels for FastLED with gamma correction
    rgb_pixels = []
    for row in rgb_image:
        for r, g, b in row:
            rgb_pixels.append([
                gamma_correct(r, gamma),
                gamma_correct(g, gamma),
                gamma_correct(b, gamma)
            ])
    
    # Save a preview image (optional)
    if output_path:
        preview_bgr = cv2.cvtColor(rgb_image, cv2.COLOR_RGB2BGR)
        cv2.imwrite(output_path, preview_bgr)
    
    return rgb_pixels

def downscale_video(input_path: str, frame_limit: int = 20, start_frame: int = 0, gamma: float = 2.2) -> List[List[Tuple[int, int, int]]]:
    cap = cv2.VideoCapture(input_path)
    if not cap.isOpened():
        raise ValueError(f"Could not open the video file {input_path}")
    
    cap.set(cv2.CAP_PROP_POS_FRAMES, start_frame)
    frame_count = 0
    all_rgb_pixels = []
    
    while frame_count < frame_limit:
        ret, frame = cap.read()
        if not ret:
            break
        
        downscaled_frame = cv2.resize(frame, (8, 8), interpolation=cv2.INTER_AREA)
        rgb_frame = cv2.cvtColor(downscaled_frame, cv2.COLOR_BGR2RGB)
        
        rgb_pixels = [
            [gamma_correct(r, gamma), gamma_correct(g, gamma), gamma_correct(b, gamma)]
            for row in rgb_frame
            for r, g, b in row
        ]
        
        all_rgb_pixels.append(rgb_pixels)
        frame_count += 1
    
    cap.release()
    return all_rgb_pixels

def downscale_gif(input_path, gamma=2.2):
    img = Image.open(input_path)
    frames = []
    for frame in range(img.n_frames):
        img.seek(frame)
        frame_rgb = img.convert("RGB").resize((8, 8))
        pixels = [
            [gamma_correct(r, gamma), gamma_correct(g, gamma), gamma_correct(b, gamma)]
            for r, g, b in frame_rgb.getdata()
        ]
        frames.append(pixels)
    return frames

