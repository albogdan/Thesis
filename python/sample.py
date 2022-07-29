###
# Copyright 2017, Google, Inc.
# Licensed under the Apache License, Version 2.0 (the `License`);
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an `AS IS` BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###

#!/usr/bin/python

import datetime
import time
import jwt
import paho.mqtt.client as mqtt


# Define some project-based variables to be used below. This should be the only
# block of variables that you need to edit in order to run this script

# https://medium.com/google-cloud/cloud-iot-step-by-step-connecting-raspberry-pi-python-2f27a2893ab5


#ssl_private_key_filepath = '/home/r/Desktop/summer_job/rpi_python/device_ec_private.pem'
ssl_algorithm = 'ES256'  # Either RS256 or ES256
root_cert_filepath = '/home/r/Desktop/summer_job/rpi_python/roots.pem'

#root_cert_filepath = '/home/r/Desktop/summer_job/rpi_python/ec_cert.pem'

project_id = 'primeval-yew-357817'
gcp_location = 'us-central1'


ssl_private_key_filepath = '/home/r/Desktop/summer_job/hand_off/ec_private.pem'
registry_id = 'gateway_registry'
device_id = 'ESP32_1'


#ssl_private_key_filepath = '/home/r/Desktop/summer_job/rpi_python/ec_private.pem'
#registry_id = 'registry_1'
#device_id = 'device_2'

#device_id = 'device_new'

# end of user-variables

cur_time = datetime.datetime.utcnow()


def create_jwt():
    token = {
        'iat': cur_time,
        'exp': cur_time + datetime.timedelta(minutes=60),
        'aud': project_id
    }

    with open(ssl_private_key_filepath, 'r') as f:
        private_key = f.read()

    return jwt.encode(token, private_key, ssl_algorithm)


_CLIENT_ID = 'projects/{}/locations/{}/registries/{}/devices/{}'.format(
    project_id, gcp_location, registry_id, device_id)
_MQTT_TOPIC = '/devices/{}/events'.format(device_id)
_MQTT_TOPIC = '/devices/{}/events/{}'.format(device_id, 'alive')
#_MQTT_TOPIC = '/devices/{}/commands'.format(device_id)

#_MQTT_TOPIC = 'projects/ece1528-cf0b9/topics/topic-1'

client = mqtt.Client(client_id=_CLIENT_ID)
# authorization is handled purely with JWT, no user/pass, so username can be whatever
client.username_pw_set(
    username='unused',
    password=create_jwt())


def error_str(rc):
    return '{}: {}'.format(rc, mqtt.error_string(rc))


def on_connect(unusued_client, unused_userdata, unused_flags, rc):
    print('on_connect', error_str(rc))


def on_publish(unused_client, unused_userdata, unused_mid):
    print('on_publish')


client.on_connect = on_connect
client.on_publish = on_publish

# Replace this with 3rd party cert if that was used when creating registry
client.tls_set(ca_certs=root_cert_filepath)
client.connect('mqtt.googleapis.com', 8883)
client.loop_start()

# Could set this granularity to whatever we want based on device, monitoring needs, etc
temperature = 0
humidity = 0
pressure = 0


for i in range(1, 11):

    payload = '{{ "isLive": true, "deviceId" : "ESP32_{}", "temperature": {}, "pressure": {}, "humidity": {} }}'.format(
        int(time.time()), i, i, i)

    #payload = "hello" + str(i)

    # Uncomment following line when ready to publish
    print("Topic is "+_MQTT_TOPIC)
    print(client.publish(_MQTT_TOPIC, payload, qos=1))

    print("{}\n".format(payload))

    time.sleep(10)

    client.loop()

client.loop_stop()
