import requests

user_name = "disonwu8@gmail.com"
password = "cottoncandy@NRL7"
imei = "300534062396950"


message = "Jun 25 2022 HTTP Test"
hex_message = message.encode('utf-8').hex()

url = "https://rockblock.rock7.com/rockblock/MT?username=" + user_name + \
    "&password=" + password + "&data=" + hex_message + "&imei=" + imei

headers = {"Accept": "text/plain"}

response = requests.post(url, headers=headers)

print(response.text)
