#include "..\..\Common.h"
#include "resource.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    64

#pragma comment(lib, "comctl32.lib")

SOCKET sock;                    // 소켓
HWND hProgress;                // 프로그레스바 핸들
HWND hEdit;                    // 상태 출력용 에디트 컨트롤
BOOL isTransferring = FALSE;   // 파일 전송 중 플래그

// 함수 선언
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void DisplayText(const char* fmt, ...);
DWORD WINAPI FileTransferThread(LPVOID arg);

// 파일 전송 데이터 구조체
struct FileTransferData {
    char filepath[MAX_PATH];
    HWND hDlg;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    // 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    // 대화상자 생성
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    // 윈속 종료
    WSACleanup();
    return 0;
}

INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM)
{
    static OPENFILENAMEW ofn;
    static wchar_t filepath[MAX_PATH];

    switch (uMsg) {
    case WM_INITDIALOG:
    {
        // 컨트롤 초기화
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_PROGRESS_CLASS;
        InitCommonControlsEx(&icex);

        // 핸들 저장
        hEdit = GetDlgItem(hDlg, IDC_EDIT1);
        hProgress = GetDlgItem(hDlg, IDC_PROGRESS1);

        // 프로그레스바 초기화
        SendMessage(hProgress, PBM_SETRANGE32, 0, 100);
        SendMessage(hProgress, PBM_SETPOS, 0, 0);

        // 파일 선택 다이얼로그 초기화
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hDlg;
        // 필터
        ofn.lpstrFilter = L"비디오 파일(*.mp4;*.avi)\0*.mp4;*.avi\0모든 파일(*.*)\0*.*\0";
        // 파일의 경로가 저장될 버퍼
        ofn.lpstrFile = filepath;
        // 파일 경로를 저장할 버퍼 최대 크기
        ofn.nMaxFile = MAX_PATH;
        // 실제 존재 파일과 실제 존재 경로만 선택 가능
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BUTTON1:
            // 파일 전송중이 아닐 때
            if (!isTransferring) {
                // 이전 선택 파일 경로 초기화
                filepath[0] = '\0';
                // 파일 열기 대화상자 표시
                if (GetOpenFileNameW(&ofn)) {
                    // 소켓 생성
                    sock = socket(AF_INET, SOCK_STREAM, 0);
                    if (sock == INVALID_SOCKET) {
                        err_quit("socket()");
                        return TRUE;
                    }
                    // 서버 연결
                    struct sockaddr_in serveraddr;
                    ZeroMemory(&serveraddr, sizeof(serveraddr));
                    serveraddr.sin_family = AF_INET;
                    if (inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr) != 1) {
                        err_quit("inet_pton()");
                        closesocket(sock);
                        return TRUE;
                    }
                    // connect()
                    serveraddr.sin_port = htons(SERVERPORT);
                    if (connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR) {
                        err_quit("connect()");
                        closesocket(sock);
                        return TRUE;
                    }

                    // 파일 전송 시작
                    FileTransferData* ftData = new FileTransferData;
                    // 문자열 변환
                    WideCharToMultiByte(CP_ACP, 0, filepath, -1, ftData->filepath, MAX_PATH, NULL, NULL);
                    ftData->hDlg = hDlg;
                    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
                    isTransferring = TRUE;

                    // 파일 전송 스레드 시작
                    std::thread fileTransferThread(FileTransferThread, ftData);
                    fileTransferThread.detach();
                }
            }
            return TRUE;

        case IDCANCEL:
            if (isTransferring) {
                closesocket(sock);
            }
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

// 파일 전송 스레드 함수
DWORD WINAPI FileTransferThread(LPVOID arg)
{
    FileTransferData* ftData = (FileTransferData*)arg;
    char* filepath = ftData->filepath;
    HWND hDlg = ftData->hDlg;

    // 파일 열기
    FILE* fp = fopen(filepath, "rb");
    if (fp == NULL) {
        DisplayText("파일을 열 수 없습니다: %s\n", filepath);
        return -1;
    }

    // 파일 크기 계산
    fseek(fp, 0, SEEK_END);
    long long filesize = _ftelli64(fp);
    fseek(fp, 0, SEEK_SET);

    // 파일 이름 추출
    char* filename = strrchr(filepath, '\\');
    if (filename == NULL) filename = filepath;
    else filename++;

    // 파일 이름 길이 전송 : 고정 길이
    int namelen = (int)strlen(filename);
    if (send(sock, (char*)&namelen, sizeof(namelen), 0) == SOCKET_ERROR) {
        err_quit("send()");
        return -1;
    }

    // 파일 이름 전송
    if (send(sock, filename, namelen, 0) == SOCKET_ERROR) {
        err_quit("send()");
        return -1;
    }

    // 파일 크기 전송
    if (send(sock, (char*)&filesize, sizeof(filesize), 0) == SOCKET_ERROR) {
        err_quit("send()");
        return -1;
    }

    // 파일 데이터 전송 : 가변 길이
    char buf[BUFSIZE];
    long long totalSent = 0;
    while (totalSent < filesize) {
        // 크기 계산
        int readlen = (int)min(BUFSIZE, filesize - totalSent);

        // 데이터 읽기
        int retval = fread(buf, 1, readlen, fp);

        // 데이터 전송
        if (retval > 0) {
            if (send(sock, buf, retval, 0) == SOCKET_ERROR) {
                err_quit("send()");
                break;
            }
            totalSent += retval;

            // 프로그레스바 업데이트
            int percent = (int)((double)totalSent * 100 / filesize);
            SendMessage(hProgress, PBM_SETPOS, percent, 0);
        }
        else {
            break;
        }
    }

    DisplayText("\n파일 전송 완료: %s\n", filename);
    if (fp != NULL) fclose(fp);
    closesocket(sock);
    SendMessage(hProgress, PBM_SETPOS, 0, 0);
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), TRUE);
    isTransferring = FALSE;
    delete ftData;
    return 0;
}

// 출력 함수
void DisplayText(const char* fmt, ...)
{
    va_list arg;
    va_start(arg, fmt);
    char cbuf[BUFSIZE * 2];
    vsprintf(cbuf, fmt, arg);
    va_end(arg);

    int nLength = GetWindowTextLength(hEdit);
    SendMessage(hEdit, EM_SETSEL, nLength, nLength);
    SendMessageA(hEdit, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
}