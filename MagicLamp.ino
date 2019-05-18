#include <Bridge.h>
#include <HttpClient.h>
#include <Process.h>
#include <Wire.h>
#include <Time.h>
#include <infineonrgb.h>
#include <stdlib.h>

String lon;
String lat;
float localSetTime;
float localRiseTime;
boolean daylight;

InfineonRGB LEDS;

void setup() {
    Bridge.begin();
    Serial.begin(9600);
    while(!Serial);
    LEDS.begin();		
    LEDS.SetDimmingLevel(0x0000);
    findLon();
    findLat();
    thisIsTheTime();
    getSunrise();
    getSunset();
}

void loop() {
  if(hour() == convertToTime(localSetTime).substring(0,2).toInt() && convertToTime(localSetTime).substring(3).toInt())
  {
    lightOn();
  }
  delay(500);
  if(hour() == convertToTime(localRiseTime).substring(0,2).toInt() && convertToTime(localRiseTime).substring(3).toInt())
  {
    lightOff();
  }
  delay(500);
}

String findLon() {
    HttpClient clientLon;
    clientLon.get("http://ip-api.com/csv/?fields=lon");
    
    while (clientLon.available()) {
       lon = clientLon.readString();
    }
    
    Serial.print("Lon: ");
    Serial.println(lon);
    
    return lon;
}

String findLat() {
    HttpClient clientLat;
    clientLat.get("http://ip-api.com/csv/?fields=lat");
    
    while (clientLat.available()) {
       lat = clientLat.readString();
    }
    
    Serial.print("Lat: ");
    Serial.println(lat);
    
    return lat;
}

void thisIsTheTime() {
    String DateL = "";
    String TimeL = "";
    
    Serial.print("Setting time zone... ");
    ProcExec("echo","'CET-1CEST,M3.5.0,M10.5.0/3' > /etc/TZ");
    Serial.println("done.");
 
    Serial.print("Syncing time with NTP... ");
    ProcExec("ntpd","-nqp 0.openwrt.pool.ntp.org");
    Serial.println("done.");
    Serial.println("");
 
    DateL = ProcExec("date","+%Y-%m-%d");
    Serial.print("Linino Date:");
    Serial.println(DateL);

    TimeL = ProcExec("date","+%T");  
    Serial.print("Linino Time:");
    Serial.println(TimeL);
 
    int dt[6];
    dt[0]=DateL.substring(0,4).toInt();
    dt[1]=DateL.substring(5,7).toInt();
    dt[2]=DateL.substring(8).toInt();
    dt[3]=TimeL.substring(0,2).toInt();
    dt[4]=TimeL.substring(3,5).toInt();
    dt[5]=TimeL.substring(6).toInt();
    
    setTime(dt[3], dt[4], dt[5], dt[2], dt[1], dt[0]);
    
    Serial.print("Date: ");
    Serial.print(month());
    Serial.print("/");
    Serial.print(day());
    Serial.print("/");
    Serial.println(year());
    Serial.print("Time: ");
    Serial.print(hour());
    Serial.print(":");
    Serial.print(minute());
    Serial.print(":");
    Serial.println(second());
}

String ProcExec(String Comm, String Par) {
   String Res = "";
   Process p;
   p.begin(Comm);
   p.addParameter(Par);
   p.run();

   while (p.available()>0) {
     char c = p.read();
     Res += c;
   }
   return Res;
}

