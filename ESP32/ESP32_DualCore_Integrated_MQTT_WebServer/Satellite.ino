bool connectToSatellite()
{
  int signalQuality = -1;
  int err;

  // Start the console serial port
  Serial.begin(115200);
  while (!Serial)
    ;

  // Start the serial port connected to the satellite satelliteModem
  IridiumSerial.begin(19200);

  // Begin satellite satelliteModem operation
  Serial.println(F("Starting satelliteModem..."));
  err = satelliteModem.begin();
  if (err != ISBD_SUCCESS)
  {
    Serial.print(F("Begin failed: error "));
    Serial.println(err);
    if (err == ISBD_NO_MODEM_DETECTED)
      Serial.println(F("No satelliteModem detected: check wiring."));
    return false;
  }

  // Example: Print the firmware revision
  char version[12];
  err = satelliteModem.getFirmwareVersion(version, sizeof(version));
  if (err != ISBD_SUCCESS)
  {
    Serial.print(F("FirmwareVersion failed: error "));
    Serial.println(err);
    return false;
  }
  Serial.print(F("Firmware Version is "));
  Serial.print(version);
  Serial.println(F("."));

  // Example: Print the IMEI
  char IMEI[16];
  err = satelliteModem.getIMEI(IMEI, sizeof(IMEI));
  if (err != ISBD_SUCCESS)
  {
    Serial.print(F("getIMEI failed: error "));
    Serial.println(err);
    return false;
  }
  Serial.print(F("IMEI is "));
  Serial.print(IMEI);
  Serial.println(F("."));

  // Example: Test the signal quality.
  // This returns a number between 0 and 5.
  // 2 or better is preferred.
  err = satelliteModem.getSignalQuality(signalQuality);
  if (err != ISBD_SUCCESS)
  {
    Serial.print(F("SignalQuality failed: error "));
    Serial.println(err);
    return false;
  }

  Serial.print(F("On a scale of 0 to 5, signal quality is currently "));
  Serial.print(signalQuality);
  Serial.println(F("."));

  return true;
}
