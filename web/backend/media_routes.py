from flask import request, jsonify, current_app
from werkzeug.utils import secure_filename
import os
from mqtt_client import MQTTClientManager
from media_processor import downscale_gif, downscale_image, downscale_video
import threading

DOWNSCALE_DIR = './web/backend/instance/downscale/'

def build_packet(images, brightness=50, fps=5, isLastPacket=False):
    return {
        "brightness": brightness,
        "fps": fps,
        "isLastPacket": isLastPacket,
        "chunk": 0,
        "images": images
    }


def allowed_file(filename, allowed_extensions):
    return '.' in filename and \
           filename.rsplit('.', 1)[1].lower() in allowed_extensions

def process_and_send_media(file_path, media_type, mqtt_client, config, gamma=2.2):
    DOWNSCALE_IMG_PATH = "downscaled_image.png"
    os.makedirs(DOWNSCALE_DIR, exist_ok=True)

    try:
        brightness = config.get("brightness", 50)
        fps = config.get("fps", 2)

        if media_type == 'image':
            rgb_data = downscale_image(file_path, os.path.join(DOWNSCALE_DIR, DOWNSCALE_IMG_PATH))

            # build numeric packet for MQTT (devices may expect numeric RGB tuples)
            packet = build_packet(
                images=[rgb_data],  # single frame wrapped in list
                brightness=brightness,
                fps=fps,
                isLastPacket=True
            )
            with open(file_path + ".json", 'w') as f:
                import json
                json.dump(packet, f)
            mqtt_client.publish("image/hsv", packet)
            print(f"Sent image packet with {len(rgb_data)} pixels")

            def rgb_to_hex(rgb):
                r, g, b = rgb
                return '#{0:02x}{1:02x}{2:02x}'.format(int(r), int(g), int(b))

            hex_pixels = [rgb_to_hex(px) for px in rgb_data]
            return {"pixelData": hex_pixels}

        elif media_type == 'video':
            frame_limit = config.get('frame_limit', 5)
            start_frame = config.get('start_frame', 0)

            print(frame_limit)
            if file_path.lower().endswith('.gif'):
                hsv_frames = downscale_gif(file_path, frame_limit=20, gamma=gamma)
            else:
                hsv_frames = downscale_video(file_path, frame_limit, start_frame, gamma=gamma)

            packet = build_packet(
                images=hsv_frames,
                brightness=brightness,
                fps=fps,
                isLastPacket=False
            )
            # Save packet to json file
            with open(file_path + ".json", 'w') as f:
                import json
                json.dump(packet, f)

            mqtt_client.publish("image/hsv", packet)
            print(f"Sent video packet with {len(hsv_frames)} frames")

    except Exception as e:
        print(f"Error processing media: {e}")
    finally:
        if os.path.exists(file_path):
            os.remove(file_path)


def init_routes(app):
    mqtt_manager = MQTTClientManager(
        app.config['MQTT_BROKER'],
        app.config['MQTT_PORT']
    )
    mqtt_manager.connect()
    
    @app.route('/hello')
    def hello():
        return "Hello world"
    
    @app.route('/api/status', methods=['GET'])
    def status():
        return jsonify({
            'status': 'online',
            'mqtt_connected': mqtt_manager.is_connected
        })
    
    @app.route('/api/upload/image', methods=['POST'])
    def upload_image():
        if 'file' not in request.files:
            return jsonify({'error': 'No file part'}), 400
        
        file = request.files['file']
        if file.filename == '':
            return jsonify({'error': 'No selected file'}), 400
        
        if not allowed_file(file.filename, current_app.config['ALLOWED_IMAGE_EXTENSIONS']):
            return jsonify({'error': 'Invalid file typ1e'}), 400
        
        filename = secure_filename(file.filename)
        file_path = os.path.join(current_app.config['UPLOAD_FOLDER'], filename)
        file.save(file_path)
        
        # Process synchronously for images so the frontend receives pixelData
        brightness = request.form.get("brightness", 50, type=int)
        fps = request.form.get("fps", 20, type=int)

        result = process_and_send_media(
            file_path, 'image', mqtt_manager, {
                "brightness": brightness,
                "fps": fps
            }
        )

        # result may contain 'pixelData' (hex strings) for the frontend
        if result and isinstance(result.get('pixelData'), list):
            return jsonify({
                'message': 'Image uploaded and processed',
                'filename': filename,
                'pixelData': result['pixelData']
            }), 200

        return jsonify({
            'message': 'Image uploaded and processing started',
            'filename': filename
        }), 202
    
    @app.route('/api/upload/video', methods=['POST'])
    def upload_video():
        if 'file' not in request.files:
            return jsonify({'error': 'No file part'}), 400
        
        file = request.files['file']
        if file.filename == '':
            return jsonify({'error': 'No selected file'}), 400
        
        if not allowed_file(file.filename, current_app.config['ALLOWED_VIDEO_EXTENSIONS']):
            return jsonify({'error': 'Invalid file type'}), 400
        
        # Get optional parameters
        frame_limit = request.form.get('frame_limit', 15, type=int)
        start_frame = request.form.get('start_frame', 968, type=int)
        
        filename = secure_filename(file.filename)
        file_path = os.path.join(current_app.config['UPLOAD_FOLDER'], filename)
        file.save(file_path)
        
        # Process in background
        brightness = request.form.get("brightness", 50, type=int)
        fps = request.form.get("fps", 20, type=int)

        config = {
            'frame_limit': frame_limit,
            'start_frame': start_frame,
            'brightness': brightness,
            'fps': fps
        }
        thread = threading.Thread(
            target=process_and_send_media,
            args=(file_path, 'video', mqtt_manager, config)
        )
        thread.start()
        
        return jsonify({
            'message': 'Video uploaded and processing started',
            'filename': filename,
            'frame_limit': frame_limit,
            'start_frame': start_frame
        }), 202
    
    @app.route('/api/send/custom', methods=['POST'])
    def send_custom():
        """Send custom payload to MQTT"""
        data = request.get_json()
        
        if not data or 'topic' not in data or 'payload' not in data:
            return jsonify({'error': 'Missing topic or payload'}), 400
        
        try:
            mqtt_manager.publish(data['topic'], data['payload'])
            return jsonify({'message': 'Message sent successfully'}), 200
        except Exception as e:
            return jsonify({'error': str(e)}), 500