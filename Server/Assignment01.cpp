#include "Common.h"

void CheckByteOrder()
{
	u_short test = 0x1234;
	BYTE* byte_ptr = (BYTE*)&test;

	printf("�׽�Ʈ �Է°�: 0x1234\n\n");
	printf("�޸𸮿� ����� �׽�Ʈ �Է°��� ������ ��Ʈ�� �о��� ���: %#x\n", *byte_ptr);
	printf("�޸𸮿� ����� �׽�Ʈ �Է°��� �ֻ��� ��Ʈ�� �о��� ���: %#x\n", *(byte_ptr + sizeof(BYTE)));
	printf("\n");

	if (*byte_ptr == 0x12) printf("ȣ��Ʈ�� ����Ʈ ���� ����� �� ������Դϴ�.\n");
	else if (*byte_ptr == 0x34) printf("ȣ��Ʈ�� ����Ʈ ���� ����� ��Ʋ ������Դϴ�.\n");
	else printf("ȣ��Ʈ�� ����Ʈ ���� ����� �� �� �����ϴ�.\n");
}

int main(int argc, char* argv[])
{
	CheckByteOrder();

	return 0;
}