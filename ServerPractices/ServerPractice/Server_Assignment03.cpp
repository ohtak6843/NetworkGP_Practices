#include "Common.h"

#define SERVERPORT 9000
#define BUFSIZE    512

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
	int len; // ���� ���� ������
	char buf[BUFSIZE + 1]; // ���� ���� ������

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

		// ���� �̸�
		char fileName[256];
		memset(&fileName, 0, sizeof(fileName));

		// ���ϸ� ũ�� �ޱ�
		retval = recv(client_sock, (char*)&len, sizeof(int), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0) {
			break;
		}

		// ���ϸ� ũ�⸸ŭ �ޱ�
		retval = recv(client_sock, fileName, len, MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0) {
			break;
		}

		printf("���� ���� �̸� : %s\n", fileName);

		// ���� ���� ������ ũ�� �ޱ�
		int totalBytes;
		retval = recv(client_sock, (char*)&totalBytes, sizeof(totalBytes), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			closesocket(client_sock);
			continue;
		}
		printf("���� ������ ũ��: %d\n", totalBytes);

		// ���� ����
		FILE* fp = fopen(fileName, "wb");
		if (fp == NULL) {
			printf("���� ����� ����\n");
			closesocket(client_sock);
			continue;
		}

		// ���� ������ �ޱ�
		int numTotal = 0;
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

			printf("\r%lf%%", double(numTotal) / totalBytes * 100);
		}
		fclose(fp);

		// ���� ��� ���
		if (numTotal == totalBytes)
			printf(" ���� ���� �Ϸ�\n");
		else
			printf(" ���� ���� ����\n");
		

		// ���� �ݱ�
		closesocket(client_sock);
		printf("[TCP ����] Ŭ���̾�Ʈ ����: IP �ּ�=%s, ��Ʈ ��ȣ=%d\n",
			addr, ntohs(clientaddr.sin_port));
	}

	// ���� �ݱ�
	closesocket(listen_sock);

	// ���� ����
	WSACleanup();
	return 0;
}