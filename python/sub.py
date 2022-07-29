from concurrent.futures import TimeoutError
from time import sleep
from google.cloud import pubsub_v1
import os

from sqlalchemy import true


os.environ["GOOGLE_APPLICATION_CREDENTIALS"] = "/home/r/Desktop/summer_job/rpi_python/keys/sa-1.json"


# TODO(developer)
project_id = "spartan-grail-350018"
subscription_id = "topic_1_pull"
# Number of seconds the subscriber should listen for messages
timeout = 5.0

subscriber = pubsub_v1.SubscriberClient()
# The `subscription_path` method creates a fully qualified identifier
# in the form `projects/{project_id}/subscriptions/{subscription_id}`
subscription_path = subscriber.subscription_path(project_id, subscription_id)


def callback(message: pubsub_v1.subscriber.message.Message) -> None:
    print(f"Received {message}.")
    message.ack()


streaming_pull_future = subscriber.subscribe(
    subscription_path, callback=callback)
print(f"Listening for messages on {subscription_path}..\n")

# Wrap subscriber in a 'with' block to automatically call close() when done.
with subscriber:

    while true:
        try:
            # When `timeout` is not set, result() will block indefinitely,
            # unless an exception is encountered first.
            result = streaming_pull_future.result(timeout=timeout)
            print(result)
        except TimeoutError:
            streaming_pull_future.cancel()  # Trigger the shutdown.
            # Block until the shutdown is complete.
            streaming_pull_future.result()

        sleep(1.0)
