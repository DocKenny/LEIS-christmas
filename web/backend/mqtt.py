import paho.mqtt.client as mqtt

def connect_mqtt(broker_address="192.168.1.199"):
    client = mqtt.Client()
    client.connect(broker_address, 1883, 60)
    client.loop_start()
    return client

def publish(client, topic, message):
    client.publish(topic, message)
    print(f"Published to {topic}: {message}")

def subscribe(client, topic, on_message):
    def on_message_wrapper(client, userdata, msg):
        print(f"Received message from {msg.topic}: {msg.payload.decode()}")
        on_message(msg.topic, msg.payload.decode())

    client.subscribe(topic)
    client.message_callback_add(topic, on_message_wrapper)

    print(f"Subscribed to {topic}")

def message_handler(topic, message):
    print(f"Handler received message from {topic}: {message}")

    
# if __name__ == "__main__":
#     client = connect_mqtt()
#     def message_handler(topic, message):
#         print(f"Handler received message from {topic}: {message}")

#     subscribe(client, "test/topic", message_handler)
#     publish(client, "test/topic", "Hello MQTT")