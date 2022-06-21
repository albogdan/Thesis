function setText(text) {
    console.log("Calling settext");
    var jsonObject = JSON.parse(text);
    document.getElementById("uplink_mode").textContent = jsonObject.uplink_mode;

    document.getElementById("network_id").textContent = jsonObject.network_id;

    document.getElementById("sta_status").textContent = jsonObject.sta_status;
    document.getElementById("sta_encrypt_mode").textContent = jsonObject.sta_encrypt_mode;
    document.getElementById("sta_ip_address").textContent = jsonObject.sta_ip_address;

    document.getElementById("cellular_status").textContent = jsonObject.cellular_status;
    
    document.getElementById("ap_ip_address").textContent = jsonObject.ap_ip_address;
    document.getElementById("ap_ssid").textContent = jsonObject.ap_ssid;
    document.getElementById("ap_password").textContent = jsonObject.ap_password;

    document.getElementById("mqtt_status").textContent = jsonObject.mqtt_status;
    document.getElementById("mqtt_ip").textContent = jsonObject.mqtt_ip;
    document.getElementById("mqtt_port").textContent = jsonObject.mqtt_port;
    
    document.getElementById("project_id").textContent = jsonObject.project_id;
    document.getElementById("location").textContent = jsonObject.location;
    document.getElementById("registry_id").textContent = jsonObject.registry_id;
    document.getElementById("device_id").textContent = jsonObject.device_id;

    document.getElementById("mac_address").textContent = jsonObject.mac_address;
    document.getElementById("rssi").textContent = jsonObject.rssi;
}

function pageLoad() {
    var xhttp = new XMLHttpRequest();
  
    // Callback
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        console.log("Response: " + this.responseText);
        setText(this.responseText);
      }
    };
    xhttp.open("GET", "loadInfo", true);
    xhttp.send();
}
  