/*
    This sketch demonstrates how to set up a simple HTTP-like server.
    The server will set a GPIO pin depending on the request
      http://server_ip/gpio/0 will set the GPIO2 low,
      http://server_ip/gpio/1 will set the GPIO2 high
    server_ip is the IP address of the ESP8266 module, will be
    printed to Serial when the module is connected.
*/
#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <Servo.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS D4

const char* ssid = "testing";
const char* password = "03123456";
const char* host = "175.213.99.205";
String path = "/pattern?light=";
int sensorPin = 0;
int servoPin = 2;
int curAngle = 30;
int angle_value;
int db_check_flag = 0;   // 1 : db checking loop / 0 : not db checking loop
int tag_data_flag = 0;   // 1 : user adjusted the blind / 0 : user did not adjust the blind
int db_timer_start, db_timer_end;  // timer for DB checking (get ligth value, and time then send) 
int adjust_timer_start, adjust_timer_end;  // timer for user adjustment wait 
int demo_day_test[3] = {9, 15, 20};
int demo_day_test_ind = 0;
int prev = 1;

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(3000);
Servo servo;

//set onewire
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
char temperatureString[6];

float getTemperature() {
  float temp;
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(0);
    delay(100);
  } while (temp == 85.0 || temp == (-127.0));
  return temp;
}


void setup() {
  servo.attach(servoPin);
  Serial.begin(115200);
  delay(10);

  // prepare GPIO3
  pinMode(D3, OUTPUT);
  digitalWrite(D3, 0);

  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void update_db(){
/*  
  WiFiClient client;
  const int httpPort = 8811;
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  client.print(String("GET ") + path + light_value + 
               "&hour=" + hour_value +
               " HTTP/1.1\r\n" +
               "Host: " + host + ":" + httpPort + "\r\n" + 
               "Connection: keep-alive\r\n\r\n");
               */
}

void send_light_hour(){
  // get light 
  int light_value = analogRead(sensorPin);
  int hour_value;
  hour_value = demo_day_test[(demo_day_test_ind++)%3];

  WiFiClient client;
  const int httpPort = 8080; // port open up in raspberry pi 
  if (!client.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  client.print(String("GET ") + path + light_value + 
               "&hour=" + hour_value +
               " HTTP/1.1\r\n" +
               "Host: " + host + ":" + httpPort + "\r\n" + 
               "Connection: keep-alive\r\n\r\n");
}

// pattern adjustment when receives pattern value from DB 
void adjust_pattern(String req){
    int ind = req.indexOf("=");
    ind++;
    String temp = "";
    while( req[ind] >= '0' && req[ind] <= '9' ){
      temp += req[ind++];
    }
    angle_value = temp.toInt();
    Serial.println("GOT pattern");
    Serial.print("TEST :");
    Serial.println(angle_value);
    
    curAngle = 30 * angle_value;
    servo.write(curAngle);
    Serial.println("Angle adjusted");
}

void loop() {
  // check current time 
  // String cur_date = getTime();
  /*
  if ( db_timer_end - db_timer_end == 3000 ) // three seconds period for demo-day
    db_check_flag = 1;
    */
    
    //temperature
    float temperature = getTemperature();
    dtostrf(temperature, 2, 2, temperatureString);
    //Serial.println(temperatureString);
    
    
   db_check_flag = 1; // test
    
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client && !db_check_flag) {
    return;
  }

  /*
  if ( db_check_flag == 1 ) { // this loop was the first time since DB checking
    db_check_flag = 0;
    tag_data_flag = 0;
    // send light and hour 
    //send_light_hour();
    client = server.available();

   
    // Read the first line of the request
    // request is a pattern value received from the pi-server.
    //String req = client.readStringUntil('\r');
    //Serial.println(req);
    
    // adjust blind with returned pattern from DB
    //adjust_pattern(req);
    adjust_timer_start = 0;//now();
    // set first_time flag to zero in order not to repeat.
    // db_check_flag = 0;
  }
  */

  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
 
  // Match the request
  int val = -1;
  if (req.indexOf("/?button=up") != -1){
    val = 1;
    Serial.println("test : button=up");
  }
  else if (req.indexOf("/?button=down") != -1){
    val = 2;
    Serial.println("test : button=down");
  }
  else if (req.indexOf("/pattern?value") != -1){
    Serial.println("FUNCTION CALL TO ADJUST PATTERN");
    adjust_pattern(req);
  }
  else{ 
    Serial.println(req); 
    Serial.println("invalid request");
  }

  
  /* ************************************
  * MAKE RESPONSE FOR THE REMOTE CONTROL
  * ************************************ */
  if ( val == 1 || val == 2 || val == 3) {
    // Set GPIO2 according to the request
    if ( val == 1 && curAngle <= 80){
      curAngle += 10;
      Serial.println("test : motor up");
    }
    else if (val == 2 && curAngle >= 10){
      curAngle -= 10;
      Serial.println("test : motor down");
    }
      
    servo.write(curAngle);
    // Prepare the response
    String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nBlind is now ";
    if (val == 1 )
      s += "down";
    else if (val == 2)
      s += "up";
    s += "</html>\n";
      
    // Send the response to the client
    if ( val == 1 || val == 2 ){
      tag_data_flag = 1; // user adjusted the blind w/ remote controler.
      client.print(s);
    }
    delay(1);
    
    // The client will actually be disconnected
    // when the function returns and 'client' object is detroyed
  
    // send adjusted data in order to update database
    /*
    if ( adjust_timer_end - adjust_timer_start >= 60000 && tag_data_flag == 1 )
      update_db(); */
  }
  Serial.println("Client disonnected");
  client.flush();
}

