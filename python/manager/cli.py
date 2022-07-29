import json
from matplotlib import use
from sqlalchemy import true


import argparse
import io
import os
import sys
import time


import cli_print

from google_manager_parser import parser

import google_iot_api as google_iot

import argparse
import io
import os
import sys
import time

from google.api_core.exceptions import AlreadyExists
from google.cloud import iot_v1
from google.cloud import pubsub
from google.oauth2 import service_account
from google.protobuf import field_mask_pb2 as gp_field_mask
from googleapiclient import discovery
from googleapiclient.errors import HttpError
from concurrent.futures import TimeoutError
from time import sleep
from google.cloud import pubsub_v1
import datetime
import time
import jwt
import paho.mqtt.client as mqtt

# Constants
CONFIG = "config"
COMMAND = "command"


# Setup Variables
project_id = None
cloud_region = None
registry_id = None
device_id = None
ssl_path = None
ssl_algo = None
google_app_cred = None


# Trigger Variables
subscriber = None


# Publishing Variables
client_id = None
client = None


queued_messages = {CONFIG: None, COMMAND: None}


def create_jwt():
    cur_time = datetime.datetime.utcnow()
    token = {
        'iat': cur_time,
        'exp': cur_time + datetime.timedelta(minutes=60),
        'aud': project_id
    }

    with open(ssl_path, 'r') as f:
        private_key = f.read()

    return jwt.encode(token, private_key, ssl_algo)


def error_str(rc):
    return '{}: {}'.format(rc, mqtt.error_string(rc))


def on_connect(unusued_client, unused_userdata, unused_flags, rc):
    print('on_connect', error_str(rc))


def on_publish(unused_client, unused_userdata, unused_mid):
    print('on_publish')


def subscription_callback(message: pubsub_v1.subscriber.message.Message) -> None:
    print(f"Received {message}.")

    client.username_pw_set(username='unused', password=create_jwt())
    client.tls_set(ca_certs=google_app_cred)
    client.connect('mqtt.googleapis.com', 8883)
    client.loop_start()

    for key in queued_messages.keys():

        message = queued_messages[key]

        if message != None:
            mqtt_topic = '/devices/{}/events/{}'.format(args.device_id, key)
            client.publish(mqtt_topic, message, qos=1)
            print("Published to topic ", mqtt_topic)
            client.loop()
            queued_messages[key] = None

    client.loop_stop()

    message.ack()


def run_command(args):

    if args.command == "setup":
        project_id = args.project_id
        cloud_region = args.cloud_region
        registry_id = args.registry_id
        ssl_algo = args.ssl_algo
        ssl_path = args.ssl_path
        os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = args.gogole_app_cred

        pass

    elif args.command == "set-trigger":
        subscriber = pubsub_v1.SubscriberClient()
        # The `subscription_path` method creates a fully qualified identifier
        # in the form `projects/{project_id}/subscriptions/{subscription_id}`
        subscription_path = subscriber.subscription_path(
            project_id, args.sub_id)
        streaming_pull_future = subscriber.subscribe(
            subscription_path, callback=subscription_callback)
        print(f"Listening for messages on {subscription_path}..\n")

        with subscriber:

            try:
                # When `timeout` is not set, result() will block indefinitely,
                # unless an exception is encountered first.
                result = streaming_pull_future.result(timeout=1000)
                print(result)
            except TimeoutError:
                streaming_pull_future.cancel()  # Trigger the shutdown.
                # Block until the shutdown is complete.
                streaming_pull_future.result()

            sleep(1.0)

    elif args.command == "downlink":

        device_id = args.device_id

        client_id = 'projects/{}/locations/{}/registries/{}/devices/{}'.format(
            project_id, cloud_region, registry_id, device_id)
        client = mqtt.Client(client_id=client_id)
        client.on_connect = on_connect
        client.on_publish = on_publish

        json_data = json.load(args.file)
        queued_messages[args.type] = json_data

    pass


if __name__ == "__main__":

    while true:

        user_input = cli_print.prompt()

        split_input = user_input.split()

        args = parser.parse_args(args=split_input)

        print(args)

        run_command(args)
