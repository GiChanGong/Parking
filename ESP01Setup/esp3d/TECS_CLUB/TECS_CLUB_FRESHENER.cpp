#include "Arduino.h"
#include "config.h"

#ifdef TECS_CLUB_FRESHENER
	#include "TASK_SIMPLE.h"
	#include <Arduino_JSON.h>

	#ifdef ESP_OLED_FEATURE
	#include "esp_oled.h"
	#endif

	#ifdef OLED_DISPLAY_SSD1306
	#include "SSD1306.h"   // alias for `#include "SSD1306Wire.h"`
	#elif defined OLED_DISPLAY_SH1106
	#include "SH1106.h"  // alias for `#include "SH1106Wire.h"`
	#endif

	#include "wificonf.h"
	#include "espcom.h"

	int SPRAY_ON_VALUE = HIGH;
	int SPRAY_ON_SERVOR = 180;
	int SPRAY_OFF_SERVOR = 0;

	char STR_MESSAGE[256];

	#ifdef FRESHENER_ESP_32
		#include <ESP32Servo.h>
		Servo SPRAY_SERVO;

		#define PIN_PW_OLED		17 // OLED POWER
		#define PIN_PW_SERVO	16 // SERVO POWER
		#define PIN_SPRAY0		15 // 자동분사
		#define PIN_SERVO		14 // SERVO
		#define PIN_NOSLEEP		13 // NOSLEEP S/W
		#define PIN_PIR			12 // PIR 센서 0
	#else
		#define PIN_PIR		 3 // PIR 센서
		#define PIN_SPRAY0	 1 // 자동분사
	#endif


	unsigned long trySyncNTPAndGetCurrentEpoch();
	extern String getCurrentTimeAsString();
	extern String epochToTimeString(unsigned long epoch);

	String STR_DEFALULT_CONFIG =  R"({
			"PIR_CONFIRM_COUNT": 3,
			"PIR_ON_KEEP_TIME": 2,
			"SPRAY_CYCLE_TIME": 300,
			"SPRAY_WAKE_BEFORE": 5,
			"SPRAY_WAKE_TIME": 30,
			"SPRAY_DURATION": 250,
			"OLED_DURATION": 600,
			"SPRAY_ON_VALUE" : 1,
			"NO_SPRAY_TIMES" : [],
			"ON_SPRAY_TIMES" : []
		})";
	
	JSONVar FRESHENER_CONFIG;

	extern SSD1306  esp_display;
	extern JSONVar readConfigFile();
	extern void saveConfigFile(JSONVar& configData);
	extern void WS_BROADCAST(JSONVar& args_JSON);

	unsigned long OLED_ON_TIME		  = 0;
	unsigned long OLED_ON_TIME_MILLIS = 0;
	bool	OLED_ON_STATE = true;

	void OLED_ON_OFF(bool OLED_ON) {
		if (OLED_ON) {
			esp_display.displayOn();
			OLED_ON_STATE = true;
			OLED_ON_TIME_MILLIS = millis();    // OLED가 켜진 시간을 기록
			OLED_ON_TIME		= trySyncNTPAndGetCurrentEpoch();
		} else {
			esp_display.displayOff();
			OLED_ON_STATE = false;
		}
	}

	extern void WS_SEND_FRESHENER_STATUS();
	void DISP_CLOCK() {
		if (!OLED_ON_STATE) return; // OLED가 꺼져 있으면 아무일도 하지 않음
		String STR_TIMESTAMP = getCurrentTimeAsString();
		OLED_DISPLAY::display_text(STR_TIMESTAMP.c_str(), 84, 16, 128);
	}

	unsigned long SPRAY_ON_TIME   = 0;
	unsigned long SPRAY_ON_MILLIS = 0;
	bool SPRAY_POSTPONED = false;
	bool PIR_STATE_ON = false;

	extern void FRESHENER_ON_OFF(bool ON_OFF);

	bool NOSLEEP_MODE_ON = false;
	#ifdef FRESHENER_ESP_32
	int  NOSLEEP_LAST_VALUE = HIGH;
	void FRESHENER_NOSLEEP_TASK() {
		int NOSLEEP_CURR_VALUE = digitalRead(PIN_NOSLEEP);
		if (NOSLEEP_CURR_VALUE != NOSLEEP_LAST_VALUE) {
			NOSLEEP_LAST_VALUE = NOSLEEP_CURR_VALUE;
			if (NOSLEEP_CURR_VALUE == HIGH) {
				NOSLEEP_MODE_ON = true;
			} else {
				NOSLEEP_MODE_ON = false;
			}
			WS_SEND_FRESHENER_STATUS();
		}
	}
	#endif

	void SET_FRESHENER_STATUS(JSONVar& JSON_STATUS) {
		//*
		JSON_STATUS["DEVICE_TIME"] = getCurrentTimeAsString();
		JSON_STATUS["LAST_SPRAY_TIME"] = epochToTimeString(SPRAY_ON_TIME);
		long SPRAY_CYCLE_TIME = int(FRESHENER_CONFIG["SPRAY_CYCLE_TIME"]); // 초
		JSON_STATUS["NEXT_SPRAY_TIME"] = epochToTimeString(SPRAY_ON_TIME + SPRAY_CYCLE_TIME);
		JSON_STATUS["OLED_ON_TIME"] = epochToTimeString(OLED_ON_TIME);
		long OLED_DURATION = int(FRESHENER_CONFIG["OLED_DURATION"]); // 초
		JSON_STATUS["OLED_AUTO_OFF_TIME"] = epochToTimeString(OLED_ON_TIME + OLED_DURATION);
		JSON_STATUS["SPRAY_POSTPONED"] = SPRAY_POSTPONED;
		JSON_STATUS["PIR"] = PIR_STATE_ON;
		JSON_STATUS["NOSLEEP"] = NOSLEEP_MODE_ON;
		JSON_STATUS["SPRAY_ON_VALUE"] = SPRAY_ON_VALUE;
		//*/
		JSON_STATUS["CONFIG"] = FRESHENER_CONFIG;
	}

	void WS_SEND_FRESHENER_STATUS() {
		JSONVar JSON_EVENT = JSONVar();
		SET_FRESHENER_STATUS(JSON_EVENT);
		WS_BROADCAST(JSON_EVENT);
	}

	// ON_SPRAY_TIMES에 해당하는지 확인하는 함수
	bool isOnSprayTime(int hour, int minute, int second) {
		int size = FRESHENER_CONFIG["ON_SPRAY_TIMES"].length();
		for (int i = 0; i < size; i++) {
			String sprayTime = String((const char*)FRESHENER_CONFIG["ON_SPRAY_TIMES"][i]);
			int sprayHour = sprayTime.substring(0, 2).toInt();
			int sprayMinute = sprayTime.substring(3, 5).toInt();

			if (hour == sprayHour && minute == sprayMinute && second == 0) {
				return true;
			}
		}
		return false;
	}

	// NO_SPRAY_TIMES에 해당하는지 확인하는 함수
	bool isNoSprayTime(int hour, int minute) {
		int size = FRESHENER_CONFIG["NO_SPRAY_TIMES"].length();
		for (int i = 0; i < size; i++) {
			String noSprayTimeRange = String((const char*)FRESHENER_CONFIG["NO_SPRAY_TIMES"][i]);
			int startHour = noSprayTimeRange.substring(0, 2).toInt();
			int startMinute = noSprayTimeRange.substring(3, 5).toInt();
			int endHour = noSprayTimeRange.substring(8, 10).toInt();
			int endMinute = noSprayTimeRange.substring(11, 13).toInt();

			if ((hour > startHour || (hour == startHour && minute >= startMinute)) &&
				(hour < endHour || (hour == endHour && minute <= endMinute))) {
				return true;
			}
		}
		return false;
	}

	#ifdef FRESHENER_ESP_32
	void DEEP_SLEEP_START() {

		if (digitalRead(PIN_NOSLEEP) == LOW) {
			sprintf(STR_MESSAGE, "PIN_NOSLEEP S/W SET CASE!");
			ESPCOM::println (STR_MESSAGE, SERIAL_PIPE);
			return;
		}

//		esp_display.displayOff();  // OLED 디스플레이 끄기
		digitalWrite(PIN_PW_OLED, LOW);		// OLED  전원 차단
		digitalWrite(PIN_PW_SERVO, LOW);	// SERVO 전원 차단
		delay(500);

		unsigned long DEEP_SLEEP_TIME = int(FRESHENER_CONFIG["SPRAY_CYCLE_TIME"])
										- int(FRESHENER_CONFIG["SPRAY_WAKE_BEFORE"])
										- int(FRESHENER_CONFIG["SPRAY_WAKE_TIME"]);
										
		unsigned long DEEP_SLEEP_MICRO_SEC = DEEP_SLEEP_TIME * 1000000; // 초 -> 마이크로초로 변환

		sprintf(STR_MESSAGE, "deepSleep start : %ld secs\n", DEEP_SLEEP_TIME);
		ESPCOM::println (STR_MESSAGE, SERIAL_PIPE);
		// DEEP_SLEEP_TIME 동안 deep sleep 상태로 진입
		esp_sleep_enable_timer_wakeup(DEEP_SLEEP_MICRO_SEC); // 마이크로초 단위
		esp_deep_sleep_start();
	}
	#endif

	void FRESHENER_MAIN_TASK() {
		if (SPRAY_ON_MILLIS <= 0) SPRAY_ON_MILLIS = millis();
		unsigned long currentTime = millis();
		// FRESHENER_CONFIG에서 설정된 SPRAY_CYCLE_TIME (초 단위)
		unsigned long SPRAY_TRIGGER_TIME;
		unsigned long SPRAY_CYCLE_TIME = long(FRESHENER_CONFIG["SPRAY_CYCLE_TIME"]) * 1000; // 초 -> 밀리초로 변환

	#ifdef FRESHENER_ESP_32
		unsigned long SPRAY_WAKE_TIME  = int(FRESHENER_CONFIG["SPRAY_WAKE_TIME"]) * 1000; // 초 -> 밀리초로 변환
		if (digitalRead(PIN_NOSLEEP) == LOW) {
			SPRAY_TRIGGER_TIME = SPRAY_CYCLE_TIME;
		} else {
			SPRAY_TRIGGER_TIME = SPRAY_WAKE_TIME;
		}
	#else
			SPRAY_TRIGGER_TIME = SPRAY_CYCLE_TIME;
	#endif

		// NTP와 동기화된 현재 시간 가져오기
		unsigned long epochTime = trySyncNTPAndGetCurrentEpoch();
		String currentTimeString = epochToTimeString(epochTime);

		// 시/분/초로 나누기 위한 변수
		int currentHour = currentTimeString.substring(0, 2).toInt();
		int currentMinute = currentTimeString.substring(3, 5).toInt();
		int currentSecond = currentTimeString.substring(6, 8).toInt();

		// NO_SPRAY_TIMES에 해당하는 시간 구간이면 return 처리
		if (isNoSprayTime(currentHour, currentMinute)) {
			#ifdef FRESHENER_ESP_32
				sprintf(STR_MESSAGE, "isNoSprayTime true CASE!");
				ESPCOM::println (STR_MESSAGE, SERIAL_PIPE);
				DEEP_SLEEP_START();
			#endif
			return;
		}

		// ON_SPRAY_TIMES에 해당하는 시각에만 스프레이 작동
		if (isOnSprayTime(currentHour, currentMinute, currentSecond)) {
			if (PIR_STATE_ON) {
				SPRAY_POSTPONED = true;
				FRESHENER_ON_OFF(false);
			} else {
				FRESHENER_ON_OFF(true);
				#ifdef FRESHENER_ESP_32
					sprintf(STR_MESSAGE, "isOnSprayTime true CASE!");
					ESPCOM::println (STR_MESSAGE, SERIAL_PIPE);
				#endif
			}
		} else if ((currentTime - SPRAY_ON_MILLIS) >= SPRAY_TRIGGER_TIME) {
			if (PIR_STATE_ON) {
				SPRAY_POSTPONED = true;
				FRESHENER_ON_OFF(false);
				sprintf(STR_MESSAGE, "WAKE -> PIR    DETECT CASE!");
			} else {
				FRESHENER_ON_OFF(true);
				sprintf(STR_MESSAGE, "WAKE -> PIR NO DETECT CASE!");
			}
			#ifdef FRESHENER_ESP_32
				ESPCOM::println (STR_MESSAGE, SERIAL_PIPE);
			#endif
		}

		// PIR 상태가 OFF일 때 연기된 스프레이 실행
		if (!PIR_STATE_ON) {
			if (SPRAY_POSTPONED) {
				SPRAY_POSTPONED = false;
				FRESHENER_ON_OFF(true);
				#ifdef FRESHENER_ESP_32
					sprintf(STR_MESSAGE, "WAKE -> SPRAY_POSTPONED CASE!");
					ESPCOM::println (STR_MESSAGE, SERIAL_PIPE);
				#endif
			}
		}
	}


	unsigned long FRESHENER_ON_TIME = 0;
	bool	FRESHENER_ON_STATE = false;
	void FRESHENER_ON_OFF(bool ON_OFF) {
		SPRAY_ON_VALUE = int(FRESHENER_CONFIG["SPRAY_ON_VALUE"]);
		if (ON_OFF) {
			#ifdef FRESHENER_ESP_32
				digitalWrite(PIN_PW_SERVO, HIGH);
				SPRAY_SERVO.write(SPRAY_ON_SERVOR);
			#endif
			digitalWrite(PIN_SPRAY0, SPRAY_ON_VALUE);
			FRESHENER_ON_TIME = millis();
			FRESHENER_ON_STATE = true;
		} else {
			digitalWrite(PIN_SPRAY0, !SPRAY_ON_VALUE);
			#ifdef FRESHENER_ESP_32
				SPRAY_SERVO.write(SPRAY_OFF_SERVOR);
			#endif
			FRESHENER_ON_STATE = false;
		}
		SPRAY_ON_TIME   = trySyncNTPAndGetCurrentEpoch();
		SPRAY_ON_MILLIS = millis();
		WS_SEND_FRESHENER_STATUS();
	}

	void FRESHENER_AUTO_OFF() {
		if (!FRESHENER_ON_STATE) return; // FRESHENER가 꺼져 있으면 아무일도 하지 않음
		// millis()는 시스템이 부팅된 이후 경과한 시간을 반환
		unsigned long currentTime = millis();
		// FRESHENER_CONFIG에서 설정된 OLED 자동 종료 시간 (초 단위)
		unsigned long SPRAY_DURATION = int(FRESHENER_CONFIG["SPRAY_DURATION"]); // 저장된 값이 밀리초
		// FRESHENER_ON_TIME SPRAY_DURATION 초가 경과했는지 확인
		if ((currentTime - FRESHENER_ON_TIME) >= SPRAY_DURATION) {
			FRESHENER_ON_OFF(false);
		#ifdef FRESHENER_ESP_32
			// SPRAY_WAKE_TIME 이후 초절전 모드로 진입
//			unsigned long SPRAY_WAKE_TIME = int(FRESHENER_CONFIG["SPRAY_WAKE_TIME"]) * 1000;
//			TASK_SIMPLE::NEW_TASK(DEEP_SLEEP_START, 5, SPRAY_WAKE_TIME,  0);
			// 1초 이후 초절전 모드로 진입 -> 한번만 실행
			TASK_SIMPLE::NEW_TASK(DEEP_SLEEP_START, 6, 1000,  0);
		#endif
		}
	}

	int  PIR_SAME_VALUE_COUNT = 0;
	int  PIR_LAST_VALUE = LOW;
	unsigned long PIR_STATE_ON_MILLIS = 0;
	void FRESHENER_PIR_TASK() {
		if (PIR_STATE_ON_MILLIS <= 0) PIR_STATE_ON_MILLIS = millis();
		unsigned long currentTime = millis();
		unsigned long PIR_ON_KEEP_TIME = int(FRESHENER_CONFIG["PIR_ON_KEEP_TIME"]) * 1000; // 초 -> 밀리초
		if ((currentTime - PIR_STATE_ON_MILLIS) < PIR_ON_KEEP_TIME) {
			return; // PIR_STATE_ON 상태를 유지
		}

		int PIR_CURR_VALUE = digitalRead(PIN_PIR);
		if (PIR_LAST_VALUE == PIR_CURR_VALUE) {
			PIR_SAME_VALUE_COUNT += 1;
		} else {
			PIR_LAST_VALUE = PIR_CURR_VALUE;
			PIR_SAME_VALUE_COUNT =  0;
		}

		int PIR_CONFIRM_COUNT = int(FRESHENER_CONFIG["PIR_CONFIRM_COUNT"]);
		if (PIR_SAME_VALUE_COUNT >= PIR_CONFIRM_COUNT) {
			PIR_SAME_VALUE_COUNT = 0;
			bool PIR_CHANGED = false;
			if (PIR_LAST_VALUE == HIGH) {
				if (!PIR_STATE_ON) PIR_CHANGED = true;
				PIR_STATE_ON = true;
				PIR_STATE_ON_MILLIS = millis();
			} else {
				if (PIR_STATE_ON)  PIR_CHANGED = true;
				PIR_STATE_ON = false;
			}
			if (PIR_CHANGED) {
				WS_SEND_FRESHENER_STATUS();
			}
		}

	}

	// OLED 자동 종료 함수
	void OLED_AUTO_OFF() {
		if (!OLED_ON_STATE) return; // OLED가 꺼져 있으면 아무일도 하지 않음
		unsigned long currentTime = millis();
		// FRESHENER_CONFIG에서 설정된 OLED 자동 종료 시간 (초 단위)
		unsigned long OLED_DURATION = int(FRESHENER_CONFIG["OLED_DURATION"]) * 1000; // 초 -> 밀리초로 변환
		// OLED_ON_TIME_MILLIS부터 autoOffTime 초가 경과했는지 확인
		if ((currentTime - OLED_ON_TIME_MILLIS) >= OLED_DURATION) {
			OLED_ON_OFF(false); // OLED 디스플레이 끄기
		}
	}

	bool api_FRESHENER_SKIP = false;
	void api_FRESHENER(JSONVar& args_JSON) {
		/*
		String STR_JSON = JSON.stringify(FRESHENER_CONFIG);
		Serial.println(STR_JSON.c_str());
		//*/

		OLED_ON_OFF(true);

		JSONVar ARGS = args_JSON["ARGS"];
		JSONVar JSON_RES = args_JSON["RES"];
		if (!api_FRESHENER_SKIP) {
			api_FRESHENER_SKIP = true;
		}

		SET_FRESHENER_STATUS(JSON_RES);

		String mode = "";
		if (ARGS.hasOwnProperty("MODE")) {
			mode = String((const char*) ARGS["MODE"]);
		} else {
			return;
		}

		if (mode == "R") { // READ_MODE is now "R"
			if (ARGS.hasOwnProperty("KEY")) {
				String key = String((const char*) ARGS["KEY"]);
				if (FRESHENER_CONFIG.hasOwnProperty(key)) {
					JSON_RES["CONFIG"] = FRESHENER_CONFIG[key];
				} else {
					JSON_RES["STATUS"] = 404;
					JSON_RES["STATUS_HELP"] = "Key not found!";
				}
			} else {
				JSON_RES["CONFIG"] = FRESHENER_CONFIG;  // 전체 설정값 반환
			}
		} else if (mode == "D") { // WRITE_MODE is now "D"
			if (ARGS.hasOwnProperty("KEY")) {
				String keyToRemove = String((const char*) ARGS["KEY"]);

				JSONVar configNew = JSONVar();

				// 키 목록을 순회하며 keyToRemove를 제외하고 새로운 JSONVar에 복사
				for (int i = 0; i < FRESHENER_CONFIG.keys().length(); i++) {
					String key = String((const char *) FRESHENER_CONFIG.keys()[i]);
					if (key != keyToRemove) {
						configNew[key] = FRESHENER_CONFIG[FRESHENER_CONFIG.keys()[i]];
					}
				}

				FRESHENER_CONFIG = JSONVar();
				FRESHENER_CONFIG = configNew;
				saveConfigFile(FRESHENER_CONFIG);

				JSON_RES["STATUS"] = 200;
				JSON_RES["STATUS_HELP"] = keyToRemove + " deleted!";		
			} else {
				JSON_RES["STATUS"] = 400;
				JSON_RES["STATUS_HELP"] = "KEY missing!";
			}
		} else if (mode == "W") { // WRITE_MODE is now "W"
			// JSON 전체를 config.json에 저장하는 경우
			if (ARGS.hasOwnProperty("JSON")) {
				String jsonString = String((const char*) ARGS["JSON"]);
				JSONVar jsonData = JSON.parse(jsonString);
				
				// JSON 파싱이 가능한지 확인
				if (JSON.typeof(jsonData) == "undefined") {
					JSON_RES["STATUS"] = 400;
					JSON_RES["STATUS_HELP"] = "Invalid JSON format!";
				} else {
					FRESHENER_CONFIG = JSONVar();
					FRESHENER_CONFIG = jsonData;
					saveConfigFile(FRESHENER_CONFIG);
					JSON_RES["STATUS"] = 200;
					JSON_RES["STATUS_HELP"] = "Config JSON saved!";
				}
			} 
			// 개별 KEY와 VALUE 저장
			else if (ARGS.hasOwnProperty("KEY") && ARGS.hasOwnProperty("VALUE")) {
				String key = String((const char*) ARGS["KEY"]);
				String value = JSON.stringify(ARGS["VALUE"]);

				// VALUE가 JSON 파싱 가능한지 확인
				JSONVar parsedValue = JSON.parse(value);
				if ((JSON.typeof(parsedValue) == "object") &&
					(parsedValue.keys().length() > 0)) {
					FRESHENER_CONFIG[key] = parsedValue;
				} else if (JSON.typeof(parsedValue) == "array") {
					FRESHENER_CONFIG[key] = parsedValue;
				} else {
					value = String((const char*) ARGS["VALUE"]);
					parsedValue = JSON.parse(value);
					if (JSON.typeof(parsedValue) != "undefined") {
						FRESHENER_CONFIG[key] = parsedValue;
					} else {
						FRESHENER_CONFIG[key] = value;
					}

				}

				saveConfigFile(FRESHENER_CONFIG);

				SET_FRESHENER_STATUS(JSON_RES);

				JSON_RES["STATUS"] = 200;
				JSON_RES["STATUS_HELP"] = "Config saved!";
			} else {
				JSON_RES["STATUS"] = 400;
				JSON_RES["STATUS_HELP"] = "JSON or KEY or VALUE missing!";
			}
		} else {
			JSON_RES["STATUS"] = 400;
			JSON_RES["STATUS_HELP"] = "Invalid MODE!";
		}

	}

	#ifdef FRESHENER_ESP_32
	void SERVO_INIT() {
		// Allow allocation of all timers
		ESP32PWM::allocateTimer(0);
		ESP32PWM::allocateTimer(1);
		ESP32PWM::allocateTimer(2);
		ESP32PWM::allocateTimer(3);
		SPRAY_SERVO.setPeriodHertz(50);    // standard 50 hz servo
		SPRAY_SERVO.attach(PIN_SERVO, 500, 2400); // attaches the servo on pin 18 to the servo object
		// using default min/max of 1000us and 2000us
		// different servos may require different min/max settings
		// for an accurate 0 to 180 sweep
	}
	#endif

	void FRESHENER_1ST_INIT() {
		pinMode(PIN_SPRAY0, OUTPUT);
		pinMode(PIN_PIR, INPUT_PULLUP);

		digitalWrite(PIN_SPRAY0, !SPRAY_ON_VALUE);

	#ifdef FRESHENER_ESP_32
		pinMode(PIN_PW_OLED, OUTPUT);
		pinMode(PIN_PW_SERVO, OUTPUT);
		digitalWrite(PIN_PW_OLED, HIGH);
		digitalWrite(PIN_PW_SERVO, LOW);
		pinMode(PIN_NOSLEEP, INPUT_PULLUP);
		SERVO_INIT();
		
		esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
		switch(wakeup_reason) {
			case ESP_SLEEP_WAKEUP_EXT0: 
				sprintf(STR_MESSAGE, "Wake up caused by external signal using RTC_IO");
				break;
			case ESP_SLEEP_WAKEUP_EXT1: 
				sprintf(STR_MESSAGE, "Wake up caused by external signal using RTC_CNTL");
				break;
			case ESP_SLEEP_WAKEUP_TIMER: 
				sprintf(STR_MESSAGE, "Wake up caused by timer");
				break;
			default: 
				sprintf(STR_MESSAGE, "Wake up was not caused by deep sleep");
				break;
		}
		ESPCOM::println (STR_MESSAGE, SERIAL_PIPE);
	#endif
	}

	void FRESHENER_2ND_INIT() {
		FRESHENER_CONFIG = readConfigFile();
		if (!FRESHENER_CONFIG.hasOwnProperty("OLED_DURATION")) {
			FRESHENER_CONFIG = JSON.parse(STR_DEFALULT_CONFIG);
			saveConfigFile(FRESHENER_CONFIG);
		}
		OLED_ON_TIME_MILLIS = millis();
		OLED_ON_TIME		= trySyncNTPAndGetCurrentEpoch();
		SPRAY_ON_TIME   = trySyncNTPAndGetCurrentEpoch();

		TASK_SIMPLE::SETUP();
		TASK_SIMPLE::NEW_TASK(OLED_AUTO_OFF, 0, 100,  1000);
		TASK_SIMPLE::NEW_TASK(DISP_CLOCK, 1, 100,  1000);
		TASK_SIMPLE::NEW_TASK(FRESHENER_AUTO_OFF, 2, 100,  50);
		TASK_SIMPLE::NEW_TASK(FRESHENER_MAIN_TASK, 3, 100,  1000);
		TASK_SIMPLE::NEW_TASK(FRESHENER_PIR_TASK, 4, 100,  100);
		#ifdef FRESHENER_ESP_32
			TASK_SIMPLE::NEW_TASK(FRESHENER_NOSLEEP_TASK, 5, 100,  100);
		#endif
	}

#endif
