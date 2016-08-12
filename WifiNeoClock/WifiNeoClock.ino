/*

 Udp NTP Client

 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket
 For more on NTP time servers and the messages needed to communicate with them,
 see http://en.wikipedia.org/wiki/Network_Time_Protocol

 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 updated for the ESP8266 12 Apr 2015 
 by Ivan Grokhotkov

 This code is in the public domain.

 */

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>

#include <NeoPixelBus.h>

#define UTCOffsetAddress 65

#define TIMECHECK 60000

#define PIN 2
#define CLOCKPIXELS 36

RgbColor red = RgbColor(10,0,0);
RgbColor orange = RgbColor(10,10,0);
RgbColor white = RgbColor(10, 10, 10);
RgbColor yellow = RgbColor(10, 10, 0);

NeoPixelBus strip = NeoPixelBus(CLOCKPIXELS, PIN);
const IPAddress AP_IP(192, 168, 1, 1);
const char* AP_SSID = "WIFI_PIXEL_CLOCK";
boolean SETUP_MODE;
String SSID_LIST;
DNSServer DNS_SERVER;
ESP8266WebServer WEB_SERVER(80);
String DEVICE_TITLE = "Wifi Clock";

unsigned long lastMillis=millis();


byte hourVal;
byte minuteVal;
byte secondsVal;

byte hourValLocal;
byte minuteValLocal;
byte secondsValLocal;

long UTCOffset=19800;


boolean isTimeZoneahead=true;
boolean isPostMeridien=false;
boolean UTCPostMeridien=false;
boolean firstRequest=true;
byte failedAttemptsCount=0;
int i,p;
int j;

unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

void setup()
{
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(3000);
  if (loadSavedConfig())
  {
    //wipeEEPROM();
    if (checkWiFiConnection())
    {
        SETUP_MODE = false;
        startWebServer();
        Serial.println("SetupMode=False");
        UTCOffset= EEPROMReadlong(UTCOffsetAddress);
        Serial.println(UTCOffset);
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());        
        Serial.println("Starting UDP");
        udp.begin(localPort);
        Serial.print("Local port: ");
        Serial.println(udp.localPort());
        strip.Begin();
        strip.Show(); // Initialize all pixels to 'off'
        //showNoInternetOnLEDs();
      
    }
    else
    {
      SETUP_MODE = true;
    }
  }
  else
  {
    SETUP_MODE = true;
    Serial.println("SetupMode=True");
    showReadyForSetup();
    setupMode();
    return;
  }
  
 
  
}

void loop()
{
  if(SETUP_MODE)
  {
    // Serial.println("Process");
    DNS_SERVER.processNextRequest(); 
    WEB_SERVER.handleClient();   
    return;
  }  
  WEB_SERVER.handleClient();  
  if(millis()-lastMillis>TIMECHECK || firstRequest)
  {
    firstRequest=false;
    lastMillis=millis();
    
    //get a random server from the pool
    WiFi.hostByName(ntpServerName, timeServerIP); 
    
    sendNTPpacket(timeServerIP); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay(1000);
    
    int cb = udp.parsePacket();
    if (!cb)
    {
        Serial.println("no packet yet");
        failedAttemptsCount=failedAttemptsCount+1;
    }
    else
    {
        failedAttemptsCount=0;
        Serial.print("packet received, length=");
        Serial.println(cb);
        // We've received a packet, read the data from it
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
        
        //the timestamp starts at byte 40 of the received packet and is four bytes,
        // or two words, long. First, esxtract the two words:
        
        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer
        // this is NTP time (seconds since Jan 1 1900):
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        Serial.print("Seconds since Jan 1 1900 = " );
        Serial.println(secsSince1900);
        
        // now convert NTP time into everyday time:
        Serial.print("Unix time = ");
        // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        const unsigned long seventyYears = 2208988800UL;
        // subtract seventy years:
        unsigned long epoch = secsSince1900 - seventyYears;
        // print Unix time:
        calculateTime(epoch);
        showTimeOnLEDs(hourValLocal,minuteValLocal);
    
    
    }
    if(failedAttemptsCount>3)
    {
      showNoInternetOnLEDs();
      failedAttemptsCount=0;
    }
  }
  
}

