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
int cartCounts[4] = {0, 0, 0, 0};
int countChanged = 0;
int shouldConfirm = 0;

#define NUM_BUTTONS 9
const uint8_t BUTTON_PINS[NUM_BUTTONS] = {0, 1, 4, 5, 6, 7, 8, 9, A1};

Bounce * buttons = new Bounce[NUM_BUTTONS];

void setup() {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].attach(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i].interval(25);
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
        cartCounts[i] += 1;
        countChanged = 1;
      } else if (i < 8) {
        if (cartCounts[i - 4] > 0) {
          cartCounts[i - 4] -= 1;
          countChanged = 1;
        }
      } else {
        shouldConfirm = 1;
      }
    }
  }

  if (countChanged) {
    countChanged = 0;
    sendCartState();
  } else if (shouldConfirm) {
    shouldConfirm = 0;
    sendConfirm();
  }
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

    zeroCartCounts();
  } else {
    // if you couldn't make a connection:
    Serial.println("connection failed");
  }
}

void zeroCartCounts() {
  memset(cartCounts, 0, sizeof cartCounts);
  countChanged = 1;
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
