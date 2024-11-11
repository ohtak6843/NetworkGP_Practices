#include "..\..\Common.h"
#include "resource.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    64

#pragma comment(lib, "comctl32.lib")

SOCKET sock;                    // ����
HWND hProgress;                // ���α׷����� �ڵ�
HWND hEdit;                    // ���� ��¿� ����Ʈ ��Ʈ��
BOOL isTransferring = FALSE;   // ���� ���� �� �÷���

// �Լ� ����
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void DisplayText(const char* fmt, ...);
DWORD WINAPI FileTransferThread(LPVOID arg);

// ���� ���� ������ ����ü
struct FileTransferData {
    char filepath[MAX_PATH];
    HWND hDlg;
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    // ���� �ʱ�ȭ
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    // ��ȭ���� ����
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    // ���� ����
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
        // ��Ʈ�� �ʱ�ȭ
        INITCOMMONCONTROLSEX icex;
        icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
        icex.dwICC = ICC_PROGRESS_CLASS;
        InitCommonControlsEx(&icex);

        // �ڵ� ����
        hEdit = GetDlgItem(hDlg, IDC_EDIT1);
        hProgress = GetDlgItem(hDlg, IDC_PROGRESS1);

        // ���α׷����� �ʱ�ȭ
        SendMessage(hProgress, PBM_SETRANGE32, 0, 100);
        SendMessage(hProgress, PBM_SETPOS, 0, 0);

        // ���� ���� ���̾�α� �ʱ�ȭ
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hDlg;
        // ����
        ofn.lpstrFilter = L"���� ����(*.mp4;*.avi)\0*.mp4;*.avi\0��� ����(*.*)\0*.*\0";
        // ������ ��ΰ� ����� ����
        ofn.lpstrFile = filepath;
        // ���� ��θ� ������ ���� �ִ� ũ��
        ofn.nMaxFile = MAX_PATH;
        // ���� ���� ���ϰ� ���� ���� ��θ� ���� ����
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

        return TRUE;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BUTTON1:
            // ���� �������� �ƴ� ��
            if (!isTransferring) {
                // ���� ���� ���� ��� �ʱ�ȭ
                filepath[0] = '\0';
                // ���� ���� ��ȭ���� ǥ��
                if (GetOpenFileNameW(&ofn)) {
                    // ���� ����
                    sock = socket(AF_INET, SOCK_STREAM, 0);
                    if (sock == INVALID_SOCKET) {
                        err_quit("socket()");
                        return TRUE;
                    }
                    // ���� ����
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

                    // ���� ���� ����
                    FileTransferData* ftData = new FileTransferData;
                    // ���ڿ� ��ȯ
                    WideCharToMultiByte(CP_ACP, 0, filepath, -1, ftData->filepath, MAX_PATH, NULL, NULL);
                    ftData->hDlg = hDlg;
                    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), FALSE);
                    isTransferring = TRUE;

                    // ���� ���� ������ ����
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

// ���� ���� ������ �Լ�
DWORD WINAPI FileTransferThread(LPVOID arg)
{
    FileTransferData* ftData = (FileTransferData*)arg;
    char* filepath = ftData->filepath;
    HWND hDlg = ftData->hDlg;

    // ���� ����
    FILE* fp = fopen(filepath, "rb");
    if (fp == NULL) {
        DisplayText("������ �� �� �����ϴ�: %s\n", filepath);
        return -1;
    }

    // ���� ũ�� ���
    fseek(fp, 0, SEEK_END);
    long long filesize = _ftelli64(fp);
    fseek(fp, 0, SEEK_SET);

    // ���� �̸� ����
    char* filename = strrchr(filepath, '\\');
    if (filename == NULL) filename = filepath;
    else filename++;

    // ���� �̸� ���� ���� : ���� ����
    int namelen = (int)strlen(filename);
    if (send(sock, (char*)&namelen, sizeof(namelen), 0) == SOCKET_ERROR) {
        err_quit("send()");
        return -1;
    }

    // ���� �̸� ����
    if (send(sock, filename, namelen, 0) == SOCKET_ERROR) {
        err_quit("send()");
        return -1;
    }

    // ���� ũ�� ����
    if (send(sock, (char*)&filesize, sizeof(filesize), 0) == SOCKET_ERROR) {
        err_quit("send()");
        return -1;
    }

    // ���� ������ ���� : ���� ����
    char buf[BUFSIZE];
    long long totalSent = 0;
    while (totalSent < filesize) {
        // ũ�� ���
        int readlen = (int)min(BUFSIZE, filesize - totalSent);

        // ������ �б�
        int retval = fread(buf, 1, readlen, fp);

        // ������ ����
        if (retval > 0) {
            if (send(sock, buf, retval, 0) == SOCKET_ERROR) {
                err_quit("send()");
                break;
            }
            totalSent += retval;

            // ���α׷����� ������Ʈ
            int percent = (int)((double)totalSent * 100 / filesize);
            SendMessage(hProgress, PBM_SETPOS, percent, 0);
        }
        else {
            break;
        }
    }

    DisplayText("\n���� ���� �Ϸ�: %s\n", filename);
    if (fp != NULL) fclose(fp);
    closesocket(sock);
    SendMessage(hProgress, PBM_SETPOS, 0, 0);
    EnableWindow(GetDlgItem(hDlg, IDC_BUTTON1), TRUE);
    isTransferring = FALSE;
    delete ftData;
    return 0;
}

// ��� �Լ�
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