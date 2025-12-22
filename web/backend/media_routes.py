from flask import request, jsonify, current_app
from werkzeug.utils import secure_filename
import os
from mqtt_client import MQTTClientManager
from media_processor import downscale_image, downscale_video
import threading

def allowed_file(filename, allowed_extensions):
    return '.' in filename and \
           filename.rsplit('.', 1)[1].lower() in allowed_extensions

def process_and_send_media(file_path, media_type, mqtt_client, config):
    """Background task to process and send media"""
    try:
        if media_type == 'image':
            hsv_data = downscale_image(file_path)
            mqtt_client.publish("image/hsv", hsv_data)
            print(f"Sent image with {len(hsv_data)} pixels")
        
        elif media_type == 'video':
            frame_limit = config.get('frame_limit', 20)
            start_frame = config.get('start_frame', 0)
            hsv_data = downscale_video(file_path, frame_limit, start_frame)
            mqtt_client.publish("video/hsv", hsv_data)
            print(f"Sent video with {len(hsv_data)} frames")
    
    except Exception as e:
        print(f"Error processing media: {e}")
    finally:
        # Clean up uploaded file
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
        return 'Hello, World!'
    
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
            return jsonify({'error': 'Invalid file type'}), 400
        
        filename = secure_filename(file.filename)
        file_path = os.path.join(current_app.config['UPLOAD_FOLDER'], filename)
        file.save(file_path)
        
        # Process in background
        thread = threading.Thread(
            target=process_and_send_media,
            args=(file_path, 'image', mqtt_manager, {})
        )
        thread.start()
        
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
        frame_limit = request.form.get('frame_limit', 20, type=int)
        start_frame = request.form.get('start_frame', 0, type=int)
        
        filename = secure_filename(file.filename)
        file_path = os.path.join(current_app.config['UPLOAD_FOLDER'], filename)
        file.save(file_path)
        
        # Process in background
        config = {
            'frame_limit': frame_limit,
            'start_frame': start_frame
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