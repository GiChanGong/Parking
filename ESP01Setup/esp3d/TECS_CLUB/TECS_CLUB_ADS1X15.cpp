#include "Arduino.h"
#include "config.h"

#ifdef TECS_CLUB_ADS1X15_FEATURE
	#include <Arduino_JSON.h>
	#include <Adafruit_ADS1X15.h>

	Adafruit_ADS1115 ads1115;  /* Use this for the 16-bit version */
	bool	init_ads1115 = false;
	Adafruit_ADS1015 ads1015;  /* Use this for the 12-bit version */
	bool	init_ads1015 = false;

	// The ADC input range (or gain) can be changed via the following
	// functions, but be careful never to exceed VDD +0.3V max, or to
	// exceed the upper and lower limits if you adjust the input range!
	// Setting these values incorrectly may destroy your ADC!
	//                                                                ADS1015  ADS1115
	//                                                                -------  -------
	// ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
	// ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
	// ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
	// ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
	// ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
	// ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV

	void api_ADS1X15(JSONVar& args_JSON) {
		JSONVar ARGS = args_JSON["ARGS"];
		JSONVar JSON_RES = args_JSON["RES"];
		int BITS = 8;
		if (ARGS.hasOwnProperty("BITS")) {
			BITS = JSON.typeof(ARGS["BITS"]) == String("string") ? atoi((const char *) ARGS["BITS"]) : (int) ARGS["BITS"];
		}
		int CH = -1;
		if (ARGS.hasOwnProperty("CH")) {
			CH = JSON.typeof(ARGS["CH"]) == String("string") ? atoi((const char *) ARGS["CH"]) : (int) ARGS["CH"];
		}
		if (CH > 3) CH = 3;

		JSONVar JSON_VALUES = JSON.parse("[]");

		if (BITS < 16) {
			if (!init_ads1015) {
				if (ads1015.begin()) {
					init_ads1015 = true;
				}
			}
			if (init_ads1015) {
				JSON_RES["ADS"] = "ADS1015";
				if (CH < 0) {
					for (int i=0; i<4; i++) {
						JSONVar JSON_ADC = JSON.parse("{}");
						int16_t adc = ads1015.readADC_SingleEnded(i);
//						float volt  = ads1015.computeVolts(adc);
						JSON_ADC["ADC"] = adc;
//						JSON_ADC["VOLT"] = volt;
						JSON_VALUES[i] = JSON_ADC;
					}
				} else {
					JSONVar JSON_ADC = JSON.parse("{}");
					int16_t adc = ads1015.readADC_SingleEnded(CH);
//					float volt  = ads1015.computeVolts(adc);
					JSON_ADC["ADC"] = adc;
//					JSON_ADC["VOLT"] = volt;
					JSON_VALUES[0] = JSON_ADC;
				}
			}
		} else {
			if (!init_ads1115) {
				if (ads1115.begin()) {
					init_ads1115 = true;
				}
			}
			if (init_ads1115) {
				JSON_RES["ADS"] = "ADS1115";
				if (CH < 0) {
					for (int i=0; i<4; i++) {
						JSONVar JSON_ADC = JSON.parse("{}");;
						int16_t adc = ads1115.readADC_SingleEnded(i);
//						float volt  = ads1115.computeVolts(adc);
						JSON_ADC["ADC"] = adc;
//						JSON_ADC["VOLT"] = volt;
						JSON_VALUES[i] = JSON_ADC;
					}
				} else {
					JSONVar JSON_ADC = JSON.parse("{}");;
					int16_t adc = ads1115.readADC_SingleEnded(CH);
//					float volt  = ads1115.computeVolts(adc);
					JSON_ADC["ADC"] = adc;
//					JSON_ADC["VOLT"] = volt;
					JSON_VALUES[0] = JSON_ADC;
				}
			}
		}

		JSON_RES["VALUES"] = JSON_VALUES;
	}

	#ifdef TECS_CLUB_ADS_TASK_FEATURE
		#include "TASK_SIMPLE.h"

		#ifdef TECS_CLUB_WS_FEATURE
			extern void WS_BROADCAST(JSONVar& args_JSON);
		#endif

		#define ADS_MAX_HISTORY 60
		int ADS_SIZE_HISTORY = 15;
		JSONVar ADS_HISTORY;
		JSONVar JSON_ADS_TASK;

		#ifdef TECS_CLUB_ADS_EVENT_FEATURE

			#define MARU_THRESHOLD 10

			// 시계열 데이터 배열과 데이터 크기
			int timeSeriesData[ADS_MAX_HISTORY];

			// 마루 개수 계산 함수
			void countMaru(int *data, JSONVar& args_JSON) {
			    int maxValue = 0;
			    int minValue = 0;
			    int maruCount = 0;
			    int trend = 0;  // 0: 상승, 1: 하락

			    for (int i = 1; i < ADS_SIZE_HISTORY; i++) {

			    	if (i == 1) {
			    		maxValue = data[i - 1];
			    		minValue = data[i - 1];
			    	} else {
			    		if (maxValue < data[i - 1]) maxValue = data[i - 1];
			    		if (minValue > data[i - 1]) minValue = data[i - 1];
			    	}

			        if ((data[i] - data[i - 1]) > MARU_THRESHOLD) {
			            if (trend == 1) {
			                maruCount++;
			            }
			            trend = 0;
			        } else if ((data[i - 1] - data[i]) > MARU_THRESHOLD) {
			            trend = 1;
			        }
			    }

			    args_JSON["CNT_MARU"]  = maruCount;
			    args_JSON["MAX_VALUE"] = maxValue;
			    args_JSON["MIN_VALUE"] = minValue;
			    args_JSON["MID_VALUE"] = (maxValue + minValue) / 2;
			}

			#ifdef TECS_CLUB_SERVO_EVENT_FEATURE
				extern void api_PCA9685(JSONVar& args_JSON);

				int  SERVO_VALUES[][16] = {
					{  10,  10, 770, 770, 770, 770, 770, 770, 770, 770, 770, 770, 770, 770, 770, 770},
					{ 770, 770, 770, 770, 770, 770, 770, 770, 770, 770, 770, 770, 770, 770, 770, 770},
					{  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10,  10}
				};

				void EXEC_SERVO(int INDEX_SERVO) {
					JSONVar JSON_REQ;
					JSONVar JSON_ARGS;
					for (int i=0; i<16; i++) {
						JSON_ARGS["INDEX"] = i;
						JSON_ARGS["VALUE"] = SERVO_VALUES[INDEX_SERVO][i];
						JSON_REQ["ARGS"] = JSON_ARGS;
						api_PCA9685(JSON_REQ);
					}
				}

				void EXEC_SERVO_EVENT(JSONVar& args_JSON) {
					int CNT_MARU = (int) args_JSON["CNT_MARU"];
					int MAX_VALUE = (int) args_JSON["MAX_VALUE"];
					int MIN_VALUE = (int) args_JSON["MIN_VALUE"];
//					int MID_VALUE = (int) args_JSON["MID_VALUE"];
					if ((MAX_VALUE > 700) && (MIN_VALUE < 500)) {
					}
					if (CNT_MARU > 2) {
						EXEC_SERVO(0);
					} else if (CNT_MARU > 1) {
						EXEC_SERVO(1);
					} else {
						EXEC_SERVO(2);
					}
				}
			#endif

			void ADS_EVENT() {
				JSONVar ARGS = JSON_ADS_TASK["ARGS"];
				int ADS_HISTORY_SIZE = ADS_HISTORY.length();
				if (ADS_HISTORY_SIZE < ADS_SIZE_HISTORY) return;

				for (int i=0; i<ADS_HISTORY_SIZE; i++) {
					JSONVar JSON_VALUES = ADS_HISTORY[i];
					JSONVar JSON_VALUE = JSON_VALUES[0];
					timeSeriesData[i] = 1024 - (int) JSON_VALUE["ADC"];
				}

				JSONVar JSON_EVENT;
				JSON_EVENT["ADS"] = "ADS_EVENT";
				JSON_EVENT["SIZE"] = ADS_HISTORY_SIZE;
				countMaru(timeSeriesData, JSON_EVENT);

				#ifdef TECS_CLUB_SERVO_EVENT_FEATURE
					EXEC_SERVO_EVENT(JSON_EVENT);
				#endif

				#ifdef TECS_CLUB_WS_FEATURE
					WS_BROADCAST(JSON_EVENT);
				#endif

				ADS_HISTORY = JSON.parse("[]");

			}

		#endif

		void ADS_TASK() {
			api_ADS1X15(JSON_ADS_TASK);

			JSONVar ARGS = JSON_ADS_TASK["ARGS"];

			JSONVar JSON_RES = JSON_ADS_TASK["RES"];
			JSONVar JSON_VALUES = JSON_RES["VALUES"];
			JSONVar JSON_ADS = JSON.parse(JSON.stringify(JSON_VALUES));

			int LAST_INDEX = ADS_HISTORY.length();
			if (LAST_INDEX < ADS_SIZE_HISTORY) {
				ADS_HISTORY[LAST_INDEX] = JSON_ADS;
			} else {
				for (int i=0; i<ADS_SIZE_HISTORY-1; i++) {
					JSONVar JSON_NEXT = ADS_HISTORY[i+1];
					ADS_HISTORY[i] = JSON_NEXT;
				}
				ADS_HISTORY[ADS_SIZE_HISTORY-1] = JSON_ADS;
			}

			#ifdef TECS_CLUB_WS_FEATURE
				if (ARGS.hasOwnProperty("WS")) {
					WS_BROADCAST(JSON_RES);
	//				WS_BROADCAST(ADS_HISTORY);
				}
			#endif

			#ifdef TECS_CLUB_ADS_EVENT_FEATURE
				ADS_EVENT();
			#endif

		}

		int  ADS_TASK_NO = 13;
		bool api_ADS_TASK_SKIP = false;
		void api_ADS_TASK(JSONVar& args_JSON) {

			JSON_ADS_TASK = JSON.parse(JSON.stringify(args_JSON));

			JSONVar ARGS = args_JSON["ARGS"];
			JSONVar JSON_RES = args_JSON["RES"];

			if (!api_ADS_TASK_SKIP) {
				ADS_HISTORY = JSON.parse("[]");
				TASK_SIMPLE::SETUP();
				TASK_SIMPLE::NEW_TASK(ADS_TASK, ADS_TASK_NO, 100, 200);
				api_ADS_TASK_SKIP = true;
			}
			
			if (ARGS.hasOwnProperty("STOP")) {
				int STOP_VAL = JSON.typeof(ARGS["STOP"]) == String("string") ? atoi((const char *) ARGS["STOP"]) : (int) ARGS["STOP"];
				if (STOP_VAL > 0) {
					TASK_SIMPLE::END_TASK(ADS_TASK_NO);
					api_ADS_TASK_SKIP = false;
				}
			}

			if (ARGS.hasOwnProperty("SP")) {
				int SP_VAL = JSON.typeof(ARGS["SP"]) == String("string") ? atoi((const char *) ARGS["SP"]) : (int) ARGS["SP"];
				if (SP_VAL <   10) SP_VAL = 10;
				if (SP_VAL > 2000) SP_VAL = 2000;
				TASK_INFO *P = TASK_SIMPLE::GET_TASK_INFO(ADS_TASK_NO);
				P->TIME_LOOP = SP_VAL;
			}
			if (ARGS.hasOwnProperty("SZ")) {
				int SZ_VAL = JSON.typeof(ARGS["SZ"]) == String("string") ? atoi((const char *) ARGS["SZ"]) : (int) ARGS["SZ"];
				if (SZ_VAL <  1) SZ_VAL = 1;
				if (SZ_VAL > 60) SZ_VAL = 60;
				ADS_SIZE_HISTORY = SZ_VAL;
			}

		}

	#endif

#endif
