#include "..\..\Common.h"
#include <commdlg.h>
#include <commctrl.h>
#include "resource.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512

// ��ȭ���� ���ν���
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// ����Ʈ ��Ʈ�� ��� �Լ�
void DisplayText(const char* fmt, ...);
// ���� �Լ� ���� ���
void DisplayError(const char* msg);
// ���� ���� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg);

// ���� ����
SOCKET sock;
char buf[BUFSIZE + 1];
HWND hSendButton;
HWND hEdit1, hEdit2, hProgressBar;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // ��ȭ���� ����
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

    // ���� ����
    WSACleanup();
    return 0;
}

// ��ȭ���� ���ν���
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
            // ���� ���� ��ư Ŭ�� �� ���� ���� ��ȭ���� ȣ��
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
                SetDlgItemText(hDlg, IDC_EDIT1, fileName); // ������ ���ϸ��� ����Ʈ ��Ʈ�ѿ� ǥ��
            }
        }
        return TRUE;

        case IDOK:
            // ���� ��ư Ŭ�� �� ���� ���� ������ ����
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

// ���� ���� ������ �Լ�
DWORD WINAPI ClientMain(LPVOID arg)
{
    int retval;

    // ���� ����
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) DisplayError("���� ���� ����\r\n");

    // ���� ����
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(SERVERIP);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) DisplayText("���� ���� ����\r\n");

    // ���� �̸� ����
    wchar_t* fileName = (wchar_t*)arg;
    char charFileName[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, fileName, -1, charFileName, MAX_PATH, NULL, NULL);
    char* mainFileName = strrchr(charFileName, '\\');
    if (mainFileName == NULL) mainFileName = charFileName;
    else mainFileName++;

    // ���� ����
    int fileLen = strlen(mainFileName);
    FILE* fp = fopen(mainFileName, "rb");
    if (fp == NULL) {
        DisplayText("���� ����� ����\r\n");
        return 0;
    }

    // ���ϸ� ũ�� ����
    retval = send(sock, (char*)&fileLen, sizeof(int), 0);
    if (retval == SOCKET_ERROR) DisplayText("���ϸ� ũ�� ���� ����\r\n");

    // ���ϸ� ũ�⸸ŭ ����
    retval = send(sock, mainFileName, fileLen, 0);
    if (retval == SOCKET_ERROR) DisplayText("���ϸ� ���� ����\r\n");

    // ������ ���� ũ�� ���ϱ�
    fseek(fp, 0, SEEK_END);
    int totalBytes = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // ������ ���� ũ�� ������
    retval = send(sock, (char*)&totalBytes, sizeof(totalBytes), 0);
    if (retval == SOCKET_ERROR) DisplayText("���� ũ�� ���� ����\r\n");

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
                DisplayText("���� ������ ũ�� ���� ����\r\n");
                break;
            }

            // ������ ���� ������ ũ�⸸ŭ ������ ������
            retval = send(sock, buf, numRead, 0);
            if (retval == SOCKET_ERROR) {
                DisplayText("���� ������ ���� ����\r\n");
                break;
            }

            numTotal += numRead;
            // ���� ����� ������Ʈ
            int progress = (int)(((double)numTotal / totalBytes) * 100);
            SendMessage(hProgressBar, PBM_SETPOS, progress, 0);
        }

        else if (numRead == 0 && numTotal == totalBytes) {
            DisplayText("\n���� ���� �Ϸ�: %d ����Ʈ\n", numTotal);
            break;
        }

        else {
            DisplayText("���� ���� ����\n");
            break;
        }
    }

    fclose(fp);

    // ���� �ݱ�
    closesocket(sock);
    EnableWindow(hSendButton, TRUE); // ���� ��ư Ȱ��ȭ
    return 0;
}

// ����Ʈ ��Ʈ�� ��� �Լ�
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

// ���� �Լ� ���� ���
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