float getSunrise() {
   isDaylightSavings();
  
   char longitude[20];
   char latitude[20];
   float longi, lati, A, B, C, E, F, JD, JC, longSun, anomSun, eOrbit, sunEqCtr, trueLongSun, trueAnomSun, sunRadVector, sunAppLong, obliqEcliptic, obliqCorr;
   float sunAscen, sunDeclin, varY, eqTime, haSunrise, solarNoon; 
   float D2R = PI / 180;
   float R2D = 180 / PI;
    
   lat.toCharArray(latitude, 20);
   lati = atof(latitude);
    
   lon.toCharArray(longitude, 20);
   longi = atof(longitude);
   
   A = year() / 100;
   B = A / 4;
   C = 2 - A + B;
   E = 365.25 * (year() + 4716);
   F = 30.6001 * (month() + 1);
   JD = C + day() + E + F - 1524.5;
   JC = (JD - 2451545) / 36525;
   longSun = int(280.46646 + JC * (36000.76983 + JC * 0.0003032)) % 360; //TODO: Non integer modulo
   anomSun = 357.52911 + JC * (35999.05029 - 0.0001537 * JC);
   eOrbit = 0.016708634 - JC * (0.000042037 + 0.0000001267 * JC);
   sunEqCtr = sin(D2R * (anomSun)) * (1.914602 - JC * (0.004817 + 0.000014 * JC)) + sin(D2R * (2 * anomSun)) * (0.019993 - 0.000101 * JC) + sin(D2R * (3 * anomSun)) * 0.000289;
   trueLongSun = longSun + sunEqCtr;
   trueAnomSun = anomSun + sunEqCtr;
   sunRadVector = (1.000001018 * (1 - eOrbit * eOrbit)) / (1 + eOrbit * cos(D2R * (trueAnomSun)));
   sunAppLong = trueLongSun - 0.00569 - 0.00478 * sin(D2R * (125.04 - 1934.136 * JC));
   obliqEcliptic = 23 + (26 + ((21.448 - JC * (46.815 + JC * (0.00059 - JC * 0.001813)))) / 60) / 60;
   obliqCorr = obliqEcliptic + 0.00256 * cos(D2R * (125.04 - 1934.136 * JC));
   sunAscen = R2D * (atan2(cos(D2R * (sunAppLong)), cos(D2R * (obliqCorr)) * sin(D2R * (sunAppLong))));
   sunDeclin = R2D * (asin(sin(D2R * (obliqCorr)) * sin(D2R * (sunAppLong))));
   varY = tan(D2R * (obliqCorr / 2)) * tan(D2R * (obliqCorr / 2));
   eqTime = 4 * R2D * (varY * sin(2 * D2R * (longSun)) - 2 * eOrbit * sin(D2R * (anomSun)) + 4 * eOrbit * varY * sin(D2R * (anomSun)) * cos(2 * D2R * (longSun)) - 0.5 * varY * varY * 
     sin(4 * D2R * (longSun)) - 1.25 * eOrbit * eOrbit * sin(2 * D2R * (anomSun)));
   haSunrise = R2D * (acos(cos(D2R * (90.833)) / (cos(D2R * (lati)) * cos(D2R * (sunDeclin))) - tan(D2R * (lati)) * tan(D2R * (sunDeclin))));
   if (daylight == true) {
     solarNoon = (720 - 4 * longi - eqTime + -5 * 60) / 1440; //TODO: Change -5 to a method for finding timezone
     solarNoon = solarNoon + .041667;
   }
   else {
     solarNoon = (720 - 4 * longi - eqTime + -5 * 60) / 1440; //TODO: Change -5 to a method for finding timezone
   }
   localRiseTime = (solarNoon * 1440 - haSunrise * 4) / 1440;
   
   Serial.print("Sunrise is at: ");
   Serial.println(convertToTime(localRiseTime));
   
   return localRiseTime;
}

float getSunset() {
   isDaylightSavings();
  
   char longitude[20];
   char latitude[20];
   float longi, lati, A, B, C, E, F, JD, JC, longSun, anomSun, eOrbit, sunEqCtr, trueLongSun, trueAnomSun, sunRadVector, sunAppLong, obliqEcliptic, obliqCorr;
   float sunAscen, sunDeclin, varY, eqTime, haSunrise, solarNoon; 
   float D2R = PI / 180;
   float R2D = 180 / PI;
    
   lat.toCharArray(latitude, 20);
   lati = atof(latitude);
    
   lon.toCharArray(longitude, 20);
   longi = atof(longitude);
   
   A = year() / 100;
   B = A / 4;
   C = 2 - A + B;
   E = 365.25 * (year() + 4716);
   F = 30.6001 * (month() + 1);
   JD = C + day() + E + F - 1524.5;
   JC = (JD - 2451545) / 36525;
   longSun = int(280.46646 + JC * (36000.76983 + JC * 0.0003032)) % 360; //TODO: Non integer modulo
   anomSun = 357.52911 + JC * (35999.05029 - 0.0001537 * JC);
   eOrbit = 0.016708634 - JC * (0.000042037 + 0.0000001267 * JC);
   sunEqCtr = sin(D2R * (anomSun)) * (1.914602 - JC * (0.004817 + 0.000014 * JC)) + sin(D2R * (2 * anomSun)) * (0.019993 - 0.000101 * JC) + sin(D2R * (3 * anomSun)) * 0.000289;
   trueLongSun = longSun + sunEqCtr;
   trueAnomSun = anomSun + sunEqCtr;
   sunRadVector = (1.000001018 * (1 - eOrbit * eOrbit)) / (1 + eOrbit * cos(D2R * (trueAnomSun)));
   sunAppLong = trueLongSun - 0.00569 - 0.00478 * sin(D2R * (125.04 - 1934.136 * JC));
   obliqEcliptic = 23 + (26 + ((21.448 - JC * (46.815 + JC * (0.00059 - JC * 0.001813)))) / 60) / 60;
   obliqCorr = obliqEcliptic + 0.00256 * cos(D2R * (125.04 - 1934.136 * JC));
   sunAscen = R2D * (atan2(cos(D2R * (sunAppLong)), cos(D2R * (obliqCorr)) * sin(D2R * (sunAppLong))));
   sunDeclin = R2D * (asin(sin(D2R * (obliqCorr)) * sin(D2R * (sunAppLong))));
   varY = tan(D2R * (obliqCorr / 2)) * tan(D2R * (obliqCorr / 2));
   eqTime = 4 * R2D * (varY * sin(2 * D2R * (longSun)) - 2 * eOrbit * sin(D2R * (anomSun)) + 4 * eOrbit * varY * sin(D2R * (anomSun)) * cos(2 * D2R * (longSun)) - 0.5 * varY * varY * 
     sin(4 * D2R * (longSun)) - 1.25 * eOrbit * eOrbit * sin(2 * D2R * (anomSun)));
   haSunrise = R2D * (acos(cos(D2R * (90.833)) / (cos(D2R * (lati)) * cos(D2R * (sunDeclin))) - tan(D2R * (lati)) * tan(D2R * (sunDeclin))));
   if (daylight == true) {
     solarNoon = (720 - 4 * longi - eqTime + -5 * 60) / 1440; //TODO: Change -5 to a method for finding timezone
     solarNoon = solarNoon + .041667;
   }
   else {
     solarNoon = (720 - 4 * longi - eqTime + -5 * 60) / 1440; //TODO: Change -5 to a method for finding timezone
   }
   localSetTime = (solarNoon * 1440 + haSunrise * 4) / 1440;
   
   Serial.print("Sunset is at: ");
   Serial.println(convertToTime(localSetTime));
   
   return localSetTime;
}

