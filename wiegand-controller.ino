/*
 * Copyright (c) 2014 jgor <jgor@indiecom.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#define GREEN 4 // pin 4 toggles green light
#define BEEP 5 // pin 5 toggles beep

typedef struct {
  long facility;
  long card;
  int bits;
} credential_t;

const int authorized[][2] = {
  {3, 1337}
};

credential_t credential;

int wiegand[64];
int wiegand_idx, waiting;

void decode35() {
  int i;
  long facility = 0, card = 0;
  
  for (i = 2; i < 14; i++) {
    facility |= (wiegand[i] << (13-i));
  }
  
  for (i = 14; i < 34; i++) {
    card |= ((long)wiegand[i] << (33-i));
  }
  
  credential.facility = facility;
  credential.card = card;
  credential.bits = 35;
}

void decode26() {
  int i;
  long facility = 0, card = 0;
  
  for (i = 1; i < 9; i++) {
    facility |= (wiegand[i] << (8-i));
  }
  
  for (i = 9; i < 25; i++) {
    card |= ((long)wiegand[i] << (24-i));
  }
  
  credential.facility = facility;
  credential.card = card;
  credential.bits = 26;
}

void decode(int len) {
  switch(len) {
    case 26: decode26(); break;
    case 35: decode35(); break;
  }
  
  
}

void verify() {
  boolean granted = false;
  
  for (int i = 0; i < sizeof(authorized) / sizeof(int); i++) {
    if (credential.facility == authorized[i][0] && credential.card == authorized[i][1]) {
      granted = true;
      break;
    }
  }
  if (granted == true) {
    access_granted();
  }
  else {
    access_denied();
  }
}

void display_card() {
  Serial2.write(0x0C); // clear screen
  delay(5); // delay 5ms after clear, per spec
  Serial2.print("F=");
  Serial2.print(credential.facility);
  Serial2.print(" C=");
  Serial2.print(credential.card);
  Serial2.write(0x0D);
  Serial2.print(credential.bits);
  Serial2.print("bit || ");
}

void access_granted() {
  Serial2.print("GRANTED");
  
  for (int i = 0; i < 3; i++) {
    digitalWrite(GREEN, LOW);
    digitalWrite(BEEP, LOW);
    delay(50);
    digitalWrite(GREEN, HIGH);
    digitalWrite(BEEP, HIGH);
    delay(50);
  }
  
  digitalWrite(GREEN, LOW);
  delay(1000);
  digitalWrite(GREEN, HIGH);
}

void access_denied() {
  Serial2.print("DENIED");
}

void data0() {
  wiegand[wiegand_idx++] = 0;
  waiting = 10000;
}

void data1() {
  wiegand[wiegand_idx++] = 1;
  waiting = 10000;
}

void setup() {
  wiegand_idx = 0;
  
  pinMode(GREEN, OUTPUT);
  digitalWrite(GREEN, HIGH);
  pinMode(BEEP, OUTPUT);
  digitalWrite(BEEP, HIGH);
  
  attachInterrupt(0, data0, FALLING);
  attachInterrupt(1, data1, FALLING);

  Serial.begin(9600);
  
  Serial2.begin(9600);
  Serial2.write(0x16); // screen on, no cursor, no blink
  Serial2.write(0x11); // backlight on
  
  Serial2.write(0x0C); // clear screen
  delay(5); // delay 5ms after clear, per spec
  Serial2.print("Scan card...");
  
}

void loop() {
  if (!waiting && wiegand_idx) {
    decode(wiegand_idx);
    wiegand_idx = 0;
    display_card();
    verify();
    delay(3000);
  }
  else if (waiting) {
    waiting--;
  }
}

