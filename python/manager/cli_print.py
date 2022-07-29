

PROMPT = "IoT-CLI > "


def print_message(message):
    print(PROMPT + message + "\n")


def prompt():
    return input(PROMPT)
