int refreshToken() {
  WiFiClientSecure client;
  String mbodyRefresh = "client_secret=[GFIT_CLIENT_SECRET]=refresh_token&refresh_token=" + getTokenGoogle() + "&client_id=[GFIT_CLIENT_ID]";
  Serial.println("REFRESH TOKEN\n\nConnecting to host\n");

  if (!client.connect(host_google, httpPort)) {
    Serial.println("Connection failed");
    return -11;
  }

  //building header
  String payload = String("POST ") +  "/oauth2/v4/token HTTP/1.1\n" +
                   "Host: " + host_google + "\n" +
                   "Content-length: " + mbodyRefresh.length() + "\n" +
                   "content-type: application/x-www-form-urlencoded\n\n" +
                   mbodyRefresh + "\r\n";

  client.println(payload);
  Serial.println(payload);

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(F("\n>>> Client Timeout !"));
      client.stop();
      return -12;
    }
  }

  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.println(F("Unexpected response: "));
    Serial.println(status);
    return -13;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders)) return -14;

  char carriageReturn[] = "\r\n";
  if (!client.find(carriageReturn)) return -15;

  const size_t capacity = JSON_OBJECT_SIZE(3);
  DynamicJsonDocument doc(capacity);

  // Parse JSON object - read docs
  DeserializationError error = deserializeJson(doc, client);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return -16;
  }

  // Extract values
  JsonObject root = doc.as<JsonObject>();
  Serial.println(F("Response:"));
  authKey = root["access_token"].as<char*>();
  Serial.println(authKey);

  if (!authKey.length() > 0) return -16;

  Serial.println(F("Correclty got the TOKEN\n"));

  // Disconnect
  client.stop();
  return 1;
}

int postWeight(String mweight) {
  int tokenStatus = refreshToken();
  if (tokenStatus < 0) return tokenStatus;

  WiFiClientSecure client;
  PRTLN("\nPOST WEIGHT\n\nConnecting to host\n");

  if (!client.connect(host_google, httpPort)) {
    Serial.println("Connection failed");
    return -21;
  }

  //building body; WARNING: "dataSourceId" could be different if you created one different from mine
  String mbody = String("{") + "\"minStartTimeNs\":" + timens + ",\"maxEndTimeNs\":" + timens + ",\"dataSourceId\":\"derived:com.google.weight:MeMyself:ArduinoScale:9988012:ScaleDataSource\",\"point\":[{\"dataTypeName\":\"com.google.weight\"," +
                 "\"originDataSourceId\":\"\",\"startTimeNanos\":" + timens + ",\"endTimeNanos\":" + timens + ",\"value\":[{\"fpVal\":" + mweight + "}]}]}";

  String payload = String("PATCH ") +  "/fitness/v1/users/me/dataSources/derived:com.google.weight:MeMyself:ArduinoScale:9988012:ScaleDataSource/datasets/" + timens + "-" + timens +  " HTTP/1.1\n" +
                   "Host: " + host_google + "\n" +
                   "Content-length: " + mbody.length() + "\n" +
                   "Content-type: application/json\n" +
                   "Authorization: Bearer " + authKey + "\n\n" +
                   mbody + "\n";

  client.println(payload);
  PRTLN(payload);

  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(F("\n>>> Client Timeout !"));
      client.stop();
      return -22;
    }
  }

  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0) {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return -23;
  }

  Serial.print(F("\nPosted succesfully: "));
  Serial.println(mweight);
  client.stop();
  return 1;
}

/*-------- NTP code ----------*/
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte ntpPacketBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime() {
  IPAddress ntpServerIP; // NTP server's ip address
  while (Udp.parsePacket() > 0);
  Serial.println("Transmit NTP Request");
  WiFi.hostByName(ntpServerName, ntpServerIP);
  yield();
  sendNTPpacket(ntpServerIP);
  yield();
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("\nReceived correct NTP Response");
      Udp.read(ntpPacketBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)ntpPacketBuffer[40] << 24;
      secsSince1900 |= (unsigned long)ntpPacketBuffer[41] << 16;
      secsSince1900 |= (unsigned long)ntpPacketBuffer[42] << 8;
      secsSince1900 |= (unsigned long)ntpPacketBuffer[43];
      return secsSince1900 - 2208988800UL + TIME_ZONE * 60;
    }
  }
  yield();
  Serial.println("No NTP Response.");
  return 0; // return 0 if unable to get the time
}

void sendNTPpacket(IPAddress &address) {
  memset(ntpPacketBuffer, 0, NTP_PACKET_SIZE);
  ntpPacketBuffer[0] = 0b11100011;   // LI, Version, Mode
  ntpPacketBuffer[1] = 0;     // Stratum, or type of clock
  ntpPacketBuffer[2] = 6;     // Polling Interval
  ntpPacketBuffer[3] = 0xEC;  // Peer Clock Precision
  ntpPacketBuffer[12]  = 49;
  ntpPacketBuffer[13]  = 0x4E;
  ntpPacketBuffer[14]  = 49;
  ntpPacketBuffer[15]  = 52;
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(ntpPacketBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}
