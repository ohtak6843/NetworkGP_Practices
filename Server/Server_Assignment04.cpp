#include "Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512

COORD finalCurPos;
CRITICAL_SECTION cs;

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
	int len; // ���� ���� ������
	char buf[BUFSIZE + 1]; // ���� ���� ������

	// Ŭ���̾�Ʈ ���� ���
	addrlen = sizeof(clientaddr);
	getpeername(client_sock, (struct sockaddr*)&clientaddr, &addrlen);
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));


	// ���� �̸�
	char fileName[256];
	memset(&fileName, 0, sizeof(fileName));

	// ���ϸ� ũ�� �ޱ�
	retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		err_display("recv()");
	}

	// ���ϸ� ũ�⸸ŭ �ޱ�
	retval = recv(client_sock, fileName, len, MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		err_display("recv()");
	}

	printf("���� ���� �̸� : %s\n", fileName);

	// ���� ���� ������ ũ�� �ޱ�
	int totalBytes;
	retval = recv(client_sock, (char*)&totalBytes, sizeof(totalBytes), MSG_WAITALL);
	if (retval == SOCKET_ERROR) {
		err_display("recv()");
	}
	printf("���� ������ ũ��: %d\n", totalBytes);

	// ���� ����
	FILE* fp = fopen(fileName, "wb");
	if (fp == NULL) {
		printf("���� ����� ����\n");
	}

	// ���� ������ �ޱ�
	int numTotal = 0;
	bool first = true;
	COORD printCurPos;
	while (1)
	{
		// ���� ������ ������ ũ�� �ޱ�
		retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}

		// ���� ������ ũ�⸸ŭ ������ �ޱ�
		retval = recv(client_sock, buf, len, MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0) {
			break;
		}
		else {
			fwrite(buf, 1, retval, fp);
			if (ferror(fp)) {
				printf("���� ����� ����\n");
				break;
			}
			numTotal += retval;
		}

		EnterCriticalSection(&cs);
		// ó�� ���۷� ��� ��
		if (first) {
			first = !first;
			printCurPos = getxy();
			printf("%s ���� ���۷� %lf%%\n", fileName, double(numTotal) / totalBytes * 100);
		}
		else {
			finalCurPos = getxy();
			gotoxy(printCurPos);
			printf("%s ���� ���۷� %lf%%", fileName, double(numTotal) / totalBytes * 100);
			gotoxy(finalCurPos);
		}
		LeaveCriticalSection(&cs);
	}

	// ���� ��� ���
	if (numTotal == totalBytes)
		printf("���� ���� �Ϸ�\n");
	else
		printf("���� ���� ����\n");
	fclose(fp);

	// ���� �ݱ�
	closesocket(client_sock);
	printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
		addr, ntohs(clientaddr.sin_port));

	return 0;
}

int main(int argc, char* argv[])
{
	int retval;

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// ���� ����
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

	// ������ ��ſ� ����� ����
	SOCKET client_sock;
	struct sockaddr_in clientaddr;
	int addrlen;

	HANDLE hThread;

	// �Ӱ� ���� �ʱ�ȭ
	InitializeCriticalSection(&cs);

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// ������ Ŭ���̾�Ʈ ���� ���
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			addr, ntohs(clientaddr.sin_port));

		// ������ ����
		hThread = CreateThread(NULL, 0, ProcessClient,
			(LPVOID)client_sock, 0, NULL);
		if (hThread == NULL) { closesocket(client_sock); }
		else { CloseHandle(hThread); }
	}

	// �Ӱ� ���� ����
	DeleteCriticalSection(&cs);

	// ���� �ݱ�
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}