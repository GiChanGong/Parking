#include "Arduino.h"
#include "config.h"

#ifdef TECS_CLUB_WEB_API_FEATURE
    #include "webinterface.h"
	#include <Arduino_JSON.h>
	#include <Wire.h>
	#include "espcom.h"

	extern void handle_root_org();

	String FILE_TO_STRING(String path) {
		String STR_RESULT = "";
		if (SPIFFS.exists(path)) {
			FS_FILE file = SPIFFS.open(path, SPIFFS_FILE_READ);
			while (file.available()){
				STR_RESULT += file.readStringUntil('\n');
			}
			file.close();
		}
		return STR_RESULT;
	}

	// config.json 파일을 읽는 함수
	JSONVar readConfigFile() {
		JSONVar configData;

		FS_FILE configFile = SPIFFS.open("/config.json", SPIFFS_FILE_READ);
		if (!configFile) {
			return configData;  // 파일이 없을 경우 빈 데이터 반환
		}

		size_t size = configFile.size();
		if (size > 4096) {
			configFile.close();
			return configData;  // 파일 크기가 너무 클 경우 빈 데이터 반환
		}

		std::unique_ptr<char[]> buf(new char[size]);
		configFile.readBytes(buf.get(), size);
		configFile.close();

		configData = JSON.parse(buf.get());
		if (JSON.typeof(configData) == "undefined") {
			return JSONVar();  // 파싱 실패시 빈 JSON 반환
		}

		return configData;
	}

	// config.json 파일에 설정값 저장하는 함수
	void saveConfigFile(JSONVar& configData) {
		FS_FILE configFile = SPIFFS.open("/config.json", SPIFFS_FILE_WRITE);
		if (!configFile) {
			return;  // 파일 열기 실패
		}

		configFile.print(JSON.stringify(configData));
		configFile.close();
	}

	void api_CONFIG(JSONVar& args_JSON) {
		JSONVar ARGS = args_JSON["ARGS"];
		JSONVar REQ;
		JSONVar RES;
		String mode = "";

		// URI와 Method 설정
		String uri = web_interface->web_server.urlDecode(web_interface->web_server.uri());
		REQ["URI"] = uri;
		REQ["Method"] = (web_interface->web_server.method() == HTTP_GET) ? "GET" : "POST";
		REQ["REQ"] = String((const char*) ARGS["REQ"]); // 요청된 api_FRESHENER 같은 항목을 포함

		if (ARGS.hasOwnProperty("MODE")) {
			mode = String((const char*) ARGS["MODE"]);
		} else {
			RES["STATUS"] = 400;
			RES["STATUS_HELP"] = "MODE not specified!";
			JSONVar response;
			response["URI"] = uri;
			response["Method"] = REQ["Method"];
			response["REQ"] = REQ;
			response["RES"] = RES;
			web_interface->web_server.send(400, "application/json", JSON.stringify(response));
			return;
		}

		if (mode == "R") { // READ_MODE is now "R"
			if (SPIFFS.exists("/config.json")) {
				JSONVar configData = readConfigFile();
				if (ARGS.hasOwnProperty("KEY")) {
					String key = String((const char*) ARGS["KEY"]);
					if (configData.hasOwnProperty(key)) {
						RES["STATUS"] = 200;
						RES["STATUS_HELP"] = "OK";
						RES["CONFIG"] = configData[key];
					} else {
						RES["STATUS"] = 404;
						RES["STATUS_HELP"] = "Key not found!";
					}
				} else {
					RES["STATUS"] = 200;
					RES["STATUS_HELP"] = "OK";
					RES["CONFIG"] = configData;  // 전체 설정값 반환
				}
			} else {
				RES["STATUS"] = 404;
				RES["STATUS_HELP"] = "Config file not found!";
			}
		} else if (mode == "D") { // WRITE_MODE is now "D"
			if (ARGS.hasOwnProperty("KEY")) {
				String keyToRemove = String((const char*) ARGS["KEY"]);

				JSONVar configData;
				if (SPIFFS.exists("/config.json")) {
					configData = readConfigFile();
				}

				JSONVar configNew = JSONVar();

				// 키 목록을 순회하며 keyToRemove를 제외하고 새로운 JSONVar에 복사
				for (int i = 0; i < configData.keys().length(); i++) {
					String key = String((const char *) configData.keys()[i]);
					if (key != keyToRemove) {
						configNew[key] = configData[configData.keys()[i]];
					}
				}

				saveConfigFile(configNew);

				RES["STATUS"] = 200;
				RES["STATUS_HELP"] = keyToRemove + " deleted!";		
			} else {
				RES["STATUS"] = 400;
				RES["STATUS_HELP"] = "KEY missing!";
			}
		} else if (mode == "W") { // WRITE_MODE is now "W"
			// JSON 전체를 config.json에 저장하는 경우
			if (ARGS.hasOwnProperty("JSON")) {
				String jsonString = String((const char*) ARGS["JSON"]);
				JSONVar jsonData = JSON.parse(jsonString);
				
				// JSON 파싱이 가능한지 확인
				if (JSON.typeof(jsonData) == "undefined") {
					RES["STATUS"] = 400;
					RES["STATUS_HELP"] = "Invalid JSON format!";
				} else {
					saveConfigFile(jsonData);
					RES["STATUS"] = 200;
					RES["STATUS_HELP"] = "Config JSON saved!";
				}
			} 
			// 개별 KEY와 VALUE 저장
			else if (ARGS.hasOwnProperty("KEY") && ARGS.hasOwnProperty("VALUE")) {
				String key = String((const char*) ARGS["KEY"]);
				String value = String((const char*) ARGS["VALUE"]);

				JSONVar configData;
				if (SPIFFS.exists("/config.json")) {
					configData = readConfigFile();
				}

				// VALUE가 JSON 파싱 가능한지 확인
				JSONVar parsedValue = JSON.parse(value);
				if (JSON.typeof(parsedValue) != "undefined") {
					// VALUE가 JSON일 경우 JSON으로 저장
					configData[key] = parsedValue;
				} else {
					// JSON 파싱이 불가능하면 문자열로 저장
					configData[key] = value;
				}

				saveConfigFile(configData);

				RES["STATUS"] = 200;
				RES["STATUS_HELP"] = "Config saved!";
			} else {
				RES["STATUS"] = 400;
				RES["STATUS_HELP"] = "JSON or KEY or VALUE missing!";
			}
		} else {
			RES["STATUS"] = 400;
			RES["STATUS_HELP"] = "Invalid MODE!";
		}

		// 최종 응답 데이터 포맷
		JSONVar response;
		response["URI"] = uri;
		response["Method"] = REQ["Method"];
		response["REQ"] = REQ;
		response["RES"] = RES;

		args_JSON = JSONVar();
		args_JSON = response;
		
		web_interface->web_server.send(200, "application/json", JSON.stringify(response));

	}

	void api_GET_FILE(JSONVar& args_JSON) {
		JSONVar ARGS = args_JSON["ARGS"];
		String FLDR = String("");
		String FILE = String("");
		if (ARGS.hasOwnProperty("FLDR")) FLDR = String((const char *) ARGS["FLDR"]);
		if (ARGS.hasOwnProperty("FILE")) FILE = String((const char *) ARGS["FILE"]);
		if (ARGS.hasOwnProperty("FLDR_TEMPLATE")) FLDR = String((const char *) ARGS["FLDR_TEMPLATE"]);
		if (ARGS.hasOwnProperty("FILE_TEMPLATE")) FILE = String((const char *) ARGS["FILE_TEMPLATE"]);

		String PATH_TEMPLATE = FLDR + "/" + FILE;
		String contentType = web_interface->getContentType(PATH_TEMPLATE);

		String PATH_TEMPLATE_WithGz = PATH_TEMPLATE + ".gz";

		if (SPIFFS.exists(PATH_TEMPLATE) || SPIFFS.exists(PATH_TEMPLATE_WithGz)) {
	        if(SPIFFS.exists(PATH_TEMPLATE_WithGz)) {
	            PATH_TEMPLATE = PATH_TEMPLATE_WithGz;
	        }
			FS_FILE file = SPIFFS.open(PATH_TEMPLATE, SPIFFS_FILE_READ);
			if (web_interface->web_server.streamFile(file, contentType) != file.size()) {
//				logger->println("api_GET_FILE : Sent less data than expected!");
			}
			file.close();
		} else {
			ARGS["MESSAGE"] = String("api_GET_FILE : ")+ PATH_TEMPLATE + String(" IS NOT FOUND!");
			web_interface->web_server.send(404, "application/json", JSON.stringify(ARGS));
		}
	}

	void api_FSCLEAN(JSONVar& args_JSON) {
		JSONVar JSON_RES = args_JSON["RES"];

		bool formatted = SPIFFS.format();
		if (formatted) {
			JSON_RES["MESSAGE"] = "Success formatting";
		} else {
			JSON_RES["MESSAGE"] = "Error formatting";
		}
	}

	void api_EECLEAN(JSONVar& args_JSON) {
		JSONVar JSON_RES = args_JSON["RES"];

		CONFIG::reset_config();
		JSON_RES["MESSAGE"] = "CONFIG::reset_config()";

		args_JSON["RES"] = JSON_RES;
	}

	void api_I2C_SCAN(JSONVar& args_JSON) {
		JSONVar JSON_RES = args_JSON["RES"];
		char	STR_BUFF[128];
		byte error, address;
		int nDevices = 0;
		JSONVar I2C_DEVICES;	
		for (address=1; address < 127; address++) {
			Wire.beginTransmission(address);
			error = Wire.endTransmission();
	    	if (error == 0) {
	    		sprintf(STR_BUFF, "I2C device found at address = %02X\n", address); 
	    		sprintf(STR_BUFF, "I2C device = %02X", address); 
				I2C_DEVICES[nDevices] = String(STR_BUFF);
	    		nDevices++;
	    	} else {
	    		sprintf(STR_BUFF, "I2C device error = %d, at address = %02X\n", error, address); 
	    	}
		}
		if (nDevices < 1) {
			sprintf(STR_BUFF, "No I2C devices found.\n"); 
		} else {
			sprintf(STR_BUFF, "Done!\n"); 
		}
		JSON_RES["I2C_DEVICES"] = I2C_DEVICES;
		JSON_RES["MESSAGE"] = String(STR_BUFF);
	}

