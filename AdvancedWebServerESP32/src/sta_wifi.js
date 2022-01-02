$("#selectWiFiEncryptionType").change(function() {
    if ($(this).val() == "personal") {
        $('#enterprise').hide();
        $('#personal').show();
        document.getElementById("wifi_header").textContent = "Connect to WPA2 Personal WiFi";
    } else if ($(this).val() == "enterprise") {
        $('#personal').hide();
        $('#enterprise').show();
        document.getElementById("wifi_header").textContent = "Connect to WPA2 Enterprise WiFi";
    } else {
        $('#personal').hide();
        $('#enterprise').hide();
        document.getElementById("wifi_header").textContent = "Connect to WiFi";
    }
  });
$("#selectWiFiEncryptionType").trigger("change");