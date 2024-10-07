#include "Common.h"

bool isLittleEndian(char* ipv4, struct in_addr ipv4_addr)
{
	struct in_addr ipv4_test;
	inet_pton(AF_INET, ipv4, &ipv4_test);
	if (ipv4_test.s_addr != ipv4_addr.s_addr)
		return true;
	
	return false;
}

bool isBigEndian(const char* ipv4, struct in_addr ipv4_addr)
{
	struct in_addr ipv4_test;
	inet_pton(AF_INET, ipv4, &ipv4_test);
	if (ipv4_test.s_addr == ipv4_addr.s_addr)
		return true;

	return false;
}

int main(int argc, char* argv[])
{
	// ���� �ʱ�ȭ
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	/*----------------*/
	/* IPv4 ��ȯ ���� */
	/*----------------*/
	// ������ IPv4 �ּ� ���
	const char* ipv4test = "147.46.114.70";
	printf("IPv4 �ּ�(��ȯ ��) = %s\n", ipv4test);

	// inet_pton() �Լ� ����
	struct in_addr ipv4num;
	inet_pton(AF_INET, ipv4test, &ipv4num);

	//// [Chapter 3 - �������� 2]
	//ipv4num.s_addr = inet_addr(ipv4test);

	printf("IPv4 �ּ�(��ȯ ��) = %#x\n", ipv4num.s_addr);

	// inet_ntop() �Լ� ����
	char ipv4str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &ipv4num, ipv4str, sizeof(ipv4str));

	//// [Chapter 3 - �������� 3]
	//printf("%s\n", inet_ntoa(ipv4num));

	printf("IPv4 �ּ�(�ٽ� ��ȯ ��) = %s\n", ipv4str);
	printf("\n");

	//// [Chapter 3 - �������� 1]
	//if (isBigEndian(ipv4test, ipv4num))
	//	printf("�� ����� ����Դϴ�.\n");
	//else printf("�� ����� ����� �ƴմϴ�.\n");
	//printf("\n");

	/*----------------*/
	/* IPv6 ��ȯ ���� */
	/*----------------*/
	// ������ IPv6 �ּ� ���
	const char* ipv6test = "2001:0230:abcd:ffab:0023:eb00:ffff:1111";
	printf("IPv6 �ּ�(��ȯ ��) = %s\n", ipv6test);

	// inet_pton() �Լ� ����
	struct in6_addr ipv6num;
	inet_pton(AF_INET6, ipv6test, &ipv6num);
	printf("IPv6 �ּ�(��ȯ ��) = 0x");
	for (int i = 0; i < 16; i++)
		printf("%02x", ipv6num.s6_addr[i]);
	printf("\n");

	// inet_ntop() �Լ� ����
	char ipv6str[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &ipv6num, ipv6str, sizeof(ipv6str));
	printf("IPv6 �ּ�(�ٽ� ��ȯ ��) = %s\n", ipv6str);

	// ���� ����
	WSACleanup();
	return 0;
}