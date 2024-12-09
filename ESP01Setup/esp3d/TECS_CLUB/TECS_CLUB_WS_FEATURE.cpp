#include "Arduino.h"
#include "config.h"

#ifdef TECS_CLUB_WS_FEATURE
	#include <Arduino_JSON.h>
    #include <syncwebserver.h>

    extern void api_CONFIG(JSONVar& args_JSON);

	JSONVar JSON_WS;

	#ifdef TECS_CLUB_MCP23X17_FEATURE
		#include "TECS_CLUB_MCP23X17.h"
	#endif
	#ifdef TECS_CLUB_ADS1X15_FEATURE
		extern void api_ADS1X15(JSONVar& args_JSON);
		#ifdef TECS_CLUB_ADS_TASK_FEATURE
			extern void api_ADS_TASK(JSONVar& args_JSON);
		#endif
	#endif
	#ifdef TECS_CLUB_PCA9685_FEATURE
		extern void api_PCA9685(JSONVar& args_JSON);
	#endif
	#ifdef TECS_CLUB_LED_SHOW_FEATURE
		extern void api_LED_SHOW(JSONVar& args_JSON);
	#endif
	#ifdef TECS_CLUB_FRESHENER
		extern void api_FRESHENER(JSONVar& args_JSON);
	#endif

	void WS_SEND(int args_ID, JSONVar& args_JSON) {
		JSONVar WS_KEYS = JSON_WS.keys();
		for (int i = 0; i < WS_KEYS.length(); i++) {
    		JSONVar ONE_JSON = JSON_WS[WS_KEYS[i]];

    		if (ONE_JSON.hasOwnProperty("MONITOR")) {
   				socket_server->sendTXT((int) ONE_JSON["ID_FROM"], JSON.stringify(args_JSON).c_str());
    		} else {
	    		if (ONE_JSON.hasOwnProperty("ID_FROM")) {
	    			if (((int) ONE_JSON["ID_FROM"]) == args_ID) {
	    				socket_server->sendTXT(args_ID, JSON.stringify(args_JSON).c_str());
	    			}
	    		}
    		}
    	}
	}

	void WS_BROADCAST(JSONVar& args_JSON) {
        // send data to all connected clients
		socket_server->broadcastTXT(JSON.stringify(args_JSON).c_str());
	}

	void WS_PROC(JSONVar& args_JSON) {
		JSONVar JSON_ARGS = args_JSON["ARGS"];
		JSONVar JSON_RES = args_JSON["RES"];

		JSON_RES["STATUS"] = 200;
		JSON_RES["STATUS_HELP"] = String("OK");

		if (JSON_ARGS.hasOwnProperty("REQ")) {
			String STR_REQ = String((const char *) JSON_ARGS["REQ"]);
			if (STR_REQ == String("MONITOR")) {
				JSONVar WS_CLIENT = JSON_WS[JSON_ARGS["ID_KEY"]];
				WS_CLIENT["MONITOR"] = true;
				JSON_RES["WS_CLIENT"] = WS_CLIENT;
			} else if (STR_REQ == String("WS_INFO")) {
				JSONVar WS_CLIENT = JSON_WS[JSON_ARGS["ID_KEY"]];
				JSON_RES["WS_CLIENT"] = WS_CLIENT;
			} else if (STR_REQ == String("api_CONFIG")) {
				api_CONFIG(args_JSON);
                JSON_RES = JSONVar();
                JSON_RES = args_JSON;
#ifdef TECS_CLUB_ADS1X15_FEATURE
			} else if (STR_REQ == String("api_ADS1X15")) {
				api_ADS1X15(args_JSON);
	#ifdef TECS_CLUB_ADS_TASK_FEATURE
			} else if (STR_REQ == String("api_ADS_TASK")) {
				api_ADS_TASK(args_JSON);
	#endif
#endif
#ifdef TECS_CLUB_PCA9685_FEATURE
			} else if (STR_REQ == String("api_PCA9685")) {
				api_PCA9685(args_JSON);
#endif
#ifdef TECS_CLUB_MCP23X17_FEATURE
			} else if (STR_REQ == String("api_MCP")) {
				OBJ_TECS_CLUB_MCP23X17.CMD_MCP(args_JSON);
#endif
#ifdef TECS_CLUB_LED_SHOW_FEATURE
			} else if (STR_REQ == String("api_LED_SHOW")) {
				api_LED_SHOW(args_JSON);
#endif
#ifdef TECS_CLUB_FRESHENER
			} else if (STR_REQ == String("api_FRESHENER")) {
				api_FRESHENER(args_JSON);
#endif
			}
		} else {
			JSON_RES["STATUS"] = 404;
			JSON_RES["STATUS_HELP"] = String("No REQ Attribute.");
		}
	}

#endif