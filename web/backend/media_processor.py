import cv2
import json
from typing import List, Tuple

def downscale_image(input_path: str, output_path: str = None) -> List[Tuple[int, int, int]]:

    image = cv2.imread(input_path)
    if image is None:
        raise ValueError(f"Could not read the image from {input_path}")
    
    # Downscale the image
    downscaled_image = cv2.resize(image, (8, 8), interpolation=cv2.INTER_AREA)
    hsv = cv2.cvtColor(downscaled_image, cv2.COLOR_BGR2HSV)
    
    hsv_pixels = []
    for row in hsv:
        for pixel in row:
            h, s, v = pixel
            hsv_pixels.append([int(h), int(s), int(v)])
    
    # Save if output path provided
    if output_path:
        cv2.imwrite(output_path, downscaled_image)
    
    return hsv_pixels


def downscale_video(input_path: str, frame_limit: int = 20, 
                    start_frame: int = 0) -> List[List[Tuple[int, int, int]]]:
    
    cap = cv2.VideoCapture(input_path)
    if not cap.isOpened():
        raise ValueError(f"Could not open the video file {input_path}")
    
    cap.set(cv2.CAP_PROP_POS_FRAMES, start_frame) # Start Frame
    frame_count = 0
    all_hsv_pixels = []
    
    while frame_count < frame_limit:
        ret, frame = cap.read()
        if not ret:
            break
        
        downscaled_frame = cv2.resize(frame, (8, 8), interpolation=cv2.INTER_AREA)
        hsv = cv2.cvtColor(downscaled_frame, cv2.COLOR_BGR2HSV)
        
        hsv_pixels = []
        for row in hsv:
            for pixel in row:
                h, s, v = pixel
                hsv_pixels.append([int(h), int(s), int(v)])
        
        all_hsv_pixels.append(hsv_pixels)
        frame_count += 1
    
    cap.release()
    return all_hsv_pixels