String convertToTime(float n) {
  float tmp;
  int newHour, newMinute, newSecond;
  String convertedTime;
  
  tmp = n * 24;
  newHour = floor(tmp);
  tmp = tmp - newHour;
  tmp = tmp * 60;
  newMinute = floor(tmp);
  if (newMinute < 10) {
    convertedTime = String(newHour) + ":0" + String(newMinute);
  }
  else {
    convertedTime = String(newHour) + ":" + String(newMinute);
  }
  
  return convertedTime;
}

boolean isDaylightSavings() {
  int marchSecondSunday, novFirstSunday, cent, i, n;
  
  cent = floor(year() / 100);
  
  for (i = 8; i <= 14; i++) { 
    if (marchSecondSunday != 0) {
      marchSecondSunday = int((i + floor(2.6 * 3 - 0.2) - 2 * cent + year() + floor(year() / 4) + floor(cent / 4))) % 7;
    }
    break;
  } 
  for (n = 1; n <= 7; n++) { 
    if (novFirstSunday != 0) {
      novFirstSunday = int((i + floor(2.6 * 11 - 0.2) - 2 * cent + year() + floor(year() / 4) + floor(cent / 4))) % 7;
    }
    break;
  } 
  
  if (month() == 3) {
   if (day() >= i) {
     daylight = true;
    }
  }
  else if (month() == 11) {
   if (day() <= n) {
    daylight = true;
   } 
  }
  else if (month() > 3) {
   daylight = true; 
  }
  else if (month() < 11) {
   daylight = true; 
  }
  else {
   daylight = false; 
  }
  
  return daylight;
}
void lightOn() {
  LEDS.SetIntensityRGB(0x0199, 0x0000, 0x0000);
  LEDS.SetDimmingLevel(0x0199);
  delay(200000);
  LEDS.SetDimmingLevel(0x0333);
  delay(200000);
  LEDS.SetDimmingLevel(0x04CC);
  delay(200000);
  LEDS.SetDimmingLevel(0x0666);
  delay(200000);
  LEDS.SetDimmingLevel(0x07FF);
  delay(200000);
  LEDS.SetDimmingLevel(0x0999);
  delay(200000);
  LEDS.SetDimmingLevel(0x0B32);
  delay(200000);
  LEDS.SetDimmingLevel(0x0CCC);
  delay(200000);
  LEDS.SetDimmingLevel(0x0E65);
  delay(200000);
  LEDS.SetDimmingLevel(0x0FFF);
}
void lightOff() {
  LEDS.SetIntensityRGB(0x0E65, 0x0000, 0x0000);
  LEDS.SetDimmingLevel(0x0E65);
  delay(200000);
  LEDS.SetDimmingLevel(0x0CCC);
  delay(200000);
  LEDS.SetDimmingLevel(0x0B32);
  delay(200000);
  LEDS.SetDimmingLevel(0x0999);
  delay(200000);
  LEDS.SetDimmingLevel(0x07FF);
  delay(200000);
  LEDS.SetDimmingLevel(0x0666);
  delay(200000);
  LEDS.SetDimmingLevel(0x04CC);
  delay(200000);
  LEDS.SetDimmingLevel(0x0333);
  delay(200000);
  LEDS.SetDimmingLevel(0x0199);
  delay(200000);
  LEDS.SetDimmingLevel(0x0000);
}
