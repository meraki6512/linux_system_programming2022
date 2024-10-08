# linux_system_programming2022
리눅스 시스템 프로그래밍 학습 및 설계 프로젝트

**1. 지정한 파일 또는 디렉토리의 이름을 탐색해서 파일 및 디렉토리의 속성을 비교하는 프로그램**  

*목표: 유닉스 리눅스 / , 컴퓨팅 환경에서 파일 속성 디렉토리 속성을 비교하여 출력하는 프로그램을 작성하고 시스템 프로그램 설계및 응용 능력을 향상* 

- 파일 또는 디렉토리의 이름과 디렉토리 경로 입력 시 입력한 디렉토리부터 탐색을 시작해 이름과 크기가 동일한 모든 파일 또는 디렉토리를 리스트로 출력함
- 리스트에서 특정 파일 또는 디렉토리를 선택하면 해당 파일 또는 디렉토리와 입력한 파일 또는 디렉토리의 차이를 출력함
- 옵션 (q s i r)은 순서에 상관없이 동시에 사용 가능함: 단, OPTION이 존재하지 않을 경우나 공백 없이 여러 이 동시에 사용된 경우 에러 처리 후 다시 입력 대기
$ q : 두 파일 내용이 다른 경우 차이점 출력 없이 알림
$ s : 두 파일 내용이 동일한 경우 알림
$ i : 대소문자 구분 없이 비교
$ r : 디렉토리 비교 시 하위 디렉토리를 전부 재귀적으로 탐색하여 모든 파일 비교


**2. ssu_sdup은 시스템 내 존재하는 동일 중복 한 파일을 찾고 삭제하는 프로그램**

*목표: 유닉스 리눅스 시스템에서 제공하는 시스템 자료구조와 시스템콜 및 라이브러리 함수를 이용하여 프로그램을 작성함으로써 시스템 프로그래밍 설계 및 응용 능력 향상* 
- 지정한 디렉토리 내에서 사용자가 입력한 조건 (파일 확장자, 파일 크기 범위)에 맞춰 동일한(중복된) 정규 파일을 찾고 삭제하는 프로그램으로 동일한 정규 파일은 파일의 이름이 달라도 파일의 크기와 내용이 동일한 경우를 말함 
- 텍스트 파일 뿐만 아니라 모든 형태의 바이너리 파일을 대상으로 함
- 내장된 명령어를 통해 네 번째 인자의 시작 디렉토리 아래 모든 서브디렉토리까지 재귀적으로 탐색하여, 첫 번째 인자로 주어진 파일과 동일한 중복된 파일을 찾는데, 두 번째 인자와 세 번째 인자 크기 사이의 파일만을 대상으로 찾음
- 동일한 중복 파일이 존재하는 경우, 사용자가 입력한 옵션에 따라 동일한 중복 파일 삭제
- 내장명령어 fmd5, fsha1, help를 위한 프로세스를 생성(fork() 하고 이를 실행(exec)함
- 내장된 명령어는 링크드 리스트로 동일한 파일 리스트를 관리함 


**3. 사용자 쓰레드 라이브러리를 이용하여 리눅스 시스템 내 존재하는 동일한 중복 파일을 찾고 삭제 및 복원하는 프로그램**    

*목표: 유닉스 리눅스 시스템에서 제공하는 시스템 자료구조와 시스템콜 및 라이브러리 함수를 이용하여 프로그램을 작성함으로써 시스템 프로그래밍 설계 및 응용 능력 향상*
- 2번에 쓰레드 라이브러리와 파일 삭제 및 복원 시 사용할 로그를 추가하여 구현
