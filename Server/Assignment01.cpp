#include "Common.h"

void CheckByteOrder()
{
	u_short test = 0x1234;
	BYTE* byte_ptr = (BYTE*)&test;

	printf("테스트 입력값: 0x1234\n\n");
	printf("메모리에 저장된 테스트 입력값의 최하위 비트를 읽었을 경우: %#x\n", *byte_ptr);
	printf("메모리에 저장된 테스트 입력값의 최상위 비트를 읽었을 경우: %#x\n", *(byte_ptr + sizeof(BYTE)));
	printf("\n");

	if (*byte_ptr == 0x12) printf("호스트의 바이트 정렬 방식은 빅 엔디안입니다.\n");
	else if (*byte_ptr == 0x34) printf("호스트의 바이트 정렬 방식은 리틀 엔디안입니다.\n");
	else printf("호스트의 바이트 정렬 방식을 알 수 없습니다.\n");
}

int main(int argc, char* argv[])
{
	CheckByteOrder();

	return 0;
}