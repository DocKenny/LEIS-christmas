from app import create_app

MQTT_BROKER = '172.20.10.8'
from flask_cors import CORS

if __name__ == '__main__':
    
    app = create_app(MQTT_BROKER)
    CORS(app)
    app.run(debug=True, host='0.0.0.0', port=5000)