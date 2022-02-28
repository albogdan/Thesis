bool connectToCellular() {
  cellular_modem.init();

  String modemInfo = cellular_modem.getModemInfo();
  Serial.print("[INFO] Modem Info: ");
  Serial.println(modemInfo);

  Serial.print("[INFO] Waiting for network...");
  if (!cellular_modem.waitForNetwork()) {
    Serial.println(" fail");
    delay(10000);
    return CELLULAR_CONNECTION_FAILURE;
  }
  Serial.println(" success");
  
  if (cellular_modem.isNetworkConnected()) { Serial.println("[INFO] Network connected"); }

  Serial.print(F("[INFO] Connecting to "));
  Serial.print(apn);
  if (!cellular_modem.gprsConnect(apn, gprsUser, gprsPass)) {
    Serial.println(" fail");
    delay(10000);
    return CELLULAR_CONNECTION_FAILURE;
  }
  Serial.println(" success");

  if (cellular_modem.isGprsConnected()) { Serial.println("[INFO] GPRS connected"); }
  return CELLULAR_CONNECTION_SUCCESS;
}