void showNoInternetOnLEDs()
{
  for(i=0;i<CLOCKPIXELS;i++)
  {
        strip.SetPixelColor(i, red);
        strip.Show();
  }
}

void showReadyForSetup()
{
  for(i=0;i<CLOCKPIXELS;i++)
  {
        strip.SetPixelColor(i, orange);
        strip.Show();
  }
}



void calculateTime(unsigned long epoch)
{
   Serial.println(epoch);
    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
   hourVal=(epoch  % 86400L) / 3600;
   minuteVal=(epoch  % 3600) / 60;
   secondsVal=epoch % 60;

   if(hourVal>11)
      {
        hourVal=hourVal%12;
        UTCPostMeridien=true;
        
      }
      
      else
      {
        UTCPostMeridien=false;
      }

    Serial.print(hourVal);
    Serial.print(":");
    Serial.print(minuteVal);
    Serial.print(":");
    Serial.println(secondsVal);

    //UTCPostMeridien

    
    unsigned long localEpoch=epoch+UTCOffset;
    hourValLocal=(localEpoch  % 86400L) / 3600;
    minuteValLocal=(localEpoch  % 3600) / 60;
    secondsValLocal=localEpoch % 60;

     if(hourValLocal>11)
      {
        hourValLocal=hourValLocal%12;
        isPostMeridien=true;
        
      }
      else
      {
        isPostMeridien=false;
      }

    
    Serial.print("Local Time is: " );
    Serial.print(hourValLocal);
    Serial.print(":");
    Serial.print(minuteValLocal);
    Serial.print(":");
    Serial.println(secondsValLocal);
    //showTimeOnLEDs(hourVal,minuteVal);
   
}
 


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}


void showTimeOnLEDs(byte hh, byte mm)
{

  
 for(i=0;i<CLOCKPIXELS;i++)
  {
        strip.SetPixelColor(i, RgbColor(3,0,3));
        //strip.show();
  }
  byte minuteLED=mm/2.5;
  byte hourLED=24+hh;
 // byte hourLEDBig=hh*2;
 // byte minuteLEDSmall=24+(mm/5);
  if(hourLED==CLOCKPIXELS)
  {
     hourLED=24;
  }
 // Serial.print(minuteLED);
 // Serial.print("/");
 // Serial.println(hourLED);
 if(isPostMeridien)// its PM
 { 
  //12 o clock
   strip.SetPixelColor(0, white);
   strip.SetPixelColor(24, white);

   //3 o clock
   strip.SetPixelColor(6, white);
   strip.SetPixelColor(27, white);
  
  //6 o clock
   strip.SetPixelColor(12, white);
   strip.SetPixelColor(30, white);
   
  //9 o clock
   strip.SetPixelColor(18, white);
   strip.SetPixelColor(33, white); 
  
  
 
 }
 else //its AM
 {

   //12 o clock
   strip.SetPixelColor(0, yellow);
   strip.SetPixelColor(24, yellow);

   //3 o clock
   strip.SetPixelColor(6, yellow);
   strip.SetPixelColor(27, yellow);
  
  //6 o clock
   strip.SetPixelColor(12, yellow);
   strip.SetPixelColor(30, yellow);
   
  //9 o clock
   strip.SetPixelColor(18, yellow);
   strip.SetPixelColor(33, yellow); 
    
  
  
 }

  
 
 
 strip.SetPixelColor(minuteLED, RgbColor(0,0,60));
 strip.SetPixelColor(hourLED, RgbColor(0,60,0));

 strip.Show();
}




/////////////////////////////
// AP Setup Mode Functions //
/////////////////////////////

