#include "Arduino.h"
#include "config.h"

#ifdef TECS_CLUB_NTP_FEATURE
	#include <NTPClient.h>   // NTP 라이브러리
	#include <WiFiUdp.h>     // UDP 프로토콜을 위한 라이브러리

	// NTP 설정
	WiFiUDP ntpUDP;
	NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // UTC 시간, 1분마다 업데이트

	unsigned long startMillis;  // millis()의 시작 값
	unsigned long startEpoch;   // NTP로 받은 시작 시점의 Unix 타임스탬프 (초 단위)
	bool ntpSynced = false;     // NTP 초기화 여부
	bool firstCall = true;      // 최초 호출 여부 플래그

	// NTP 실패 시 기본 Unix 타임스탬프 (2024년 1월 1일 00:00:00)
	unsigned long defaultEpoch = 1704067200; // 2024-01-01 00:00:00 UTC

	// 타임존 오프셋 (한국 표준시 KST는 UTC+9이므로 32400초)
	const int timezoneOffset = 9 * 3600;

	// NTP를 1회 시도하여 currentEpoch 값을 계산하는 함수
	unsigned long trySyncNTPAndGetCurrentEpoch() {
		unsigned long currentMillis = millis();

		if (firstCall) {
			// 최초 호출 시 NTP 초기화 시도
			timeClient.begin();

			if (timeClient.forceUpdate()) {
				// NTP 연동 성공 시
				startMillis = currentMillis;  // NTP 동기화 시점의 millis() 값
				startEpoch = timeClient.getEpochTime();  // NTP 동기화 시점의 Unix 타임스탬프
				ntpSynced = true;
				// Serial.println("NTP time sync successful.");
			} else {
				// NTP 연동 실패 시 기본 시간 사용
				startMillis = currentMillis;  // 기본 시간 기준 millis() 값 저장
				startEpoch = defaultEpoch;    // 기본 시간의 Unix 타임스탬프 설정
				ntpSynced = false;
				// Serial.println("Failed to get time from NTP server. Using default time.");
			}
			firstCall = false;  // 최초 호출 완료
		}

		// 경과된 시간을 초 단위로 더하여 currentEpoch 계산
		unsigned long currentEpoch = startEpoch + (currentMillis - startMillis) / 1000;

		// 타임존 오프셋을 더하여 최종 Unix 타임스탬프 계산
		currentEpoch += timezoneOffset;

		return currentEpoch;
	} 

	// Unix 타임스탬프를 년월일시분초 형식의 문자열로 변환하는 함수
	String epochToDateTimeString(unsigned long epoch) {
		// Unix 타임스탬프를 년/월/일/시/분/초로 변환
		struct tm *ptm = gmtime((time_t *)&epoch);

		int year = ptm->tm_year + 1900;
		int month = ptm->tm_mon + 1;
		int day = ptm->tm_mday;
		int hour = ptm->tm_hour;
		int minute = ptm->tm_min;
		int second = ptm->tm_sec;

		// 시간을 문자열로 포맷팅하여 반환
		char buffer[25];
		sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
		return String(buffer);
	}

	// Unix 타임스탬프를 년월일시분초 형식의 문자열로 변환하는 함수
	String epochToTimeString(unsigned long epoch) {
		// Unix 타임스탬프를 년/월/일/시/분/초로 변환
		struct tm *ptm = gmtime((time_t *)&epoch);

		int hour = ptm->tm_hour;
		int minute = ptm->tm_min;
		int second = ptm->tm_sec;

		// 시간을 문자열로 포맷팅하여 반환
		char buffer[25];
		sprintf(buffer, "%02d:%02d:%02d", hour, minute, second);
		return String(buffer);
	}

	// 년, 월, 일, 시, 분, 초를 입력받아 Unix Epoch 값을 반환하는 함수
	unsigned long dateTimeToEpoch(int year, int month, int day, int hour, int minute, int second) {
		struct tm t;

		// struct tm 초기화 및 값 설정
		t.tm_year = year - 1900;  // tm_year는 1900년을 기준으로 한 연도이므로, 입력값에서 1900을 빼야 함
		t.tm_mon = month - 1;     // tm_mon은 0부터 시작하므로, 입력된 월에서 1을 빼야 함
		t.tm_mday = day;          // 일
		t.tm_hour = hour;         // 시
		t.tm_min = minute;        // 분
		t.tm_sec = second;        // 초
		t.tm_isdst = -1;          // 자동으로 서머타임 여부 결정

		// mktime() 함수는 로컬 시간을 기준으로 Unix 타임스탬프(Epoch)를 반환
		time_t epochTime = mktime(&t);

		// mktime이 -1을 반환하는 경우는 유효하지 않은 시간일 때임 (처리할 수 있음)
		if (epochTime == -1) {
			Serial.println("Invalid date/time input");
			return 0;
		}

		return (unsigned long)epochTime;  // Unix Epoch 타임스탬프 반환
	}
	
	// getCurrentTimeAsString 함수: NTP를 시도하여 currentEpoch 값을 얻고 이를 문자열로 변환하여 반환
	String getCurrentTimeAsString() {
		unsigned long currentEpoch = trySyncNTPAndGetCurrentEpoch(); // NTP 시도를 포함한 currentEpoch 값을 얻음
		return epochToTimeString(currentEpoch);     // 문자열로 변환하여 반환
	}

	String getCurrentDateTimeAsString() {
		unsigned long currentEpoch = trySyncNTPAndGetCurrentEpoch(); // NTP 시도를 포함한 currentEpoch 값을 얻음
		return epochToDateTimeString(currentEpoch);     // 문자열로 변환하여 반환
	}

#endif
