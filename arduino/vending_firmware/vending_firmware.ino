#include <SPI.h>
#include <WiFiNINA.h>
#include <Bounce2.h>

#include "arduino_secrets.h" 
///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)

int status = WL_IDLE_STATUS;

// Initialize the Wifi client library
WiFiClient client;

// server address:
//char server[] = "example.org";
IPAddress server(178,62,213,33);

unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
const unsigned long postingInterval = 10L * 1000L; // delay between updates, in milliseconds

int counter = 0;
int cartCounts[5] = {0, 0, 0, 0, 0};
int isCountsChanged = 0;
//11 = 00, 16 = 03, 22 = 11, 33 = 22
int productNumbers[4] = {0, 3, 11, 22};
int productDelays[4] = {30, 24, 22, 21}; //seconds
//int productDelays[4] = {27, 21, 19, 18};
int shouldPressNumbers = 0;
unsigned long readyToPressTime;

#define NUM_BUTTONS 9
const uint8_t BUTTON_PINS[NUM_BUTTONS] = {0, 1, 4, 5, 6, 7, 8, 9, A1};

#define NUM_OUTPUT_BUTTONS 5
const uint8_t OUTPUT_BUTTON_PINS[NUM_OUTPUT_BUTTONS] = {10, 11, 12, 13, 14};

Bounce * buttons = new Bounce[NUM_BUTTONS];

void setup() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].attach(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i].interval(25);
  }

  for (int i = 0; i < NUM_OUTPUT_BUTTONS; i++) {
    pinMode(OUTPUT_BUTTON_PINS[i], OUTPUT);   
  }
  
  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  /*while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }*/

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  // you're connected now, so print out the status:
  printWifiStatus();
}

void loop() {
  // if there's incoming data from the net connection.
  // send it out the serial port.  This is for debugging
  // purposes only:
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

  for (int i = 0; i < NUM_BUTTONS; i++)  {
    buttons[i].update();
    
    if (buttons[i].fell()) {
      if (i < 4) {
        if (cartCounts[4] == 0) {
          cartCounts[i] += 1;
          isCountsChanged = 1;
        }
      } else if (i < 8) {
        if (cartCounts[i - 4] > 0) {
          if (cartCounts[4] == 0) {
            cartCounts[i - 4] -= 1;
            isCountsChanged = 1;
          }
        }
      } else {
        isCountsChanged = 1;

        if (cartCounts[4] < 2) {
          cartCounts[4]++;
        } else {
          zeroCartCounts();
          cartCounts[4] = 0;
        }

        if (cartCounts[4] == 2) {
          shouldPressNumbers = 1;
          readyToPressTime = millis();
        }
      }
    }
  }

  /*for (int i = 0; i < NUM_OUTPUT_BUTTONS; i++) {
    if (cartCounts[4] == 1) {
      digitalWrite(OUTPUT_BUTTON_PINS[i], HIGH);
    } else {
      digitalWrite(OUTPUT_BUTTON_PINS[i], LOW);
    }    
  }*/

  if (isCountsChanged) {
    isCountsChanged = 0;
    sendCartState();
  }

  if (shouldPressNumbers && (millis() - readyToPressTime > 500)) {
    shouldPressNumbers = 0;
    pressNumbersForCart();

    zeroCartCounts();
    cartCounts[4] = 0;
    sendCartState();
  }

  /*if (millis() - readyToPressTime > 1000) {
    readyToPressTime = millis();
    cartCounts[0] = 1;
    pressNumbersForCart();
  }*/

  /*digitalWrite(10, HIGH);
  delay(500);
  digitalWrite(10, LOW);
  delay(500);*/
}

void pressNumbersForCart() {
  Serial.println("Press numbers");
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < cartCounts[i]; j++) {
      Serial.print("i: ");
      Serial.println(i);
      Serial.print("j: ");
      Serial.println(j);
      pressNumbers(productNumbers[i]);
      delay(productDelays[i] * 1000);
      pressNumber(4);
      delay(2000);
    }
  }
}

void pressNumbers(int productNumber) {
  int firstDigit = productNumber / 10;
  int secondDigit = productNumber % 10;

  Serial.print("firstDigit: ");
  Serial.println(firstDigit);
  Serial.print("secondDigit: ");
  Serial.println(secondDigit);
  
  digitalWrite(OUTPUT_BUTTON_PINS[firstDigit], HIGH);
  delay(500);
  digitalWrite(OUTPUT_BUTTON_PINS[firstDigit], LOW);
  delay(1000);
  digitalWrite(OUTPUT_BUTTON_PINS[secondDigit], HIGH);
  delay(500);
  digitalWrite(OUTPUT_BUTTON_PINS[secondDigit], LOW);
}

void pressNumber(int index) {
  digitalWrite(OUTPUT_BUTTON_PINS[index], HIGH);
  delay(100);
  digitalWrite(OUTPUT_BUTTON_PINS[index], LOW);
  delay(100);
}

void sendCartState() {
  // close any connection before send a new request.
  // This will free the socket on the Nina module
  client.stop();

  char buffer[20];
  int length = sizeof(cartCounts) / sizeof(int);
  int index = 0;
  int i;

  for (i = 0; i < length; i++) {
    index += sprintf(&buffer[index], "%d,", cartCounts[i]);
  }

  buffer[index - 1] = '\0';

  Serial.print("buffer: ");
  Serial.print(buffer);

  Serial.print("; length: ");
  Serial.println(index);

  char contentLengthBuffer[30];
  sprintf(contentLengthBuffer, "content-length: %d", index - 1);

  // if there's a successful connection:
  if (client.connect(server, 3000)) {
    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.println("POST /state HTTP/1.1");
    client.println("Host: 178.62.213.33");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Content-Type: text/plain");
    client.println(contentLengthBuffer);
    client.println("Connection: close");
    client.println();
    client.println(buffer);
    client.println();

    // note the time that the connection was made:
    lastConnectionTime = millis();
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
}

void sendConfirm() {
  // close any connection before send a new request.
  // This will free the socket on the Nina module
  client.stop();

  // if there's a successful connection:
  if (client.connect(server, 3000)) {
    Serial.println("connecting...");
    client.println("GET /confirm HTTP/1.1");
    client.println("Host: 178.62.213.33");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();

    // note the time that the connection was made:
    lastConnectionTime = millis();
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
}

void zeroCartCounts() {
  Serial.println("Clearing counts");
  memset(cartCounts, 0, sizeof cartCounts);
  isCountsChanged = 1;
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
