import argparse
import io
import os
import sys
import time


parser = argparse.ArgumentParser(
    description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
)


commands = parser.add_subparsers(dest="command", help='Management Functions')


setup_parser = commands.add_parser("setup")
setup_parser.add_argument(
    "-cloud_region", help="GCP cloud region", required=True
)
setup_parser.add_argument(
    "-project_id",
    help="GCP cloud project name.", required=True
)
setup_parser.add_argument(
    "-registry_id",
    help="Registry id. If not set, a name will be generated.", required=True
)

setup_parser.add_argument(
    "-ssl_algo",
    help="The SSL algorithim used. ", choices=("ES256"), required=True
)

setup_parser.add_argument(
    "-ssl_file_path",
    help="The full path to the privat ekey path to publish messages.", required=True
)

setup_parser.add_argument(
    "-gogole_app_cred",
    help="The full path to the file with the required google application credentials.", required=True
)


set_trigger_parser = commands.add_parser("set-trigger")

set_trigger_parser.add_argument(
    "-sub_id", help="The subscription id the Manager listens for before publishing queued commands", required=True
)


downlink_parser = commands.add_parser("downlink")
set_trigger_parser.add_argument(
    "-device_id", help="The device the Manager listens to ", required=True
)
downlink_parser.add_argument(
    "-type", help="The type of down link message.", required=True
)
downlink_parser.add_argument(
    "-file",  help="The json file where the command is stored.", required=True
)
