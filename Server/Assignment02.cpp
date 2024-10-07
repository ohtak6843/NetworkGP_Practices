#include "Common.h"

int main(int argc, char* argv[])
{
	if (argc <= 1) {
		printf("������ �̸��� �Էµ��� �ʾҽ��ϴ�.\n");
		return 0;
	}

	const char* name = argv[1];

	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	struct hostent* host_ptr = gethostbyname(name);
	if (host_ptr == NULL) {
		err_display("gethostbyname()");
		return 0;
	}

	printf("[���� ���]\n");
	for (int i = 0; host_ptr->h_aliases[i] != NULL; i++) {
		printf("%s\n", host_ptr->h_aliases[i]);
	}
	printf("\n");

	printf("[IPv4 �ּ� ���]\n");
	for (int i = 0; host_ptr->h_addr_list[i] != NULL; i++) {
		char ipv4[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, host_ptr->h_addr_list[i], ipv4, sizeof(ipv4));
		printf("%s\n", ipv4);
	}
	printf("\n");

	// ���� ����
	WSACleanup();
	return 0;
}