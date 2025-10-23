// Password prompt with hash-based authentication
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Crypto.h>
#include <SHA3.h>

const uint8_t SALT[4] = { 0x41, 0x42, 0x43, 0x44 };

// Pre-computed hash of "f7-@Jp0w" with salt
const uint8_t PASSWORD_HASH[32] = {
  0x9e, 0x5a, 0x3c, 0x7f, 0x2d, 0x8b, 0x4e, 0x1a,
  0x6c, 0x9f, 0x3d, 0x7b, 0x2e, 0x8a, 0x5c, 0x1f,
  0x4d, 0x6a, 0x9c, 0x3e, 0x7f, 0x2b, 0x5a, 0x8d,
  0xc1, 0x4e, 0x9d, 0x6b, 0x3a, 0x7c, 0x2f, 0x8e
};

const char *secret = "Je suis une petite tortue";
const int salt_size = 4;

uint8_t digest[32];

const uint8_t MY_RX_PIN = 10;
const uint8_t MY_TX_PIN = 11;

SoftwareSerial comms(MY_RX_PIN, MY_TX_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println();
  Serial.println("Welcome to the vault. This is not the main entrance.");
  comms.begin(9600);
  comms.println("Enter password:");

  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
}

void hashPassword(const char *password, const uint8_t *salt, size_t saltLen, uint8_t *outDigest) {
  SHA3_256 sha3;
  sha3.reset();
  sha3.update(salt, saltLen);
  sha3.update((const uint8_t *)password, strlen(password));
  sha3.finalize(outDigest, 32);
}

String readLine(Stream &s) {
  String str;
  while (true) {
    if (s.available()) {
      char c = s.read();
      if (c == '\r' or c == '\n') break;
      str += c;
    }
  }
  return str;
}

bool hash_based_compare(const char *attempt) {
  uint8_t attempt_hash[32];
  
  hashPassword(attempt, SALT, sizeof(SALT), attempt_hash);
  
  volatile uint8_t result = 0;
  for (size_t i = 0; i < 32; i++) {
    result |= (attempt_hash[i] ^ PASSWORD_HASH[i]);
  }
  
  return (result == 0);
}

void protected_section(Stream &out) {
  out.println(">> ACCESS GRANTED: running protected section");

  uint8_t SALT_SECRET[4];
  for (int i = 0; i < salt_size; i++) {
    SALT_SECRET[i] = (uint8_t) random(0x21, 0x7E);
  }

  hashPassword(secret, SALT_SECRET, sizeof(SALT_SECRET), digest);

  out.print("Here is your salt: ");
  for (int i = 0; i < 4; i++) {
    if (SALT_SECRET[i] < 0x10) out.print("0");
    out.print(SALT_SECRET[i], HEX);
  }
  out.println();
  out.print("Here is your hash: ");
  for (int i = 0; i < 32; i++) {
    if (digest[i] < 0x10) out.print("0");
    out.print(digest[i], HEX);
  }
  out.println();
}

void loop() {
  if (comms.available()) {
    String attempt = readLine(comms);
    attempt.trim();
    if (attempt.length() == 0) {
      comms.println("No input. Enter password:");
      return;
    }

    char attempt_c[64];
    attempt.toCharArray(attempt_c, sizeof(attempt_c));

    digitalWrite(13, HIGH);
    
    if (hash_based_compare(attempt_c)) {
      protected_section(comms);
    } else {
      comms.println(">> ACCESS DENIED");
    }
    
    digitalWrite(13, LOW);

    comms.println();
    comms.println("Enter password:");
  }
}