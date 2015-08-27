#include <LibAPRS.h>
#include <string.h>
#include <U8glib.h>
#include <TinyGPS++.h>

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);

TinyGPSPlus gps;

unsigned int txCounter = 1; //First TX in Setup = 1!
unsigned long lastTx = 0;

float lat = 0.00;
float lon = 0.00;
float wayPointLatitude, wayPointLongitude;
float latitudeRadians, wayPointLatitudeRadians, longitudeRadians, wayPointLongitudeRadians;
float distanceToWaypoint, bearing, deltaLatitudeRadians, deltaLongitudeRadians;
const float pi = 3.14159265;
const int radiusOfEarth = 6371; // in km
char nw, wl;
boolean gotPacket = false;
AX25Msg incomingPacket;
uint8_t *packetData;
float lastTxLat = lat;
float lastTxLng = lon;
float lastTxdistance = 0.0;
int previousHeading = 400;
int lastbearing =0;

int headingDelta, lastheadingDelta = 0;

byte hour = 0, minute = 0, second = 0;
unsigned long secondtimer = 0;

#define ADC_REFERENCE REF_3V3
// OR
//#define ADC_REFERENCE REF_5V

// You can also define whether your modem will be
// running with an open squelch radio:
#define OPEN_SQUELCH false


void aprs_msg_callback(struct AX25Msg *msg) {
  if (!gotPacket) {
    gotPacket = true;
    memcpy(&incomingPacket, msg, sizeof(AX25Msg));

    if (freeMemory() > msg->len) {
      packetData = (uint8_t*)malloc(msg->len);
      memcpy(packetData, msg->info, msg->len);
      incomingPacket.info = packetData;
    } 
    else {
      gotPacket = false;
    }
  }
}

void setup() {

  Serial.begin(9600);
  delay(200);
  Serial.println("AT+DMOCONNECT");
  delay(200);
  Serial.println("AT+DMOSETGROUP=1,144.3900,144.3900,0000,1,0000");
  delay(200);
  Serial.println("AT+SETFILTER=0,1,0");
  delay(200);
  Serial.println("AT+DMOSETVOLUME=8");

  // Initialise APRS library - This starts the modem
  APRS_init(ADC_REFERENCE, OPEN_SQUELCH);

  // You must at a minimum configure your callsign and SSID
  APRS_setCallsign("WA6MAT", 9);

  // You can define preamble and tail like this:
  APRS_setPreamble(550);
  APRS_setTail(250);

  // You can use the normal or alternate symbol table:
  // APRS_useAlternateSymbolTable(false);

  // And set what symbol you want to use:
  APRS_setSymbol('>');
      char *comment = "";
//    APRS_sendLoc(comment, strlen(comment));
//  APRS_sendPkt(comment, strlen(comment));

}


void TxtoRadio()
{
  // Only send status/version every 10 packets to save packet size
  if ( txCounter % 10 == 0 )
  {
    // We'll define a comment string
    char *comment = "VST v2 Run Forrest Run";
    APRS_sendLoc(comment, strlen(comment));
  } 
  else {

    char latOut[15], lngOutTmp[15];
    float latDegMin, lngDegMin = 0.0;


    latDegMin = convertDegMin(lat);
    lngDegMin = convertDegMin(lon);

    dtostrf(latDegMin, 2, 2, latOut );
    //latOut=tmp;

    dtostrf(lngDegMin, 2, 2, lngOutTmp );
    //lngOutTmp=tmp1;

    char lngOut[15] = {
    };
    int cur_len = strlen(latOut);
    latOut[cur_len] = nw;
    latOut[cur_len+1] = '\0';
    Serial.println(lngDegMin);
    delay(200);
//    Serial.println(lngDegMin);
//    delay(200);
    if(lngDegMin < 10000)
    {
      int n = strlen(lngOutTmp);

      lngOut[0] = '0';

      for(int i = 1; i < n + 1; i++)
      {
        lngOut[i] = lngOutTmp[i-1];
      }

      cur_len = strlen(lngOut);
      lngOut[cur_len] = '\0';
    } 
    else {
      strncpy(lngOut, lngOutTmp, 13); 
    }
    Serial.println(lngOutTmp);
    delay(200);
    Serial.println(lngOut);
    delay(200);
    lngOut[cur_len] = wl;
    lngOut[cur_len+1] = '\0';  
    strcat (lngOut,"W");
    // And send the update

   APRS_setLat(latOut);
   APRS_setLon(lngOut);


    // And send the update
    char *comment = "VST v2 Run Forrest Run";
    APRS_sendLoc(comment, strlen(comment));
    
    // Reset the Tx internal
    lastTxdistance = 0;   // Ensure this value is zero before the next Tx
    lastTxLat = lat;
    lastTxLng = lon;
    previousHeading = lastbearing;
  }
  lastTx = millis();
  txCounter++;

} // endof TxtoRadio()


