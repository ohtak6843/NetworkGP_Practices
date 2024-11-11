#include "..\..\Common.h"
#include <commdlg.h>
#include <commctrl.h>
#include "resource.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512

// 대화상자 프로시저
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// 에디트 컨트롤 출력 함수
void DisplayText(const char* fmt, ...);
// 소켓 함수 오류 출력
void DisplayError(const char* msg);
// 파일 전송 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);

// 전역 변수
SOCKET sock;
char buf[BUFSIZE + 1];
HWND hSendButton;
HWND hEdit1, hEdit2, hProgressBar;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // 대화상자 생성
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    // 윈속 종료
    WSACleanup();
    return 0;
}

// 대화상자 프로시저
INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static wchar_t fileName[MAX_PATH];

    switch (uMsg) {
    case WM_INITDIALOG:
        hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
        hEdit2 = GetDlgItem(hDlg, IDC_EDIT2);
        hSendButton = GetDlgItem(hDlg, IDOK);
        hProgressBar = GetDlgItem(hDlg, IDC_PROGRESS1);

        SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);
        SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BUTTON_SELECT:
            // 파일 선택 버튼 클릭 시 파일 선택 대화상자 호출
        {
            OPENFILENAME ofn = { 0 };
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = hDlg;
            ofn.lpstrFile = fileName;
            ofn.lpstrFile[0] = '\0';
            ofn.nMaxFile = sizeof(fileName);
            ofn.lpstrFilter = L"All Files\0*.*\0";
            ofn.nFilterIndex = 1;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            if (GetOpenFileName(&ofn)) {
                SetDlgItemText(hDlg, IDC_EDIT1, fileName); // 선택한 파일명을 에디트 컨트롤에 표시
            }
        }
        return TRUE;

        case IDOK:
            // 전송 버튼 클릭 시 파일 전송 스레드 실행
            EnableWindow(hSendButton, FALSE);
            //CreateThread(NULL, 0, ClientMain, (LPVOID)"C:\\Users\\OHT\\Desktop\\University\\3Y-2S\\NetworkGP\\NetworkGP_Practices\\ClientPractices\\Assignment05_Client\\test.mp4", 0, NULL);
            CreateThread(NULL, 0, ClientMain, (LPVOID)fileName, 0, NULL);
            return TRUE;

        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            closesocket(sock);
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

// 파일 전송 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg)
{
    int retval;

    // 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) DisplayError("소켓 생성 오류\r\n");

    // 서버 연결
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) DisplayText("서버 연결 오류\r\n");

    // 파일 이름 조정
    wchar_t* fileName = (wchar_t*)arg;
    char charFileName[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, fileName, -1, charFileName, MAX_PATH, NULL, NULL);
    char* mainFileName = strrchr(charFileName, '\\');
    if (mainFileName == NULL) mainFileName = charFileName;
    else mainFileName++;

    // 파일 열기
    int fileLen = strlen(mainFileName);
    FILE* fp = fopen(mainFileName, "rb");
    if (fp == NULL) {
        DisplayText("파일 입출력 오류\r\n");
        return 0;
    }

    // 파일명 크기 전송
    retval = send(sock, (char*)&fileLen, sizeof(int), 0);
    if (retval == SOCKET_ERROR) DisplayText("파일명 크기 전송 오류\r\n");

    // 파일명 크기만큼 전송
    retval = send(sock, mainFileName, fileLen, 0);
    if (retval == SOCKET_ERROR) DisplayText("파일명 전송 오류\r\n");

    // 전송할 파일 크기 구하기
    fseek(fp, 0, SEEK_END);
    int totalBytes = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // 전송할 파일 크기 보내기
    retval = send(sock, (char*)&totalBytes, sizeof(totalBytes), 0);
    if (retval == SOCKET_ERROR) DisplayText("파일 크기 전송 오류\r\n");

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
                DisplayText("파일 데이터 크기 전송 오류\r\n");
                break;
            }

            // 전송할 파일 데이터 크기만큼 데이터 보내기
            retval = send(sock, buf, numRead, 0);
            if (retval == SOCKET_ERROR) {
                DisplayText("파일 데이터 전송 오류\r\n");
                break;
            }

            numTotal += numRead;
            // 전송 진행률 업데이트
            int progress = (int)(((double)numTotal / totalBytes) * 100);
            SendMessage(hProgressBar, PBM_SETPOS, progress, 0);
        }

        else if (numRead == 0 && numTotal == totalBytes) {
            DisplayText("\n파일 전송 완료: %d 바이트\n", numTotal);
            break;
        }

        else {
            DisplayText("파일 전송 실패\n");
            break;
        }
    }

    fclose(fp);

    // 소켓 닫기
    closesocket(sock);
    EnableWindow(hSendButton, TRUE); // 전송 버튼 활성화
    return 0;
}

// 에디트 컨트롤 출력 함수
void DisplayText(const char* fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    char cbuf[BUFSIZE * 2];
    vsprintf(cbuf, fmt, arg);
    va_end(arg);

    int nLength = GetWindowTextLength(hEdit2);
    SendMessage(hEdit2, EM_SETSEL, nLength, nLength);
    SendMessageA(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
}

// 소켓 함수 오류 출력
void DisplayError(const char* msg)
{
    LPVOID lpMsgBuf;
    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, WSAGetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (char*)&lpMsgBuf, 0, NULL);
    DisplayText("[%s] %s\r\n", msg, (char*)lpMsgBuf);
    LocalFree(lpMsgBuf);
}