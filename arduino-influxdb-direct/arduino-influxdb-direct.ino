#include <Ethernet.h>
#include <Timer.h>

EthernetClient client;

// network information
byte mac[] = {0x98, 0x4F, 0xEE, 0x00, 0x81, 0x54};
IPAddress ip(172, 16, 0, 100);

IPAddress server(160, 98, 61, 150);

/** InfluxDB HTTP port */
const int eth_port = 8086;

/** Size of the buffer for HTTP content */
const int bufferSize = 2048;

/** An character array filled with null terminate chars */
char buf[bufferSize] = {'\0'};

/* Sensor */
int temperature_sensor = 1;
int luminosity_sensor = 0;

// Temperature info for computation
int a;
float temperature;
int B = 3975;
float resistance;

/* TIMER */
int timerAction;
Timer timer;

void setup() {
  Serial.begin(115200);
  delay(1000);
  connectToInflux();
  timerAction = timer.every(100, updateData); // each second call
}

void loop() {
  timer.update();
}

void sendData(char* data, int dataSize) {
  //first we need to connect to InfluxDB server
  int conState = client.connect(server, eth_port);

  if (conState <= 0) { //check if connection to server is stablished
    Serial.print("Could not connect to InfluxDB Server, Error #");
    Serial.println(conState);
    return;
  }

  //Send HTTP header and buffer
  client.println("POST /write?db=arduino HTTP/1.1");
  client.println("Host: www.embedonix.com");
  client.println("User-Agent: Arduino/1.0");
  client.println("Connection: close");
  client.println("Content-Type: application/x-www-form-urlencoded");
  client.print("Content-Length: ");
  client.println(dataSize);
  client.println();
  client.println(data);

  delay(50); //wait for server to process data

  //Now we read what server has replied and then we close the connection
  Serial.println("Reply from InfluxDB");
  while (client.available()) { //receive char
    Serial.print((char)client.read());
  }
  Serial.println(); //empty line

  client.stop();
}

void connectToInflux() {
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }
  delay(2000); // give time to allow connection

  //do a fast test if we can connect to server
  int conState = client.connect(server, eth_port);

  if (conState > 0) {
    Serial.println("Connected to InfluxDB server");
    client.stop();
  }

  //print the error number and return false
  Serial.print("Could not connect to InfluxDB Server, Error #");
  Serial.println(conState);
}

void updateData() {
  // get temperature
  a = analogRead(temperature_sensor);
  resistance = (float)(1023 - a) * 10000 / a;
  temperature = 1 / (log(resistance / 10000) / B + 1 / 298.15) - 273.15;
  // get light
  int luminosity = analogRead(luminosity_sensor);

  int numChars = 0;

  // measurement name
  numChars = sprintf(buf, "arduino-sensor,");

  // tag with space at the end
  numChars += sprintf(&buf[numChars], "SOURCE=arduino_1 ");

  // field
  numChars += sprintf(&buf[numChars], "temp=%f,", temperature);
  numChars += sprintf(&buf[numChars], "light=%d", luminosity);

  //Print the buffer on the serial line to see how it looks
  Serial.print("Sending following dataset to InfluxDB: ");
  Serial.println(buf);

  sendData(buf, numChars);
  memset(buf, '\0', bufferSize);
  delay(1000); //some small delay!
}

