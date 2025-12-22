import paho.mqtt.client as mqtt
import json
from threading import Lock

class MQTTClientManager:
    _instance = None
    _lock = Lock()
    
    def __new__(cls, broker_address="192.168.1.199", port=1883):
        if cls._instance is None:
            with cls._lock:
                if cls._instance is None:
                    cls._instance = super().__new__(cls)
                    cls._instance._initialized = False
        return cls._instance
    
    def __init__(self, broker_address="192.168.1.199", port=1883):
        if self._initialized:
            return
        self.broker_address = broker_address
        self.port = port
        self.client = mqtt.Client()
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.is_connected = False
        self._initialized = True
    
    def _on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print(f"Connected to MQTT Broker at {self.broker_address}:{self.port}")
            self.is_connected = True
        else:
            print(f"Failed to connect, return code {rc}")
            self.is_connected = False
    
    def _on_disconnect(self, client, userdata, rc):
        print(f"Disconnected from MQTT Broker with code {rc}")
        self.is_connected = False
    
    def connect(self):
        try:
            self.client.connect(self.broker_address, self.port, 60)
            self.client.loop_start()
            return True
        except Exception as e:
            print(f"Error connecting to MQTT broker: {e}")
            return False
    
    def publish(self, topic, message):
        if not self.is_connected:
            raise ConnectionError("MQTT client is not connected")
        
        if isinstance(message, (dict, list)):
            message = json.dumps(message)
        
        result = self.client.publish(topic, message)
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            print(f"Published to {topic}")
            return True
        else:
            print(f"Failed to publish to {topic}")
            return False
    
    def disconnect(self):
        self.client.loop_stop()
        self.client.disconnect()