// Load Saved Configuration from EEPROM
boolean loadSavedConfig() {
  Serial.println("Reading Saved Config....");
  String ssid = "";
  String password = "";
  if (EEPROM.read(0) != 0) {
    for (int i = 0; i < 32; ++i) {
      ssid += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(ssid);
    for (int i = 32; i < 96; ++i) {
      password += char(EEPROM.read(i));
    }
    Serial.print("Password: ");
    Serial.println(password);
    WiFi.begin(ssid.c_str(), password.c_str());
    return true;
  }
  else {
    Serial.println("Saved Configuration not found.");
    return false;
  }
}

// Boolean function to check for a WiFi Connection
boolean checkWiFiConnection() {
  int count = 0;
  Serial.print("Waiting to connect to the specified WiFi network");
  while ( count < 30 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Connected!");
      return (true);
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  return false;
}

void initHardware()
{
  // Serial and EEPROM
 // Serial.begin(115200);
  
  
  

}

// Start the web server and build out pages
void startWebServer() {
  
  if (SETUP_MODE) {
    Serial.print("Starting Web Server at IP address: ");
    Serial.println(WiFi.softAPIP());
    // Settings Page
    WEB_SERVER.on("/settings", []() {
      String s = "<h2>Wi-Fi Settings</h2>";
      s +="<p>Please select the SSID of the network you wish to connect to and then enter the password and submit.</p>";
      s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
      s += SSID_LIST;
      s += "</select>";
      s +="<br><br>Password: <input name=\"pass\" length=64 type=\"password\"><br><br>";
      s +="<label>Timezone: </label><select name=\"tz\">";
      int timeZoneSeconds=0;
      for(int tzhour=-12;tzhour<13;tzhour++)
      {
        
        for(int tzminute=0;tzminute<60;tzminute=tzminute+15)
        {

          if(tzhour<0)
          {
            timeZoneSeconds=tzhour*3600-(tzminute*60);
          }
          else
          {
            timeZoneSeconds=(tzhour*3600)+ (tzminute*60);
          }
          
          s+="<option value=\"";
          s+=timeZoneSeconds;
          s+="\">";
          s+=tzhour;
          s+=":";
          s+=tzminute;
          s+="</option>";
          //timezoneCounter=timezoneCounter+1;
        }
      }
      s += "</select>";
      
      s +="<br><br>";
      
      s +="<input type=\"submit\"></form>";
      WEB_SERVER.send(200, "text/html", makePage("Wi-Fi Settings", s));
    });
    // setap Form Post
    WEB_SERVER.on("/setap", []() {
      for (int i = 0; i < 96; ++i) 
      {
        EEPROM.write(i, 0);
      }
      String ssid = urlDecode(WEB_SERVER.arg("ssid"));
      Serial.print("SSID: ");
      Serial.println(ssid);
      String pass = urlDecode(WEB_SERVER.arg("pass"));
      Serial.print("Password: ");
      Serial.println(pass);
      long timeZone=atol(urlDecode(WEB_SERVER.arg("tz")).c_str());
      Serial.print("TimeZone:");
      Serial.println(timeZone);
      
      Serial.println("Writing SSID to EEPROM...");
      for (int i = 0; i < ssid.length(); ++i) {
        EEPROM.write(i, ssid[i]);
      }
      Serial.println("Writing Password to EEPROM...");
      for (int i = 0; i < pass.length(); ++i) {
        EEPROM.write(32 + i, pass[i]);
      }
      EEPROM.commit();
      EEPROMWritelong(UTCOffsetAddress,timeZone);
      EEPROM.commit();
      Serial.println("Write EEPROM done!");
      String s = "<h1>WiFi Setup complete.</h1>";
      s += "<p>The button will be connected automatically to \"";
      s += ssid;
      s += "\" after the restart.";
      WEB_SERVER.send(200, "text/html", makePage("Wi-Fi Settings", s));
      ESP.restart();
    });
    // Show the configuration page if no path is specified
    WEB_SERVER.onNotFound([]() {
      String s = "<h1>WiFi Configuration Mode</h1>";
      s += "<p><a href=\"/settings\">Wi-Fi Settings</a></p>";
      WEB_SERVER.send(200, "text/html", makePage("Access Point mode", s));
    });
  }
  else {
    Serial.print("Starting Web Server at ");
    Serial.println(WiFi.localIP());
    WEB_SERVER.on("/", []() {
      IPAddress ip = WiFi.localIP();
      String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
      String s = "<h1>Wifi Pixel Clock</h1>";
      s += "<h3>Network Details</h3>";
      s += "<p>Connected to: " + String(WiFi.SSID()) + "</p>";
      s += "<p>IP Address: " + ipStr + "</p>";
      s += "<h3>Time Details</h3>";
      s += "<p>Current Time: "+ getFormattedTime(hourValLocal, minuteValLocal, secondsValLocal,isPostMeridien) + "</p>";  
      s += "<p>Selected Time Zone: "+getTimeZone(UTCOffset)+"</p>";
      //s += "<p>&nbsp;</p>";
      s += "<p>UTC Time: "+ getFormattedTime(hourVal, minuteVal, secondsVal,UTCPostMeridien ) + "</p>";
     
         
      s += "<h3>Options</h3>";
      s += "<p><a href=\"/reset\">Clear Clock Settings Settings</a></p>";
      WEB_SERVER.send(200, "text/html", makePage("Station mode", s));
    });
    WEB_SERVER.on("/reset", []() 
    {
      wipeEEPROM();
      String s = "<h1>Wi-Fi settings was reset.</h1><p> Clock has been restarted.Please re-configure clock.</p>";
      
      WEB_SERVER.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
      ESP.restart();
    });
  }
  WEB_SERVER.begin();
 
}

// Build the SSID list and setup a software access point for setup mode
void setupMode() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  int n = WiFi.scanNetworks();
  delay(100);
  Serial.println("");
  for (int i = 0; i < n; ++i) {
    SSID_LIST += "<option value=\"";
    SSID_LIST += WiFi.SSID(i);
    SSID_LIST += "\">";
    SSID_LIST += WiFi.SSID(i);
    SSID_LIST += "</option>";
  }
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(AP_SSID);
  DNS_SERVER.start(53, "*", AP_IP);
  startWebServer();
  Serial.print("Starting Access Point at \"");
  Serial.print(AP_SSID);
  Serial.println("\"");
}

String makePage(String title, String contents) {
  String s = "<!DOCTYPE html><html><head>";
  s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
  s += "<style>";
  // Simple Reset CSS
  s += "*,*:before,*:after{-webkit-box-sizing:border-box;-moz-box-sizing:border-box;box-sizing:border-box}";
  s += "html{font-size:100%;-ms-text-size-adjust:100%;-webkit-text-size-adjust:100%}html,button,input";
  s += ",select,textarea{font-family:sans-serif}article,aside,details,figcaption,figure,footer,header,";
  s += "hgroup,main,nav,section,summary{display:block}body,form,fieldset,legend,input,select,textarea,";
  s += "button{margin:0}audio,canvas,progress,video{display:inline-block;vertical-align:baseline}";
  s += "audio:not([controls]){display:none;height:0}[hidden],template{display:none}img{border:0}";
  s += "svg:not(:root){overflow:hidden}body{font-family:sans-serif;font-size:16px;font-size:1rem;";
  s += "line-height:22px;line-height:1.375rem;color:#585858;font-weight:400;background:#fff}";
  s += "p{margin:0 0 1em 0}a{color:#cd5c5c;background:transparent;text-decoration:underline}";
  s += "a:active,a:hover{outline:0;text-decoration:none}strong{font-weight:700}em{font-style:italic}";
  // Basic CSS Styles
  s += "body{font-family:sans-serif;font-size:16px;font-size:1rem;line-height:22px;line-height:1.375rem;";
  s += "color:#585858;font-weight:400;background:#fff}p{margin:0 0 1em 0}";
  s += "a{color:#cd5c5c;background:transparent;text-decoration:underline}";
  s += "a:active,a:hover{outline:0;text-decoration:none}strong{font-weight:700}";
  s += "em{font-style:italic}h1{font-size:32px;font-size:2rem;line-height:38px;line-height:2.375rem;";
  s += "margin-top:0.7em;margin-bottom:0.5em;color:#343434;font-weight:400}fieldset,";
  s += "legend{border:0;margin:0;padding:0}legend{font-size:18px;font-size:1.125rem;line-height:24px;line-height:1.5rem;font-weight:700}";
  s += "label,button,input,optgroup,select,textarea{color:inherit;font:inherit;margin:0}input{line-height:normal}";
  s += ".input{width:100%}input[type='text'],input[type='email'],input[type='tel'],input[type='date']";
  s += "{height:36px;padding:0 0.4em}input[type='checkbox'],input[type='radio']{box-sizing:border-box;padding:0}";
  // Custom CSS
  s += "header{width:100%;background-color: #2c3e50;top: 0;min-height:60px;margin-bottom:21px;font-size:15px;color: #fff}.content-body{padding:0 1em 0 1em}header p{font-size: 1.25rem;float: left;position: relative;z-index: 1000;line-height: normal; margin: 15px 0 0 10px}";
  s += "</style>";
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += "<header><p>" + DEVICE_TITLE + "</p></header>";
  s += "<div class=\"content-body\">";
  s += contents;
  s += "</div>";
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}

/////////////////////////
// Debugging Functions //
/////////////////////////

void wipeEEPROM()
{
  EEPROM.begin(512);
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++)
    EEPROM.write(i, 0);
  EEPROM.commit();
  EEPROM.end();
}

