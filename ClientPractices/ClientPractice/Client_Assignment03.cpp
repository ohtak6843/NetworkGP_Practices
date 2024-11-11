#include "..\..\Common.h"

char* SERVERIP = (char*)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    50

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		printf("���ϸ��� �Էµ��� �ʾҽ��ϴ�.\n");
		return 0;
	}

	int retval;
	const char* fileName = argv[1];
	int fileLen = strlen(fileName);

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// ���� ����
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

	// ���� ����
	FILE* fp = fopen(fileName, "rb");
	if (fp == NULL) {
		printf("���� ����� ����\n");
		return 0;
	}

	// ���ϸ� ũ�� ����
	retval = send(sock, (char*)&fileLen, sizeof(int), 0);
	if (retval == SOCKET_ERROR) err_quit("send()");

	// ���ϸ� ũ�⸸ŭ ����
	retval = send(sock, fileName, fileLen, 0);
	if (retval == SOCKET_ERROR) err_quit("send()");

	// ������ ���� ũ�� ���ϱ�
	fseek(fp, 0, SEEK_END);
	int totalBytes = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	// ������ ���� ũ�� ������
	retval = send(sock, (char*)&totalBytes, sizeof(totalBytes), 0);
	if (retval == SOCKET_ERROR) err_quit("send()");

	// ������ ��ſ� ����� ����
	char buf[BUFSIZE];
	int numRead;
	int numTotal = 0;

	// ������ ������ ���
	while (1)
	{
		numRead = fread(buf, 1, BUFSIZE, fp);
		if (numRead > 0) {
			// ������ ���� ������ ũ�� ������
			retval = send(sock, (char*)&numRead, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}

			// ������ ���� ������ ũ�⸸ŭ ������ ������
			retval = send(sock, buf, numRead, 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}

			numTotal += numRead;
		}

		else if (numRead == 0 && numTotal == totalBytes) {
			printf("\n���� ���� �Ϸ�: %d ����Ʈ\n", numTotal);
			break;
		}
		
		else {
			printf("���� ����� ����\n");
			break;
		}
	}

	fclose(fp);

	// ���� �ݱ�
	closesocket(sock);

	// ���� ����
	WSACleanup();
	return 0;
}