#ifdef TECS_CLUB_PCA9685_FEATURE
	#include "TECS_CLUB_PCA9685.h"

	void api_PCA9685(JSONVar& args_JSON) {
		JSONVar ARGS = args_JSON["ARGS"];
		JSONVar JSON_RES = args_JSON["RES"];
		char STR_BUFF[128];
		if (ARGS.hasOwnProperty("PIN")) {
			int PIN		= JSON.typeof(ARGS["PIN"])   == String("string") ? atoi((const char *) ARGS["PIN"])   : (int) ARGS["PIN"];
			int VALUE	= JSON.typeof(ARGS["VALUE"]) == String("string") ? atoi((const char *) ARGS["VALUE"]) : (int) ARGS["VALUE"];
			OBJ_TECS_CLUB_PCA9685.analogWrite(PIN, VALUE);
			sprintf(STR_BUFF,"TECS_CLUB_PCA9685 analogWrite(%d, %d)\n", PIN, VALUE);
			JSON_RES["PIN"] = PIN;
			JSON_RES["VALUE"] = VALUE;
		} else {
			if (ARGS.hasOwnProperty("DEBUG"))	OBJ_TECS_CLUB_PCA9685.DEBUG_MODE= atoi((const char *) ARGS["DEBUG"]) > 0 ? true : false;
			if (ARGS.hasOwnProperty("ENABLE"))	OBJ_TECS_CLUB_PCA9685.PCA_ENABLE= atoi((const char *) ARGS["ENABLE"]) > 0 ? true : false;
			if (ARGS.hasOwnProperty("BANK"))	OBJ_TECS_CLUB_PCA9685.PCA_BANK	= JSON.typeof(ARGS["BANK"]) == String("string") ? atoi((const char *) ARGS["BANK"]) : (int) ARGS["BANK"];
			if (ARGS.hasOwnProperty("INDEX"))	OBJ_TECS_CLUB_PCA9685.PCA_INDEX	= JSON.typeof(ARGS["INDEX"]) == String("string") ? atoi((const char *) ARGS["INDEX"]) : (int) ARGS["INDEX"];
			if (ARGS.hasOwnProperty("VALUE"))	OBJ_TECS_CLUB_PCA9685.PCA_VALUE	= JSON.typeof(ARGS["VALUE"]) == String("string") ? atoi((const char *) ARGS["VALUE"]) : (int) ARGS["VALUE"];
			OBJ_TECS_CLUB_PCA9685.UPDATE_PCA_VALUES();
			sprintf(STR_BUFF,
				"PCA_ENABLE = %d, PCA_BANK = %d, PCA_INDEX = %d, PCA_VALUE = %d\n",
				OBJ_TECS_CLUB_PCA9685.PCA_ENABLE,
				OBJ_TECS_CLUB_PCA9685.PCA_BANK,
				OBJ_TECS_CLUB_PCA9685.PCA_INDEX,
				OBJ_TECS_CLUB_PCA9685.PCA_VALUE
			);
			JSON_RES["PCA_ENABLE"] = OBJ_TECS_CLUB_PCA9685.PCA_ENABLE;
			JSON_RES["PCA_BANK"] = OBJ_TECS_CLUB_PCA9685.PCA_BANK;
			JSON_RES["PCA_INDEX"] = OBJ_TECS_CLUB_PCA9685.PCA_INDEX;
			JSON_RES["PCA_VALUE"] = OBJ_TECS_CLUB_PCA9685.PCA_VALUE;
		}
		JSON_RES["PCA_STATUS"] = STR_BUFF;
	}

