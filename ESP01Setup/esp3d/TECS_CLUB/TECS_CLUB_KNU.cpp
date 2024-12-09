#include "Arduino.h"
#include "config.h"

#ifdef TECS_CLUB_KNU_FEATURE
  #include "webinterface.h"
  #include <Arduino_JSON.h>
  #include <Wire.h>
  #include "TASK_SIMPLE.h"

  int LED_PINS[16] = {
    144, 145, 146, 147, 148, 149, 150, 151,
    152, 153, 154, 155, 156, 157, 158, 159
  };

  #define TRIG_PIN 1
  #define ECHO_PIN 3
  
  // LAST OF Arduino setup() Function
  void TECS_CLUB_KNU_1ST_INIT() {
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
  }

  // AFTER WEB SERVER READY!
  void TECS_CLUB_KNU_2ND_INIT() {
  }

  void DISTANCE_MEASURE(JSONVar& RES) {
    //*
    digitalWrite(TRIG_PIN, LOW);
    delay(2);
    digitalWrite(TRIG_PIN, HIGH);
    delay(10);
    digitalWrite(TRIG_PIN, LOW);
    float Length = pulseIn(ECHO_PIN, HIGH,30000);

    float Distance = ((float) (340 * Length) / 10000) / 2;
    //*/
    //float Distance = 3.14159;

    RES["DISTANCE"] = Distance;
  }

#ifdef TECS_CLUB_KNU_EVENT_FEATURE
  extern void WS_BROADCAST(JSONVar& args_JSON);
  void TECS_CLUB_KNU_EVENT() {
    JSONVar JSON_EVENT;
    JSON_EVENT["EVENT"] = "DISTANCE";
    DISTANCE_MEASURE(JSON_EVENT);
    WS_BROADCAST(JSON_EVENT);
  }

  void KNU_EVENT_SETUP() {
      TASK_SIMPLE::SETUP();
      TASK_SIMPLE::NEW_TASK(TECS_CLUB_KNU_EVENT,  15, 100,  1000);
  }
#endif


	bool KNU_EVENT_SKIP = false;

  void api_KNU(JSONVar& args_JSON) {
    JSONVar ARGS = args_JSON["ARGS"];
    JSONVar REQ  = ARGS;
    JSONVar RES  = args_JSON["RES"];;

    String uri = web_interface->web_server.urlDecode(web_interface->web_server.uri());
    REQ["URI"] = uri;
    REQ["Method"] = (web_interface->web_server.method() == HTTP_GET) ? "GET" : "POST";

    RES["STATUS"] = 200;
    RES["STATUS_HELP"] = "OK";

    String what = "";

    if (!ARGS.hasOwnProperty("WHAT")) {
      RES["STATUS"] = 400;
      RES["STATUS_HELP"] = "WHAT not specified!";
    } else {
      what = String((const char*) ARGS["WHAT"]);
      if (what == "LED") {
        if (ARGS.hasOwnProperty("N") && ARGS.hasOwnProperty("S")) {
          int LED_NO = atoi((const char*) ARGS["N"]);
          int ON_OFF = atoi((const char*) ARGS["S"]);
          digitalWrite(LED_PINS[LED_NO], ON_OFF > 0 ? LOW : HIGH);
        } else {
          RES["STATUS"] = 400;
          RES["STATUS_HELP"] = "S or N is not specified!";
        }
      } else if (what == "EVENT") {
        if (!KNU_EVENT_SKIP) {
          KNU_EVENT_SKIP = true;
          KNU_EVENT_SETUP();
        }
      } else if (what == "DISTANCE") {
        DISTANCE_MEASURE(RES);
      } else {
          RES["STATUS"] = 404;
          RES["STATUS_HELP"] = String("I don't know how to operate ") + what;
      }
    }

    // 최종 응답 데이터 포맷
    JSONVar response;
    response["URI"] = uri;
    response["Method"] = REQ["Method"];
    response["REQ"] = REQ;
    response["RES"] = RES;
    web_interface->web_server.send((int) RES["STATUS"], "application/json", JSON.stringify(response));
  }

#endif
