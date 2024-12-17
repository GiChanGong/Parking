from flask import Flask, jsonify
import requests
import time
import gspread
from google.oauth2.service_account import Credentials


app = Flask(__name__)
ESP01_URLS = {
    "ESP01_9": "http://172.20.10.9/?REQ=api_KNU&WHAT=DISTANCE",
    "ESP01_7": "http://172.20.10.7/?REQ=api_KNU&WHAT=DISTANCE",
    "ESP01_11": "http://172.20.10.11/?REQ=api_KNU&WHAT=DISTANCE",
}

# Google Sheets API 인증
SERVICE_ACCOUNT_FILE = "centering-talon-426107-r3-72b95e1d0a78.json"  # JSON 키 파일 이름
SCOPES = ["https://www.googleapis.com/auth/spreadsheets", "https://www.googleapis.com/auth/drive"]

credentials = Credentials.from_service_account_file(SERVICE_ACCOUNT_FILE, scopes=SCOPES)

# Google 스프레드시트 클라이언트 생성
gc = gspread.authorize(credentials)
SPREADSHEET_NAME = "ESP01"  # 스프레드시트 이름

try:
    worksheet = gc.open(SPREADSHEET_NAME).get_worksheet(1)
    print("스프레드시트 연결 성공!")
    print(f"첫 번째 시트 이름: {worksheet.title}")
except gspread.exceptions.SpreadsheetNotFound:
    print("스프레드시트를 찾을 수 없습니다. 권한과 이름을 확인하세요.")


@app.route('/', methods=['GET'])
def fetch_and_update():
    total_parking_spots = len(ESP01_URLS)  # 총 주차 공간 수
    occupied_spots = 0  # 차가 있는 공간 수

    try:
        for whoami, url in ESP01_URLS.items():
            try:
                # 각 ESP01에서 데이터 가져오기
                response = requests.get(url, timeout=10)  # 요청 제한 시간 5초
                if response.status_code == 200:
                    esp_data = response.json()
                    distance = esp_data.get("RES", {}).get("DISTANCE")
                    if distance is not None:
                        # 주차 상태 판별
                        if distance < 15:  # 15cm 미만이면 차가 있다고 간주
                            occupied_spots += 1
                            space = 1  # 점유 상태
                        else:
                            space = 0  # 빈 상태

                        # 데이터 기록
                        timestamp = time.strftime("%Y-%m-%d %H:%M:%S")  # 현재 시간
                        remaining_spots = total_parking_spots - occupied_spots  # 남은 자리 계산

                        worksheet.append_row([
                            timestamp,  # 현재 시간
                            whoami,     # ESP01 이름
                            distance,   # 거리(cm)
                            space,      # 점유 상태
                            occupied_spots,  # 현재 차량 대수
                            remaining_spots  # 남은 자리 수
                        ])
            except requests.exceptions.RequestException as e:
                print(f"Failed to fetch data from {whoami}: {e}")
                continue
        
        # 결과 반환
        remaining_spots = total_parking_spots - occupied_spots
        return jsonify({
            "total_spots": total_parking_spots,
            "occupied_spots": occupied_spots,
            "remaining_spots": remaining_spots,
            "message": "Data processed and added to Google Sheets"
        }), 200

    except Exception as e:
        return jsonify({"error": str(e)}), 500


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)