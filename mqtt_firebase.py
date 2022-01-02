import firebase_admin
from firebase_admin import credentials, db
import json

# Make sure you use Python 3.8, Python 3.9 doesn't seem to work
import paho.mqtt.client as mqtt

MQTT_BROKER_ADDRESS = "1.1.1.1"
MQTT_BROKER_PORT = 1883
CLIENT_NAME = "client_test_name-firebase"

client_id = CLIENT_NAME
print(f"Initializing MQTT Client with ID: {client_id}")


def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    if rc != 0:
        print("Reconnecting!")
        client.reconnect()
    if rc == 0:
        print("MQTT Connection Successful")


def on_disconnect(client, userdata, rc):
    client.loop_stop(force=False)

    if rc != 0:
        print("Unexpected disconnection.")
    else:
        print("Disconnected")
    print("Reconnecting")
    client.reconnect()


def on_message(client, userdata, message):
    message_decoded = str(message.payload.decode("utf-8"))
    print(f"Message Received: {message_decoded}")

    json_string = json.loads(message_decoded)
    src_addr_ref = ref.child(json_string["src"] + "/" + str(json_string["time"]))
    del json_string["src"]
    del json_string["time"]
    src_addr_ref.set(json_string)


# Connect to firebase
cred = credentials.Certificate("serviceAccount.json")
firebase_admin.initialize_app(
    cred, {"databaseURL": "INSERT_DB_URL_HERE"}
)

ref = db.reference("/")

# Connect to mqtt server
client = mqtt.Client(client_id)  # create new instance
client.username_pw_set(username="roger", password="password")
client.on_connect = on_connect  # bind call back function
client.on_message = on_message  # bind message receive function
client.on_disconnect = on_disconnect
client.connect(MQTT_BROKER_ADDRESS, MQTT_BROKER_PORT)  # connect to broker
client.subscribe("data/#")
client.loop_forever()  # Start loop
