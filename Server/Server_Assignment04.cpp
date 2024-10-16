#include "Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512

COORD finalCurPos;
CRITICAL_SECTION cs;

void gotoxy(int x, int y)
{
	COORD pos = { (short)x, (short)y };
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void gotoxy(COORD pos)
{
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

COORD getxy()
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	return csbi.dwCursorPosition;
}

DWORD WINAPI ProcessClient(LPVOID arg)
{
	int retval;
	SOCKET client_sock = (SOCKET)arg;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	int len; // 고정 길이 데이터
	char buf[BUFSIZE + 1]; // 가변 길이 데이터

	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));


	// 파일 이름
	char fileName[256];
	memset(&fileName, 0, sizeof(fileName));

	// 파일명 크기 받기
	retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		EnterCriticalSection(&cs);
		err_display("recv()");
		LeaveCriticalSection(&cs);
	}

	// 파일명 크기만큼 받기
	retval = recv(client_sock, fileName, len, MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		EnterCriticalSection(&cs);
		err_display("recv()");
		LeaveCriticalSection(&cs);
	}

	EnterCriticalSection(&cs);
	printf("받을 파일 이름 : %s\n", fileName);
	LeaveCriticalSection(&cs);

	// 전송 받을 데이터 크기 받기
	int totalBytes;
	retval = recv(client_sock, (char*)&totalBytes, sizeof(totalBytes), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		EnterCriticalSection(&cs);
		err_display("recv()");
		LeaveCriticalSection(&cs);
	}

	EnterCriticalSection(&cs);
	printf("받을 데이터 크기: %d\n", totalBytes);
	LeaveCriticalSection(&cs);

	// 파일 열기
	FILE* fp = fopen(fileName, "wb");
	if (fp == NULL) {
		EnterCriticalSection(&cs);
		printf("파일 입출력 오류\n");
		LeaveCriticalSection(&cs);
	}

	// 파일 데이터 받기
	int numTotal = 0;
	bool first = true;
	COORD printCurPos;
	while (1)
	{
		// 받을 파일의 데이터 크기 받기
		retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			EnterCriticalSection(&cs);
			err_display("recv()");
			LeaveCriticalSection(&cs);
			break;
		}

		// 받을 데이터 크기만큼 데이터 받기
		retval = recv(client_sock, buf, len, MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			EnterCriticalSection(&cs);
			err_display("recv()");
			LeaveCriticalSection(&cs);
			break;
		}
		else if (retval == 0) {
			break;
		}
		else {
			fwrite(buf, 1, retval, fp);
			if (ferror(fp)) {
				EnterCriticalSection(&cs);
				printf("파일 입출력 오류\n");
				LeaveCriticalSection(&cs);
				break;
			}
			numTotal += retval;
		}

		EnterCriticalSection(&cs);
		if (first) {
			first = !first;
			printCurPos = getxy();
			printf("%s 파일 전송률 %lf%%\n", fileName, double(numTotal) / totalBytes * 100);
		}
		else {
			finalCurPos = getxy();
			gotoxy(printCurPos);
			printf("%s 파일 전송률 %lf%%\n", fileName, double(numTotal) / totalBytes * 100);
			gotoxy(finalCurPos);
		}
		LeaveCriticalSection(&cs);
	}

	// 전송 결과 출력
	EnterCriticalSection(&cs);
	if (numTotal == totalBytes)
		printf("%s 파일 전송 완료\n", fileName);
	else
		printf("%s 파일 전송 실패\n", fileName);
	fclose(fp);
	LeaveCriticalSection(&cs);

	// 소켓 닫기
	closesocket(client_sock);
	EnterCriticalSection(&cs);
	printf("\n[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
		addr, ntohs(clientaddr.sin_port));
	LeaveCriticalSection(&cs);

	return 0;
}

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 소켓 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	HANDLE hThread;

	// 임계 영역 초기화
	InitializeCriticalSection(&cs);

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			EnterCriticalSection(&cs);
			err_display("accept()");
			LeaveCriticalSection(&cs);
			break;
		}

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		EnterCriticalSection(&cs);
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));
		LeaveCriticalSection(&cs);

		// 스레드 생성
		hThread = CreateThread(NULL, 0, ProcessClient,
			(LPVOID)client_sock, 0, NULL);
		if (hThread == NULL) { closesocket(client_sock); }
		else { CloseHandle(hThread); }
	}

	// 임계 영역 삭제
	DeleteCriticalSection(&cs);

	// 소켓 닫기
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}