#include <iostream>
#include "Common.h"

using namespace std;

WORD calcByte(int low, int high)
{
	BYTE lowByte = static_cast<BYTE>(low);
	BYTE highByte = static_cast<BYTE>(high);
	
	return (((WORD)lowByte & 0x00ff) | (((WORD)highByte << 8) & 0xff00));
}

void printBinaryByte(WORD version)
{
	cout << static_cast<int>(LOBYTE(version)) << "." << static_cast<int>(HIBYTE(version)) << endl;
}

int main(int argc, char* argv[])
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(calcByte(2, 2), &wsa) != 0)
		return 1;
	printf("[알림] 윈속 초기화\n");

	// [Chapter2 - 연습문제 1]
	cout << "wVersion: "; printBinaryByte(wsa.wVersion);
	cout << "wHighVersion: "; printBinaryByte(wsa.wHighVersion);
	cout << "szDescription: " << wsa.szDescription << ", " << endl;
	cout << "szSystemStatus: " << wsa.szSystemStatus << endl;


	// 윈속 종료
	WSACleanup();
	printf("[알림] 윈속 종료\n");
	return 0;
}