void processPacket() {
  if (gotPacket) {
    gotPacket = false;
    free(packetData);
  }
}

boolean whichExample = false;

void loop() {

  if (millis() - secondtimer >= 1000)
  {
    second++;
    secondtimer = millis();

    if (second == 60)
    {
      second = 0;
      minute++;
      if (minute == 60)
      {
        minute = 0;
        hour++;
        if (hour == 24)
        {
          hour = 0;
        }
      }
    }
  }

  u8g.setFont(u8g_font_5x8);
  u8g.setColorIndex(255);

  while (Serial.available() > 0)
  {
    gps.encode(Serial.read());
  }


  if ( gps.time.isUpdated() )
  {
    hour = gps.time.hour();
    minute = gps.time.minute();
    second = gps.time.second();
  }

  ///////////////// Triggered by location updates ///////////////////////
  if ( gps.location.isUpdated() )
  {
    lastTxdistance = TinyGPSPlus::distanceBetween(
    gps.location.lat(),
    gps.location.lng(),
    lastTxLat,
    lastTxLng);

    lastbearing = (int)TinyGPSPlus::courseTo(
    lastTxLat,
    lastTxLng,
    gps.location.lat(),
    gps.location.lng());

    lat = gps.location.lat();
    lon = gps.location.lng();

    nw = gps.location.rawLat().negative ? 'S' : 'N';
    wl = gps.location.rawLng().negative ? 'E' : 'W';
    // Get headings and heading delta

    headingDelta = abs(lastbearing - (int)gps.course.deg());
    if (headingDelta  > 180){
      headingDelta  = 360 - headingDelta;
    }

    lastheadingDelta = abs(lastbearing - previousHeading);
    if (lastheadingDelta > 180){
      lastheadingDelta = 360 - lastheadingDelta;
    }

  } // endof gps.location.isUpdated()


  float Volt = (float) readVcc()/1000;

  long latt, lonn;

  lonn = (lon*100000) + 18000000; // Step 1
  latt = (lat*100000) + 9000000; // Adjust so Locn AA is at the pole
  char MH[6] = {
    'A', 'A', '0', '0', 'a', 'a'  }; // Initialise our print string
  MH[0] += lonn / 2000000; // Field
  MH[1] += latt / 1000000;
  MH[2] += (lonn % 2000000) / 200000; // Square
  MH[3] += (latt % 1000000) / 100000;
  MH[4] += (lonn % 200000) / 8333; // Subsquare .08333 is 5/60
  MH[5] += (latt % 100000) / 4166; // .04166 is 2.5/60
  String MH_txt = ""; // Build up Maidenhead
  int i = 0; // into a string that's easy to print
  while (i < 6){
    MH_txt += MH[i];
    i++; 
  }

  if (!gotPacket)
  {
    u8g.firstPage();
    do
    {
      u8g.setPrintPos(0, 9);
      printTime();

      u8g.setPrintPos(65, 9);
      printDate();

      u8g.setPrintPos(0, 18);
      u8g.print("SAT's: ");
      u8g.print(gps.satellites.value());
      u8g.setPrintPos(65, 18);
      u8g.print(gps.speed.kmph(), 0);
      u8g.print(" Km/H");

      u8g.setPrintPos(0, 27);
      u8g.print("LAT: ");
      u8g.print(lat);
      u8g.setPrintPos(65, 27);
      u8g.print("LON: ");
      u8g.print(lon);
      u8g.setPrintPos(0, 36);
      u8g.print("Volts: ");
      u8g.print(Volt);
      u8g.setPrintPos(65, 36);
      u8g.print("Grid: ");
      u8g.print(MH_txt);
    }
    while( u8g.nextPage() );
  } 
  else {
    gotPacket = false;
    //message was received
    u8g.firstPage();
    do
    {
      u8g.setPrintPos(0,9);
      u8g.print("S: ");
      u8g.print(incomingPacket.src.call);
      u8g.print("-");
      u8g.print(incomingPacket.src.ssid);
      u8g.setPrintPos(75, 9);
      u8g.print("R: ");
      u8g.print(incomingPacket.dst.call);
      u8g.print("-");
      u8g.print(incomingPacket.dst.ssid);
      int h = 18;
      int n = 0;
      u8g.setPrintPos(0, h);
      for (int i = 0; i < incomingPacket.len; i++) {
         
        if(n==24){
          h = h + 9;
          n = 0;
          u8g.setPrintPos(0, h);
        }
       u8g.print(incomingPacket.info[i]);
        
         n++;
      }
       free(packetData);
       delay(5000);
    }while(u8g.nextPage());
   
  } 

 
  // Only check the below if locked satellites > 3
  if ( gps.satellites.value() >= 3 && (millis()-lastTx) > 5000 && gps.hdop.value() < 500 )
  {
    // Check for heading between 70 and 95degrees in more than 50m and previousHeading is defined (for left or right turn)
    if ( headingDelta > 70 && headingDelta < 95 && lastheadingDelta > 50 && lastTxdistance > 50 && previousHeading != 400)
    {
      TxtoRadio();
    } // endif headingDelta


    // if headingdelta < 95 deg last Tx distance is more than 100m+ 4 x Speed lower than 60 KM/h OR lastdistance is more than 600m
    // if more than 110 deg + 500 meter
    else if ( (headingDelta < 95 && lastTxdistance > ( 100 + (gps.speed.kmph()*4) + gps.hdop.value()) && gps.speed.kmph() < 60) || (lastTxdistance > 600  && gps.speed.kmph() < 60))
    {
      TxtoRadio();

    }

    // lastTxdistance >= 10 x Speed in Km/H and speed more than 60 Km/H = minimum 600m
    else if ( lastTxdistance > (gps.speed.kmph()*10) && gps.speed.kmph() >= 60 )
    {
      TxtoRadio();

    }

    else if ( (millis()-lastTx) > 30000) // every 30 seconds
    {
      TxtoRadio();
      txCounter++;
      lastTx = millis();
    } // endif of check for lastTx > txInterval

  } // Endif check for satellites

}
// convert degrees to radians
void radianConversion()
{
  deltaLatitudeRadians = (wayPointLatitude - lat) * pi / 180;
  deltaLongitudeRadians = (wayPointLongitude - lon) * pi / 180;
  latitudeRadians = lat * pi / 180;
  wayPointLatitudeRadians = wayPointLatitude * pi / 180;
  longitudeRadians = lon * pi / 180;
  wayPointLongitudeRadians = wayPointLongitude * pi / 180;
}

