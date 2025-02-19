#include <ESP8266WiFi.h>

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <Ticker.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <sunset.h>


#define DAY_BRIGHTNESS 128
#define NIGHT_BRIGHTNESS 24
int brightness = DAY_BRIGHTNESS;

String txt_time ;

struct tm tm;         // http://www.cplusplus.com/reference/ctime/tm/

// Credentials ----------------------------------------
const char* SSID = "xxxxxxxx";      // "mySSID";
const char* PW   = "xxxxxxxx";      // "myWiFiPassword";

// Timezone -------------------------------------------
#define TZName       "CET-1CEST,M3.5.0,M10.5.0/3"   // Berlin (examples see at the bottom)
#define FORMAT24H          // if not defined time will be displayed in 12h fromat


// Sunrise, Sunset -------------------------------------------
#define LONGITUDE 7.01
#define LATITUDE  43.5
#define TIMEZONE    +1

const char* NTP_SERVER[] = {"fr.pool.ntp.org", "at.pool.ntp.org", "europe.pool.ntp.org"};

// The object for the Ticker
Ticker tckr;

// date strings -------------------------------------------
String M_arr[12] = {"Jan.", "Feb.", "Mar.", "Apr.", "May", "June", "July", "Aug.", "Sep.", "Oct.", "Nov.", "Dec."};
String WD_arr[7] = {"Sun,", "Mon,", "Tue,", "Wed,", "Thu,", "Fri,", "Sat,"};

// Sunrise,Sunset palette -------------------------------------------
int Sunrise_RGB[]={255,0,0,
                   255,77,0,
                   255,103,0,
                   255,129,0,
                   255,167,0,
                   167,52,167,
                   167,167,167,
                   82, 167,167, 
                   42, 167,167,
                   0, 167, 255};

int Sunset_RGB[]={238,175,0 ,
                  251,144,98,
                  238,175,0,
                  251,144,98,
                  238,93,108,
                  206,73,147,
                  106,13,131,
                  54 ,13,131,
                   0 ,10,178,
                   0 ,10,255};


int SecondLine_RGB[]={64,64,64,
                      64,0,64,
                      64,0,80,
                      64,0,144,
                      0,128,0,
                      0,128,64,
                      0,128,80,
                      0,128,144,
                      128,0,0,
                      128,0,64,
                      128,0,80,
                      128,0,144,
                      128,128,0,
                      128,128,64,
                      128,128,80,
                      128,128,144};
                   
// LedMAtrix -------------------------------------------
#define PIN D6

#define PIXELPERCHAR 6
Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32,8, PIN,
                            NEO_MATRIX_TOP         + NEO_MATRIX_LEFT +
                            NEO_MATRIX_COLUMNS     + NEO_MATRIX_ZIGZAG,
                            NEO_GRB            + NEO_KHZ800);
                            
SunSet sun;
//----------------------------------------------------------------------------------------------------------------------

extern "C" uint8_t sntp_getreachability(uint8_t);

bool getNtpServer() { // connect WiFi -> fetch ntp packet -> disconnect Wifi
    uint8_t cnt = 0;
    WiFi.mode(WIFI_STA);
    WiFi.begin(SSID, PW);

    while(WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
        cnt++;
        if(cnt > 20)
            break;
    }

    if(WiFi.status() != WL_CONNECTED) return false;

    Serial.println("\nconnected with: " + WiFi.SSID());
    Serial.println("IP Address: " + WiFi.localIP().toString());
    bool timeSync;
    uint32_t timeout { millis() };
    
    configTime(0, 0, "fr.pool.ntp.org");  
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 0);
    delay(1000);
    
    do {
        delay(25);
        if(millis() - timeout >= 1e3) {
            Serial.printf("waiting for NTP %02ld sec\n", (millis() - timeout) / 1000);
            delay(975);
        }
        sntp_getreachability(0) ? timeSync = true : sntp_getreachability(1) ? timeSync = true :
        sntp_getreachability(2) ? timeSync = true : false;
    } while(millis() - timeout <= 16e3 && !timeSync);

    Serial.printf("NTP Synchronization %s!\n", timeSync ? "successfully" : "failed");
    time_t tnow = time(0);
    Serial.print(String(ctime(&tnow)));
    WiFi.disconnect();
    return timeSync;
}

