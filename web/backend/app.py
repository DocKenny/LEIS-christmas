import os
from flask import Flask, request, jsonify
from werkzeug.utils import secure_filename
import threading

def create_app(mqtt_broker='192.168.1.199', test_config=None):
    app = Flask(__name__, instance_relative_config=True)
    app.config.from_mapping(
        SECRET_KEY='dev',
        DATABASE=os.path.join(app.instance_path, 'backend.sqlite'),
        UPLOAD_FOLDER=os.path.join(app.instance_path, 'uploads'),
        MAX_CONTENT_LENGTH=100 * 1024 * 1024,  # 100MB max file size
        ALLOWED_IMAGE_EXTENSIONS={'png', 'jpg', 'jpeg', 'bmp'},
        ALLOWED_VIDEO_EXTENSIONS={'mp4', 'avi', 'mov', 'mkv', 'webm', 'gif'},
        MQTT_BROKER=mqtt_broker,
        MQTT_PORT=1883
    )
    
    if test_config is None:
        app.config.from_pyfile('config.py', silent=True)
    else:
        app.config.from_mapping(test_config)
        
    # Ensure directories exist
    try:
        os.makedirs(app.instance_path)
        os.makedirs(app.config['UPLOAD_FOLDER'])
    except OSError:
        pass
    
    # Import routes
    from media_routes import init_routes
    init_routes(app)
    
    return app