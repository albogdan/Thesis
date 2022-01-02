from elasticsearch import Elasticsearch
import firebase_admin
from firebase_admin import credentials, db
import json
import logging as log
import requests
import uuid

index_settings = {
    "settings": {"number_of_shards": 1, "number_of_replicas": 0},
    "mappings": {
        "properties": {
            "src_addr": {"type": "keyword"},
            "datetime": {
                "type": "date",
                "format": "strict_date_optional_time||epoch_second",
            },
            "temperature": {"type": "integer"},
        }
    },
}


def connect_elasticsearch(host="localhost", port=9200):
    log.info(f"Connecting to ES at {host}:{port}")
    es = None
    es = Elasticsearch([{"host": host, "port": port}])
    if es.ping():
        log.info("Elasticsearch connected.")
    else:
        log.info("Elasticsearch not connected.")

    return es


def create_index(es_host: Elasticsearch, index_name="nodes"):
    index_created = False
    try:
        if not es_host.indices.exists(index=index_name):
            # Ignore 400 means to ignore "Index Already Exist" error.
            es_host.indices.create(
                index=index_name,
                settings=index_settings["settings"],
                mappings=index_settings["mappings"],
            )
            log.info("Index created")
            index_created = True
    except Exception as ex:
        log.error(str(ex))
    finally:
        return index_created


def store_record(es_host: Elasticsearch, index_name, record, id):
    try:
        result = es_host.index(
            index=index_name, doc_type="_doc", document=record, id=id
        )
    except Exception as ex:
        log.error(str(ex))


first_read = True


def firebase_realtime_database_listener_callback(event):
    global first_read
    print(f"TYPE: {event.event_type}")  # can be 'put' or 'patch'
    print(f"PATH: {event.path}")  # relative to the reference, it seems
    print(f"DATA: {event.data}")  # new data at /reference/event.path. None if deleted

    if first_read:
        print("[INFO] First read")
        first_read = False
        for src_addr in event.data:
            print(f"Num entries for {src_addr}: {len(event.data[src_addr])}")
            for epoch_time in event.data[src_addr]:
                output_dict = {
                    "src_addr": src_addr,
                    "datetime": epoch_time,
                    "temperature": event.data[src_addr][epoch_time]["data"],
                }
                store_record(
                    es_host=connect_elasticsearch(),
                    index_name="nodes",
                    record=output_dict,
                    id=uuid.uuid5(uuid.NAMESPACE_X500, src_addr + "|" + epoch_time),
                )
                print(f"Out: {output_dict}")
    else:
        split_path = event.path.split("/")[1:]
        print(f"Split path: {split_path}")
        output_dict = {
            "src_addr": split_path[0],
            "datetime": split_path[1],
            "temperature": event.data["data"],
        }
        store_record(
            es_host=connect_elasticsearch(),
            index_name="nodes",
            record=output_dict,
            id=uuid.uuid5(uuid.NAMESPACE_X500, split_path[0] + "|" + split_path[1]),
        )
        print(f"New entry: {output_dict}")


if __name__ == "__main__":
    # Start the logger
    log.basicConfig(level=log.INFO)

    # Connect to Elasticsearch
    es = connect_elasticsearch()
    index_created = create_index(es_host=es)

    # Connect to Firebase, use a service account
    cred = credentials.Certificate("serviceAccount.json")
    obj = firebase_admin.initialize_app(
        cred, {"databaseURL": "INSERT_DB_URL_HERE"}
    )

    db.reference("", app=obj).listen(firebase_realtime_database_listener_callback)
    
    