void timer50ms() {

    static unsigned int cnt50ms = 0;
    static unsigned int cnt1s = 0;
    static unsigned int cnt1h = 0;
    cnt50ms++;
    if (cnt50ms == 20) {
        cnt1s++;
        cnt50ms = 0;
    }
    if (cnt1s == 3600) { // 1h
        cnt1h++;
        cnt1s = 0;
    }
    if (cnt1h == 24 && cnt1s == 50 ) { // 1d
        if (getNtpServer()) Serial.println("internal clock synchronized with ntp");
        else Serial.println("no daily timepacket received");
        cnt1h = 0;
    }
}

//Scrolling text 
//----------------------------------------------------------------------------------------------------------------------
void writeText(String msg) {
  int msgSize = (msg.length() * PIXELPERCHAR) + (2 * PIXELPERCHAR); // CACULATE message length;
  int scrollingMax = (msgSize) + matrix.width(); // ADJUST Displacement for message length;
  int x = matrix.width(); // RESET Cursor Position and Start Text String at New Position on the Far Right;

  matrix.fillScreen(0); // BLANK the Entire Screen;
  matrix.setCursor(x, 0); // Set Starting Point for Text String;
  matrix.print(msg); // Set the Message String;

 while(x>= -scrollingMax) {     
    matrix.fillScreen(0); // BLANK the Entire Screen;
    matrix.setCursor(x, 0); // Set Starting Point for Text String;
    matrix.print(msg); // Set the Message String;
    matrix.show(); // DISPLAY the Text/Image
    delay(40); // SPEED OF SCROLLING or FRAME RATE;
    x--;
    
  }
 }


//----------------------------------------------------------------------------------------------------------------------
// Display date 
void DisplayDate()
{           time_t tnow = time(0);
            localtime_r(&tnow, &tm);
            String txt= "   ";
            txt += WD_arr[tm.tm_wday] + " ";
            txt += String(tm.tm_mday) + ". ";
            txt += M_arr[tm.tm_mon] + " ";
            txt += String(tm.tm_year + 1900) + "   ";
            Serial.println(txt);
            writeText(txt);
}
//----------------------------------------------------------------------------------------------------------------------
void DisplaySunMoon()
{           
  char buffer[8];
  String txt= "   ";
  int sunrise;
  int sunset; 
  int moonphase;
  
  time_t tnow = time(0);
  localtime_r(&tnow, &tm);
  sun.setCurrentDate(tm.tm_year, tm.tm_mon+1,tm.tm_mday);
  sunrise=    sun.calcSunrise();
  sunset=     sun.calcSunset();
  moonphase=  sun.moonPhase(time(0));
  
  sprintf(buffer, "%02d:%02d", int(sunrise/60) , sunrise%60);  
  txt+= "Sunrise:" + String(buffer);
  
  sprintf(buffer, "%02d:%02d", int(sunset/60) , sunset%60);  
   txt+= " Sunset:" + String(buffer);
  
  txt+= " MoonPhase:"+ String(moonphase) +"%";
  Serial.println(txt);
  writeText(txt);
}


//----------------------------------------------------------------------------------------------------------------------

void setup() {
    Serial.begin(115200);
    delay(100);

    Serial.println("init..");
    if (!getNtpServer()){
        Serial.println("can't get time from NTP");
        while(1){;}  // endless loop, stop
    }
    Serial.println("NTP Done..");
    tckr.attach(0.05, timer50ms);    // every 50 msec


   sun.setPosition(LATITUDE, LONGITUDE, TIMEZONE);

   time_t tnow = time(0);
   localtime_r(&tnow, &tm);
   sun.setCurrentDate(tm.tm_year, tm.tm_mon+1,tm.tm_mday);

   Serial.println("Sunrise:" + String(sun.calcSunrise()));
   Serial.println("Sunset: " + String(sun.calcSunset()));
   Serial.println("MoonPhase:"+ String(sun.moonPhase(time(0))));
   
   randomSeed(analogRead(0));
   matrix.begin();
   matrix.setTextWrap(false);
   matrix.setBrightness(brightness);
   matrix.setTextColor(matrix.Color(0, brightness>>1, brightness));

}


