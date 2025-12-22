import json
import cv2
from mqtt import connect_mqtt, publish, subscribe, message_handler

def downscale_image(input_path, output_path):
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
            hsv_pixels.append((int(h), int(s), int(v)))
        
    # Save the downscaled image
    cv2.imwrite(output_path, downscaled_image)

    return hsv_pixels

def downscale_video(input_path, output_path_pattern, frame_limit):
    cap = cv2.VideoCapture(input_path)
    if not cap.isOpened():
        raise ValueError(f"Could not open the video file {input_path}")

    cap.set(cv2.CAP_PROP_POS_FRAMES, 200) # Start frame

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
                hsv_pixels.append((int(h), int(s), int(v)))
        all_hsv_pixels.append(hsv_pixels)

        output_path = output_path_pattern.format(frame_count)
        cv2.imwrite(output_path, downscaled_frame)

        frame_count += 1

    cap.release()
    return all_hsv_pixels

def send_payload(client, topic, payload):
    message = json.dumps(payload)
    publish(client, topic, message)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Downscale an image by a given scale factor.")
    parser.add_argument("input_path", type=str, help="Path to the input image file.")
    parser.add_argument("output_path", type=str, help="Path to save the downscaled image.")

    args = parser.parse_args()

    # # Image
    # payload = downscale_image(args.input_path, args.output_path)
    # print("payload size:", len(payload))
    # client = connect_mqtt()
    # send_payload(client, "image/hsv", payload)

    # Video
    payload = downscale_video(args.input_path, "video/downscaled_frame_{}.png", frame_limit=20)
    print("payload size:", len(payload))
    client = connect_mqtt()
    send_payload(client, "video/hsv", payload)






                                                                                                                             