//This function will write a 4 byte (32bit) long to the eeprom at
//the specified address to address + 3.
void EEPROMWritelong(int address, long value)
{
  //Decomposition from a long to 4 bytes by using bitshift.
  //One = Most significant -> Four = Least significant byte
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
  
  //Write the 4 bytes into the eeprom memory.
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

//This function will return a 4 byte (32bit) long from the eeprom
//at the specified address to address + 3.
long EEPROMReadlong(long address)
{
  //Read the 4 bytes from the eeprom memory.
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
  
  //Return the recomposed long by using bitshift.
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}



String getFormattedTime(byte lhour, byte lminute, byte lsecond, byte lIsPostMeridien)
{
  String formattedTime="";
  if(lhour<10)  {    formattedTime=formattedTime+"0"+lhour+":";  }
  else {     formattedTime=formattedTime+lhour+":"; }
  

  if(lminute<10)  {    formattedTime=formattedTime+"0"+lminute+":";  }
  else  {    formattedTime=formattedTime+lminute+":";  }

  if(lsecond<10)  {    formattedTime=formattedTime+"0"+lsecond; }
  else  {     formattedTime=formattedTime+lsecond;  }

  if(lIsPostMeridien)
  {
   formattedTime=formattedTime+" PM";
  }
  else
  {
    formattedTime=formattedTime+" AM";
  }
  
  return formattedTime;
}

String getTimeZone(long loffset)
{
  //Serial.print("Recieved Offset:");
  //Serial.println(loffset);
  String timeZone="";
  unsigned long offset=0;
  byte offsetHour;
  byte offsetMinute;
  if(loffset<0)
  {
    timeZone=timeZone+"-";
    offset=loffset*-1;
    
  }
  else
  {
    timeZone=timeZone+"+";
    offset=loffset;
    
  }
  //Serial.print("TimeZone: ");
  //Serial.println(timeZone);
  offsetHour=floor(offset/3600);

 // Serial.print("offsetHour: ");
  //Serial.println(offsetHour);
  offsetMinute=(offset%3600)/60;


 // Serial.print("offsetMinute: ");
 // Serial.println(offsetMinute);
  
  return timeZone+offsetHour+":"+offsetMinute;
  
}