//----------------------------------------------------------------------------------------------------------------------
// Set text color according to time of the day and week 
// returns brightnss for seconds tick 
int setColor(struct tm &tm)
{
  int time_mins;
  int col_index;
  int brightness;
  int sunrise;
  int sunset; 
  
  sun.setCurrentDate(tm.tm_year, tm.tm_mon+1,tm.tm_mday);
  sunrise=    sun.calcSunrise();
  sunset=     sun.calcSunset();

  time_mins= tm.tm_hour*60+tm.tm_min;
  brightness = DAY_BRIGHTNESS;
  matrix.setTextColor(matrix.Color(0,DAY_BRIGHTNESS>>1,DAY_BRIGHTNESS));
  
    // dawn : dark blue 
  if (abs(time_mins-sunset)>20)
  {
     matrix.setTextColor(matrix.Color(0,0,100));
     brightness = 100;
  }
  
  // night /day color switch   
  if ((tm.tm_wday==0) || (tm.tm_wday==6))   // weekends 
  {
  
      if (tm.tm_hour>9 && tm.tm_hour<23)
      {
          matrix.setTextColor(matrix.Color(0,DAY_BRIGHTNESS>>1,DAY_BRIGHTNESS));
          brightness = 128;
      }
      else                           
      {
          matrix.setTextColor(matrix.Color(NIGHT_BRIGHTNESS,0,0));
          brightness = NIGHT_BRIGHTNESS;
      }
  }
   else          
   {   
      if (tm.tm_hour>7 && tm.tm_hour<23)
      {
          matrix.setTextColor(matrix.Color(0,DAY_BRIGHTNESS>>1,DAY_BRIGHTNESS));
          brightness = DAY_BRIGHTNESS;
      }
      else                           
      {
          matrix.setTextColor(matrix.Color(NIGHT_BRIGHTNESS,0,0));
          brightness = NIGHT_BRIGHTNESS;
      }
   }
 
     
  // sunrise 
  if (abs(time_mins-sunrise)<20)
  {
    col_index= 5+((time_mins-sunrise) /4);
    brightness = (Sunrise_RGB[3*col_index]+Sunrise_RGB[3*col_index+1]+Sunrise_RGB[3*col_index+2]) /3;
    matrix.setTextColor(matrix.Color(Sunrise_RGB[3*col_index]>>1,Sunrise_RGB[3*col_index+1]>>1,Sunrise_RGB[3*col_index+2]>>1));
  }
  
  // sunset 
  if (abs(time_mins-sunset)<20)
  {
    brightness =(Sunset_RGB[3*col_index]+Sunset_RGB[3*col_index+1]+Sunset_RGB[3*col_index+2]) /3;
    col_index= 5+((time_mins-sunset)/4 );
    matrix.setTextColor(matrix.Color(Sunset_RGB[3*col_index]>>1,Sunset_RGB[3*col_index+1]>>1,Sunset_RGB[3*col_index+2]>>1));
  }


  return(brightness);
}

//----------------------------------------------------------------------------------------------------------------------



void loop() {
    // color according to sunrise , sunset 
    // print sunrise, sunset 
    // get weather + temp 
    // print date 
    char buffer[8];
    int col_index; 
    time_t now = time(&now);
    localtime_r(&now, &tm);

    if (tm.tm_sec==0)
        brightness = setColor(tm);

    if ((tm.tm_sec==10) && (brightness==DAY_BRIGHTNESS))
      if ((tm.tm_min %2 ==0))
        DisplayDate();
      else
        DisplaySunMoon();

    sprintf(buffer, "%02d:%02d", tm.tm_hour,tm.tm_min);
    txt_time=String(buffer);    
    //txt_time= String(tm.tm_hour)+":"+String(tm.tm_min);
    
    matrix.fillScreen(matrix.Color(0, 0, 0));
    matrix.setCursor(2, 0);
    // matrix.setTextColor(matrix.Color(0, brightness>>1, brightness));
    matrix.print(txt_time);  

    col_index = (tm.tm_min % 16);
    
    // Seconds line     
    if (brightness==DAY_BRIGHTNESS)
      matrix.drawLine(0, 7, (tm.tm_sec >>1),7, matrix.Color(SecondLine_RGB[3*col_index],SecondLine_RGB[3*col_index+1],SecondLine_RGB[3*col_index+2]));
    else 
      matrix.drawLine(0, 7, (tm.tm_sec >>1),7, matrix.Color(brightness, 0, 0));
      
    matrix.show();
    delay(100);

}