#endif
#ifdef TECS_CLUB_MCP23X17_FEATURE
	#include "TECS_CLUB_MCP23X17.h"
#endif

#ifdef TECS_CLUB_ADS1X15_FEATURE
	extern void api_ADS1X15(JSONVar& args_JSON);
	#ifdef TECS_CLUB_ADS_TASK_FEATURE
		extern void api_ADS_TASK(JSONVar& args_JSON);
	#endif
#endif

#ifdef TECS_CLUB_TASK_FEATURE
	#include "TASK_SIMPLE.h"
	#define PIN_TX 1
	#define PIN_RX 3

	#ifdef TECS_CLUB_FRESHENER
		extern void api_FRESHENER(JSONVar& args_JSON);
	#else
		bool TX_ON = false;
		void TX_ON_OFF() {
			if (TX_ON) {
				digitalWrite(PIN_TX, HIGH);
			} else {
				digitalWrite(PIN_TX, LOW);
			}
			TX_ON = !TX_ON;
		}

		int RX_VAL = LOW;
		void RX_READ() {
			RX_VAL = analogRead(PIN_RX);
		}

		int  TX_TASK_NO = 0;
		int  RX_TASK_NO = 1;
		bool api_TX_RX_SKIP = false;
		void api_TX_RX(JSONVar& args_JSON) {
			JSONVar ARGS = args_JSON["ARGS"];
			JSONVar JSON_RES = args_JSON["RES"];

			if (!api_TX_RX_SKIP) {
				TASK_SIMPLE::SETUP();

				pinMode(PIN_TX, OUTPUT);
				pinMode(PIN_RX, INPUT );

				TASK_SIMPLE::NEW_TASK(TX_ON_OFF,  TX_TASK_NO, 100, 500);
				TASK_SIMPLE::NEW_TASK(RX_READ,  RX_TASK_NO, 100, 500);
				api_TX_RX_SKIP = true;
			}
			int S = 0;
			if (ARGS.hasOwnProperty("S")) {
				S = atoi((const char *) ARGS["S"]);
				TASK_INFO* T = TASK_SIMPLE::GET_TASK_INFO(TX_TASK_NO);
				T->TIME_LOOP = S;
				T = TASK_SIMPLE::GET_TASK_INFO(RX_TASK_NO);
				T->TIME_LOOP = S;
			}

			JSON_RES["VAL"] = RX_VAL;
			JSON_RES["S"] = S;
		}
	#endif

	#ifdef TECS_CLUB_LED_SHOW_FEATURE
		extern void api_LED_SHOW(JSONVar& args_JSON);
	#endif

	#ifdef TECS_CLUB_KNU_FEATURE
		extern void api_KNU(JSONVar& args_JSON);
	#endif

	
