import firebase_admin
from firebase_admin import credentials, db
import json

cred = credentials.Certificate("serviceAccount.json")
firebase_admin.initialize_app(
    cred, {"databaseURL": "INSERT_DB_URL_HERE"}
)

ref = db.reference("/")


def insert_into_database(base_ref, data_list):
    for i in range(len(data_list)):
        key = list(data_list[i].keys())[0]
        key_data = data_list[i][key]
        child_ref = base_ref.child(f"{key}")

        if type(key_data) == list:
            insert_into_database(child_ref, key_data)
        elif type(key_data) == dict:
            child_ref.set(key_data)
        else:
            print("[ERROR] Data type not dict or list")


with open("testdata.json", "r") as f:
    print("[INFO] Starting program.")
    print("[INFO] Reading JSON.")
    file_contents = json.load(f)

    print("[INFO] Inserting into the database.")
    insert_into_database(ref, file_contents)

    print("[INFO] Program complete.")
