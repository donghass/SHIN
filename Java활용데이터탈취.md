🛡️ 실습 개요
목표: 실행 중인 자바 서버의 메모리에 침투하여 결제 시 발생하는 민감 정보(카드번호 등)를 가로채고, 원하는 데이터만 필터링하여 로그로 남기기.

기술 스택: Java Agent (Javassist), Spring Boot (Target), Python Flask (Attacker), Linux (EC2).

1단계: 공격자 수신 서버 설정 (Python Flask)
타겟 서버에서 보낸 데이터를 받아 필터링하고 파일로 저장하는 '그물'을 만드는 작업입니다.

가상환경 생성 및 접속

Bash
python3 -m venv venv
source venv/bin/activate
pip install flask
수신 프로그램 작성 (collect.py)

nano collect.py 명령어로 파일을 열고 필터링 로직이 포함된 Flask 코드를 작성합니다.

핵심 기능: couponCode, totalAmount 등 불필요한 필드 제거 및 collect.log 저장.

수신 서버 실행

Bash
python3 collect.py
2단계: Java Agent 스파이 도구 제작 (내 PC)
타겟 서버의 메서드에 침투할 JAR 파일을 만드는 과정입니다.

에이전트 코드 작성 (SpyAgent.java)

대상: com.bookvillage.backend.controller.OrderController의 checkout 메서드.

내용: $2(CartRequest) 객체의 내용을 가로채 Sender.sendToAttacker()로 전달.

전송 코드 작성 (Sender.java)

가로챈 데이터를 공격자 IP의 9090 포트로 POST 전송.

인코딩 깨짐 방지를 위해 StandardCharsets.UTF_8 설정.

빌드 및 생성

Bash
./gradlew clean jar
결과물: build/libs/collect-1.0.jar

3단계: 타겟 서버(EC2) 침투 및 실행
실제 서비스 중인 서버에 에이전트를 주입합니다.

에이전트 파일 전송 (내 PC -> EC2)

Bash
scp -i "your-key.pem" build/libs/collect-1.0.jar ubuntu@[EC2_IP]:/home/ubuntu/
기존 서버 프로세스 종료 (EC2)

8080 포트 점유 에러 해결을 위해 기존 자바를 모두 죽입니다.

Bash
pkill -9 -f java
에이전트를 포함하여 서버 재시작

Bash
java -javaagent:/home/ubuntu/collect-1.0.jar -jar target/your-app.jar &
## export $(grep -v '^#' .env | xargs) && java -javaagent:/home/ubuntu/collect-1.0.jar -jar target/*.jar &
성공 시 로그: [!] Agent Loaded, [+] Successfully hooked OrderController.checkout()

4단계: 데이터 탈취 확인 및 모니터링
실제 유저가 결제했을 때 데이터가 들어오는지 확인합니다.

실시간 로그 모니터링 (공격자 터미널)

수신 서버가 가동 중인 창 외에 새 창을 열어 확인합니다.

Bash
tail -f collect.log
결과 분석

cardNumber, cardExpiry, cardCvc 등 민감 정보가 포함되었는지 확인.

요청한 대로 couponCode 등이 제외되었는지 확인.