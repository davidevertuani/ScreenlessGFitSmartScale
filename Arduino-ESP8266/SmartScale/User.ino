/multi-user support
String token_google_user1 = "[TOKEN_USER_1]";
String token_google_user2 = "[TOKEN_USER_2]";
String token_google_user3 = "[TOKEN_USER_3]";
String token_google_user4 = "[TOKEN_USER_4]";

String token_pushbullet_user1 = "[PUSHBULLET_TOKEN_USER_1]";
String token_pushbullet_user2 = "[PUSHBULLET_TOKEN_USER_2]";
String token_pushbullet_user3 = "[PUSHBULLET_TOKEN_USER_3]";
String token_pushbullet_user4 = "[PUSHBULLET_TOKEN_USER_4]";

String CHAR_POOO = "\\uD83D\\uDCA9";
String CHAR_LOVE = "\\uD83D\\uDE0D";

String get_pushbulet_token(){
  switch (user) {
    case 0:
      return token_pushbullet_user1;
      break;
    case 1:
      return token_pushbullet_user2;
      break;
    case 2:
      return token_pushbullet_user3;
      break;
    case 3:
      return token_pushbullet_user4;
      break;
  }
}

int getLed(int user) {
  switch (user) {
    case 0:
      return LED0;
      break;
    case 1:
      return LED1;
      break;
    case 2:
      return LED2;
      break;
    case 3:
      return LED3;
      break;
  }
}

void allLedOff() {
  digitalWrite(LED0, getLow(LED0));
  digitalWrite(LED1, getLow(LED1));
  digitalWrite(LED2, getLow(LED2));
  digitalWrite(LED3, getLow(LED3));
}

void allLedOn() {
  digitalWrite(LED0, getHigh(LED0));
  digitalWrite(LED1, getHigh(LED1));
  digitalWrite(LED2, getHigh(LED2));
  digitalWrite(LED3, getHigh(LED3));
}

//simple function to blink leds in a pattern
bool ledPhase = true;
void changeAllLeds(bool l0, bool l1, bool l2, bool l3) {
  if (ledPhase) {
    digitalWrite(LED0, ((l0) ? getHigh(LED0) : getLow(LED0)));
    digitalWrite(LED1, ((l1) ? getHigh(LED1) : getLow(LED1)));
    digitalWrite(LED2, ((l2) ? getHigh(LED2) : getLow(LED2)));
    digitalWrite(LED3, ((l3) ? getHigh(LED3) : getLow(LED3)));
  } else {
    digitalWrite(LED0, ((l0) ? getLow(LED0) : getHigh(LED0)));
    digitalWrite(LED1, ((l1) ? getLow(LED1) : getHigh(LED1)));
    digitalWrite(LED2, ((l2) ? getLow(LED2) : getHigh(LED2)));
    digitalWrite(LED3, ((l3) ? getLow(LED3) : getHigh(LED3)));
  }
  ledPhase = !ledPhase;
}

void blink_led() {
  if ((millis() - lastblink) > 1000) {
    if (currentUser > -1) digitalWrite(getLed(currentUser), !digitalRead(getLed(currentUser)));
    else changeAllLeds(true, true, true, true);
    lastblink = millis();
  }
}


void blink_update() {
  if ((millis() - lastblink) > 1000) {
    changeAllLeds(true, false, false, true);
    lastblink = millis();
  }
}

void shift_led() {
  if ((millis() - lastblink) > 750) {
    current_shift_led++;
    current_shift_led = current_shift_led % user_count;
    int mled = getLed(current_shift_led);
    allLedOff();
    digitalWrite(mled, getHigh(mled));
    lastblink = millis();
  }
}

String getTokenGoogle() {
  switch (currentUser) {
    case 0:
      return token_google_user1;
      break;
    case 1:
      return token_google_user2;
      break;
    case 2:
      return token_google_user3;
      break;
    case 3:
      return token_google_user4;
      break;
  }
}

String getTopic() {
  switch (currentUser) {
    case 0:
      return "User1"; //customize with family members names...
      break;
    case 1:
      return "User2";
      break;
    case 2:
      return "User3";
      break;
    case 3:
      return "User4";
      break;
  }
}

int getLow(int pin) {
  if (pin == LED3) return HIGH;
  else return LOW;
}

int getHigh(int pin) {
  if (pin == LED3) return LOW;
  else return HIGH;
}

String getPhrase(float weight) {
  if (currentUser < 0) return getJoke();

  int kg = (int) weight;
  int gr = (int) ((weight - kg) * 100);

  float oldWeight = getKg() + (getGr() * 0.01);
  writeWeight(kg, gr);

  float diff = weight - oldWeight;
  String phrase = "Since last time: " + String((diff > 0) ? "+" : "") + String(diff, 2) + " kg " + ((diff > 0) ?  CHAR_POOO : CHAR_LOVE) + "\n";

  if (oldWeight > weight) phrase += getCompliment();
  else phrase += getJoke();

  return phrase;
}

String getCompliment() {
  switch (random(2)) {
    case 0: return "You're so slim today!";
    case 1: return "Custom compliment 2"; break;
    //etc...
    }
}

String getJoke() {
  switch (random(2)) {
    case 0: return "No elephants accepted on the scale."; break;
    case 1: return "Custom joke 2"; break;
    //etc...
  }
}

int getKg() {
  return EEPROM.read(currentUser);
}

int getGr() {
  return EEPROM.read(currentUser + 4);
}

void writeWeight(int kg, int gr) {
  EEPROM.write(currentUser, kg);
  EEPROM.write(currentUser + 4, gr);
  EEPROM.commit();
}