// calculate distance from present location to next way point
float calculateDistance()
{
  radianConversion();
  float a = sin(deltaLatitudeRadians / 2) * sin(deltaLatitudeRadians / 2) +
    sin(deltaLongitudeRadians / 2) * sin(deltaLongitudeRadians / 2) *
    cos(latitudeRadians) * cos(wayPointLatitudeRadians);
  float c = 2 * atan2(sqrt(a), sqrt(1 - a));
  float d = radiusOfEarth * c;
  return d * 1;                  // distance in kilometers
}

// calculate bearing from present location to next way point
float calculateBearing()
{
  radianConversion();
  float y = sin(deltaLongitudeRadians) * cos(wayPointLatitudeRadians);
  float x = cos(latitudeRadians) * sin(wayPointLatitudeRadians) -
    sin(latitudeRadians) * cos(wayPointLatitudeRadians) * cos(deltaLongitudeRadians);
  bearing = atan2(y, x) / pi * 180;
  if(bearing < 0)
  {
    bearing = 360 + bearing;
  }
  return bearing;
}



bool startsWith(const char *pre, const char *str)
{
  size_t lenpre = strlen(pre),
  lenstr = strlen(str);
  return lenstr < lenpre ? false : strncmp(pre, str, lenpre) == 0;
}

float convertDegMin(float decDeg)
{
  float DegMin;
  int intDeg = decDeg;
  decDeg -= intDeg;
  decDeg *= 60;
  DegMin = ( intDeg * 100 ) + decDeg;
  return DegMin;
}


/*
String padding( int number, byte width )
 {
   String result;
 
  // Prevent a log10(0) = infinity
  int temp = number;
  if (!temp)
  {
    temp++;
  }
 
  for ( int i = 0; i < width - (log10(temp)) - 1; i++)
  {
    result.concat('0');
  }
  result.concat(number);
  return result;
 }
 */

float conv_coords(float in_coords)
{
  //Initialize the location.
  float f = in_coords;
  // Get the first two digits by turning f into an integer, then doing an integer divide by 100;
  // firsttowdigits should be 77 at this point.
  int firsttwodigits = ((int)f) / 100;                             //This assumes that f < 10000.
  float nexttwodigits = f - (float)(firsttwodigits * 100);
  float theFinalAnswer = (float)(firsttwodigits + nexttwodigits / 60.0);
  return theFinalAnswer;
}

static void printDate()
{
  if (gps.date.day() < 10)
    u8g.print("0");
  u8g.print(gps.date.day());
  u8g.print(".");
  if (gps.date.month() < 10)
    u8g.print("0");
  u8g.print(gps.date.month());
  u8g.print(".");
  u8g.print(gps.date.year());
}

static void printTime()
{
  if (hour < 10)
    u8g.print("0");
  u8g.print(hour);
  u8g.print(":");
  if (minute < 10)
    u8g.print("0");
  u8g.print(minute);
  u8g.print(":");
  if (second < 10)
    u8g.print("0");
  u8g.print(second);
}


long readVcc() {                 
  long result;  
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);                     // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV  
  return result;
}