#endif

	void PROC_WEB_API(JSONVar args_JSON) {
		JSONVar ARGS = args_JSON["ARGS"];
		JSONVar JSON_RES = args_JSON["RES"];
		
		JSON_RES["STATUS"] = 200;
		JSON_RES["STATUS_HELP"] = String("OK");

		JSONVar ARGS_KEYS = ARGS.keys();
		if (ARGS_KEYS.length() < 1) {
			ARGS["REQ"] = "api_GET_FILE";
			ARGS["FLDR"] = "/WA";
			ARGS["FILE"] = "index.html";
			api_GET_FILE(args_JSON);
		} else if (ARGS.hasOwnProperty("REQ")) {
			String STR_REQ = String((const char *) ARGS["REQ"]);
			if (STR_REQ == String("DUMMY")) {
			} else if (STR_REQ == String("api_CONFIG")) {
				api_CONFIG(args_JSON);
			} else if (STR_REQ == String("api_FSCLEAN")) {
				api_FSCLEAN(args_JSON);
			} else if (STR_REQ == String("api_EECLEAN")) {
				api_EECLEAN(args_JSON);
			} else if (STR_REQ == String("api_GET_PAGE") || STR_REQ == String("api_GET_FILE")) {
				api_GET_FILE(args_JSON);
#ifdef TECS_CLUB_PCA9685_FEATURE
			} else if (STR_REQ == String("api_PCA9685")) {
				api_PCA9685(args_JSON);
			} else if (STR_REQ == String("api_I2C_SCAN")) {
				api_I2C_SCAN(args_JSON);
#endif
#ifdef TECS_CLUB_MCP23X17_FEATURE
			} else if (STR_REQ == String("api_MCP")) {
				OBJ_TECS_CLUB_MCP23X17.CMD_MCP(args_JSON);
#endif
#ifdef TECS_CLUB_ADS1X15_FEATURE
			} else if (STR_REQ == String("api_ADS1X15")) {
				api_ADS1X15(args_JSON);
	#ifdef TECS_CLUB_ADS_TASK_FEATURE
			} else if (STR_REQ == String("api_ADS_TASK")) {
				api_ADS_TASK(args_JSON);
	#endif
	#ifdef TECS_CLUB_KNU_FEATURE
			} else if (STR_REQ == String("api_KNU")) {
				api_KNU(args_JSON);
	#endif

#endif
#ifdef TECS_CLUB_TASK_FEATURE
	#ifdef TECS_CLUB_FRESHENER
			} else if (STR_REQ == String("api_FRESHENER")) {
				api_FRESHENER(args_JSON);
	#else
			} else if (STR_REQ == String("api_TX_RX")) {
				api_TX_RX(args_JSON);
	#endif
	#ifdef TECS_CLUB_LED_SHOW_FEATURE
				} else if (STR_REQ == String("api_LED_SHOW")) {
					api_LED_SHOW(args_JSON);
	#endif
#endif
			}
			web_interface->web_server.send(200, "application/json", JSON.stringify(args_JSON));
		} else if (ARGS.hasOwnProperty("ESP3D")) {
			handle_root_org();
		} else {
			web_interface->web_server.send(200, "application/json", JSON.stringify(args_JSON));
		}
	}

	void handle_web_interface_root() {

	    String uri = web_interface->web_server.urlDecode(web_interface->web_server.uri());
		JSONVar JSON_Object;
		JSON_Object["URI"] = uri;
		JSON_Object["Method"] = (web_interface->web_server.method() == HTTP_GET) ? "GET" : "POST";

		JSONVar WS_Args;
		for (uint8_t i = 0; i < web_interface->web_server.args(); i++) {
			WS_Args[web_interface->web_server.argName(i)] = web_interface->web_server.arg(i);
		}
		JSON_Object["ARGS"] = WS_Args;
		PROC_WEB_API(JSON_Object);
	}

#endif
