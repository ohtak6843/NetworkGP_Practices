#include "..\..\Common.h"

char* SERVERIP = (char*)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    50

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		printf("파일명이 입력되지 않았습니다.\n");
		return 0;
	}

	int retval;
	const char* fileName = argv[1];
	int fileLen = strlen(fileName);

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	// 파일 열기
	FILE* fp = fopen(fileName, "rb");
	if (fp == NULL) {
		printf("파일 입출력 오류\n");
		return 0;
	}

	// 파일명 크기 전송
	retval = send(sock, (char*)&fileLen, sizeof(int), 0);
	if (retval == SOCKET_ERROR) err_quit("send()");

	// 파일명 크기만큼 전송
	retval = send(sock, fileName, fileLen, 0);
	if (retval == SOCKET_ERROR) err_quit("send()");

	// 전송할 파일 크기 구하기
	fseek(fp, 0, SEEK_END);
	int totalBytes = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// 전송할 파일 크기 보내기
	retval = send(sock, (char*)&totalBytes, sizeof(totalBytes), 0);
	if (retval == SOCKET_ERROR) err_quit("send()");

	// 데이터 통신에 사용할 변수
	char buf[BUFSIZE];
	int numRead;
	int numTotal = 0;

	// 서버와 데이터 통신
	while (1)
	{
		numRead = fread(buf, 1, BUFSIZE, fp);
		if (numRead > 0) {
			// 전송할 파일 데이터 크기 보내기
			retval = send(sock, (char*)&numRead, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}

			// 전송할 파일 데이터 크기만큼 데이터 보내기
			retval = send(sock, buf, numRead, 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}

			numTotal += numRead;
		}

		else if (numRead == 0 && numTotal == totalBytes) {
			printf("\n파일 전송 완료: %d 바이트\n", numTotal);
			break;
		}
		
		else {
			printf("파일 입출력 오류\n");
			break;
		}
	}

	fclose(fp);

	// 소켓 닫기
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}