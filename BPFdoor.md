# 🛡️ BPF 기반 Backdoor & Reverse Shell 실습 가이드

본 실습은 **Raw Socket + BPF Filter 기반 백도어가 특정 패킷을 수신했을 때 Reverse Shell을 실행하는 구조**를 이해하기 위한 보안 학습 실습이다.

⚠️ **주의**  
이 실습은 반드시 **격리된 실습 환경(로컬 VM / 테스트 서버)**에서만 수행해야 한다.

---

# 1. 실습 목적

본 실습을 통해 다음 개념을 이해한다.

- Raw Socket 패킷 캡처
- BPF (Berkeley Packet Filter)
- Magic Packet 기반 백도어 트리거
- Reverse Shell 동작 원리
- DNS 트래픽 위장 기법

---

# 2. 실습 환경

| 구성 | 설명 |
|---|---|
공격자 PC | Reverse shell을 수신하는 시스템 |
타겟 서버 | 백도어 프로그램이 실행되는 시스템 |
네트워크 | 동일 네트워크 또는 접근 가능한 환경 |

예시 구성

```
Attacker PC
     │
     │ UDP trigger
     ▼
Target Server (Backdoor 실행)
     │
     │ Reverse shell
     ▼
Attacker Listener
```

---

# 3. 코드 역할

| 코드 | 실행 위치 | 역할 |
|---|---|---|
Backdoor 코드 | 타겟 서버 | 패킷 감시 + Reverse Shell 실행 |
Trigger 코드 | 공격자 PC | Magic Packet 전송 |

---

# 4. 실습 준비

## 4.1 타겟 서버에서 백도어 코드 컴파일

```bash
gcc backdoor.c -o backdoor
```

Raw Socket을 사용하기 때문에 **root 권한으로 실행해야 한다.**

```bash
sudo ./backdoor
```

실행 후 프로그램은 **네트워크 패킷을 계속 감시**한다.

---

# 5. 공격자 PC 설정

## 5.1 Reverse Shell Listener 실행

```bash
nc -lvnp 4444
```

옵션 설명

| 옵션 | 설명 |
|---|---|
-l | listen 모드 |
-v | verbose 출력 |
-n | DNS 조회 비활성 |
-p | 포트 지정 |

---

# 6. 트리거 프로그램 실행

## 6.1 공격자 PC에서 컴파일

```bash
gcc trigger.c -o trigger
```

## 6.2 실행

```bash
./trigger
```

이 프로그램은 다음 패킷을 전송한다.

```
X[공격자 IP]:4444
```

예시

```
X192.168.0.10:4444
```

---

# 7. 공격 동작 과정

1️⃣ 타겟 서버에서 백도어 실행

```
sudo ./backdoor
```

2️⃣ 공격자 PC에서 리스너 실행

```
nc -lvnp 4444
```

3️⃣ 공격자 PC에서 트리거 실행

```
./trigger
```

4️⃣ 백도어가 패킷 감지

```
magic byte = X
```

5️⃣ 공격자 IP 추출

```
[공격자 IP]
```

6️⃣ Reverse Shell 실행

```
connect([공격자 IP]:4444)
```

7️⃣ 공격자 PC에서 쉘 획득

```
whoami
id
```

---

# 8. 패킷 구조

트리거 패킷 payload

```
X[attacker_ip]:[port]
```

예

```
X192.168.1.10:4444
```

| 위치 | 의미 |
|---|---|
X | Magic Byte |
IP | 공격자 서버 |
Port | Reverse Shell 포트 |

---

# 9. 왜 DNS 포트를 사용하는가

본 실습에서는 **UDP 53 포트(DNS)** 를 사용한다.

이유

| 이유 | 설명 |
|---|---|
방화벽 우회 | DNS는 대부분 허용됨 |
탐지 회피 | 정상 트래픽처럼 보임 |
패킷 위장 | IDS 탐지 회피 |

---

# 10. 주요 기술 요소

| 기술 | 설명 |
|---|---|
Raw Socket | 네트워크 패킷 직접 캡처 |
BPF Filter | 특정 패킷만 필터링 |
Magic Packet | 백도어 트리거 |
Fork | 백그라운드 Reverse Shell 실행 |
Reverse Shell | 공격자에게 쉘 연결 |

---

# 11. 공격 흐름 요약

```
Attacker PC
    │
    │ UDP Packet
    ▼
Target Server
    │
    │ Magic Packet 감지
    ▼
Reverse Shell 실행
    │
    ▼
Attacker Listener
```

---

# 12. 탐지 방법 (보안 관점)

이와 같은 백도어는 다음 방법으로 탐지할 수 있다.

### Raw Socket 프로세스 탐지

```
lsof
ss
netstat
```

### BPF 필터 확인

```
bpftool prog
```

### 네트워크 패킷 분석

IDS 룰 예시

```
udp dst port 53
payload startswith "X"
```

### Reverse Shell 탐지

- EDR
- Network Monitoring
- Firewall Logs

---

# 13. 학습 포인트

이 실습을 통해 다음 공격 기법을 이해할 수 있다.

- Packet Sniffing Backdoor
- Covert Channel
- Magic Packet Trigger
- Reverse Shell Execution

---

# 14. 참고

이 구조는 실제 악성코드인 **BPFdoor**와 유사한 구조를 가진다.

특징

- Raw Socket 사용
- BPF 기반 필터
- Magic Packet 트리거
- Reverse Shell 실행

---

# 15. 실습 종료

실습 종료 시 백도어 프로세스를 종료한다.

```
killall backdoor
```

또는

```
ps aux | grep backdoor
kill -9 [PID]
```

---

# ⚠️ 보안 주의사항

본 코드는 학습 목적이며 **실제 시스템에 사용하면 불법 행위가 될 수 있다.**

반드시 다음 환경에서만 사용한다.

- 로컬 VM
- 보안 실습 환경
- 개인 연구 환경