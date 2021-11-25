function setText(text) {
    console.log("Calling settext");
    var jsonObject = JSON.parse(text);
    document.getElementById("sta_status").textContent = jsonObject.sta_status;
    document.getElementById("sta_ip_address").textContent = jsonObject.sta_ip_address;
    document.getElementById("ap_ip_address").textContent = jsonObject.ap_ip_address;
    document.getElementById("mac_address").textContent = jsonObject.mac_address;
    document.getElementById("rssi").textContent = jsonObject.rssi;
}

function pageLoad() {
    console.log("Calling info page load");
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
  