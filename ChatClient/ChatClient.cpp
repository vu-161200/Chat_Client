#include "framework.h"
#include "ChatClient.h"
#include "stdio.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "winsock2.h"

#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)

#define MAX_LOADSTRING 100

#define WM_SOCKET WM_USER + 1
#define WM_PRIVATE_MESSAGE WM_USER + 2
HINSTANCE hInst;                                
WCHAR szTitle[MAX_LOADSTRING];                  
WCHAR szWindowClass[MAX_LOADSTRING];            

typedef struct {
    char id[32];
    HWND hDlg;
} CHAT_INFO;

SOCKET client;
char clientId[32];

CHAT_INFO chatDlgs[64];
int numChatDlgs;

HWND hDlgConnect;
HWND hWndListMessage, hWndListClient, hWndEditMessage, hWndBtnSend;


ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    AboutDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ConnectDlgProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    ChatDlgProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   /* SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8000);
    system("pause");*/
    numChatDlgs = 0;

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CHATCLIENT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHATCLIENT));
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CHATCLIENT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_CHATCLIENT);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    return RegisterClassExW(&wcex);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; 

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, 0, 500, 340, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    hWndListMessage = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("LISTBOX"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOVSCROLL,
        10, 10, 350, 200, hWnd, (HMENU)IDC_LIST_MESSAGE, GetModuleHandle(NULL), NULL);

    hWndListClient = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("LISTBOX"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOVSCROLL | LBS_NOTIFY,
        370, 10, 100, 200, hWnd, (HMENU)IDC_LIST_CLIENT, GetModuleHandle(NULL), NULL);

    hWndEditMessage = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), TEXT(""), WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        10, 220, 350, 40, hWnd, (HMENU)IDC_EDIT_MESSAGE, GetModuleHandle(NULL), NULL);

    hWndBtnSend = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("BUTTON"), TEXT("Send"), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
        370, 220, 100, 40, hWnd, (HMENU)IDC_BUTTON_SEND, GetModuleHandle(NULL), NULL);

    EnableWindow(hWndListMessage, FALSE);
    EnableWindow(hWndListClient, FALSE);
    EnableWindow(hWndEditMessage, FALSE);
    EnableWindow(hWndBtnSend, FALSE);
    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SOCKET:
    {
        if (WSAGETSELECTERROR(lParam))
        {
            closesocket(wParam);
            return 0;
        }
        if (WSAGETSELECTEVENT(lParam) == FD_READ)
        {
            char buf[256];
            char cmd[32], state[32];
            int ret = recv(client, buf, sizeof(buf), 0);
            buf[ret] = 0;

            SendDlgItemMessageA(hWnd, IDC_LIST_MESSAGE, LB_ADDSTRING, 0, (LPARAM)buf);

            sscanf(buf, "%s%s", cmd, state);
            if (strcmp(cmd, "CONNECT") == 0)
            {
                if (strcmp(state, "OK") == 0)
                {
                    EndDialog(hDlgConnect, 0);
                    EnableMenuItem(GetMenu(hWnd), ID_FILE_CONNECT, MF_DISABLED);
                    EnableMenuItem(GetMenu(hWnd), ID_FILE_DISCONNECT, MF_ENABLED);
                    EnableWindow(hWndListMessage, TRUE);
                    EnableWindow(hWndListClient, TRUE);
                    EnableWindow(hWndEditMessage, TRUE);
                    EnableWindow(hWndBtnSend, TRUE);
                    send(client, "LIST", 4, 0);
                }
                else
                {
                    char* errorMsg = buf + 14;
                    MessageBoxA(hWnd, errorMsg, "Error", MB_OK);
                }
            }
            else if (strcmp(cmd, "DISCONNECT") == 0)
            {
                if (strcmp(state, "OK") == 0)
                {
                    EnableMenuItem(GetMenu(hWnd), ID_FILE_CONNECT, MF_ENABLED);
                    EnableMenuItem(GetMenu(hWnd), ID_FILE_DISCONNECT, MF_DISABLED);

                   
                    SendDlgItemMessageA(hWnd, IDC_LIST_CLIENT, LB_RESETCONTENT, 0, 0);
                    SendDlgItemMessageA(hWnd, IDC_LIST_MESSAGE, LB_RESETCONTENT, 0, 0);
                    SetDlgItemTextA(hWnd, IDC_EDIT_MESSAGE, "");

                    EnableWindow(hWndListMessage, FALSE);
                    EnableWindow(hWndListClient, FALSE);
                    EnableWindow(hWndEditMessage, FALSE);
                    EnableWindow(hWndBtnSend, FALSE);

                    closesocket(client);
                }
            }
            else if (strcmp(cmd, "LIST") == 0)
            {
                if (strcmp(state, "OK") == 0)
                {
                    char* listIds = buf + 8;
                    char* pid = strtok(listIds, ",");
                    while (pid != NULL)
                    {
                        SendDlgItemMessageA(hWnd, IDC_LIST_CLIENT, LB_ADDSTRING, 0, (LPARAM)pid);
                        pid = strtok(NULL, ",");
                    }
                }
            }
            else if (strcmp(cmd, "MESSAGE") == 0)
            {
                char* msg = buf + strlen("MESSAGE") + strlen(state) + 2;
                int i = 0;
                for (; i < numChatDlgs; i++)
                    if (strcmp(chatDlgs[i].id, state) == 0)
                        break;

                if (i < numChatDlgs)
                    SendMessageA(chatDlgs[i].hDlg, WM_PRIVATE_MESSAGE, (WPARAM)msg, 0);
            }
            else if (strcmp(cmd, "USER_CONNECT") == 0)
            {
                SendDlgItemMessageA(hWnd, IDC_LIST_CLIENT, LB_RESETCONTENT, 0, 0);
                send(client, "LIST", 4, 0);
            }
            else if (strcmp(cmd, "USER_DISCONNECT") == 0)
            {
                SendDlgItemMessageA(hWnd, IDC_LIST_CLIENT, LB_RESETCONTENT, 0, 0);
                send(client, "LIST", 4, 0);
            }
        }

        if (WSAGETSELECTEVENT(lParam) == FD_CLOSE)
        {
            closesocket(wParam);
            return 0;
        }
    }
    break;
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case ID_FILE_CONNECT:
        {
            SOCKADDR_IN addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            addr.sin_port = htons(8000);

            int ret = connect(client, (SOCKADDR*)&addr, sizeof(addr));
            if (ret == SOCKET_ERROR)
            {
                MessageBoxA(hWnd, "Cannot connect to server.", "Error", MB_OK);
            }
            else
            {
                WSAAsyncSelect(client, hWnd, WM_SOCKET, FD_READ | FD_CLOSE);

                hDlgConnect = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG_CONNECT), hWnd, ConnectDlgProc);
                ShowWindow(hDlgConnect, SW_SHOW);
            }
        }

        break;
        case ID_FILE_DISCONNECT:
        {
            send(client, "DISCONNECT", 10, 0);
        }
        break;
        case IDC_BUTTON_SEND:
        {
            char buf[256];
            GetDlgItemTextA(hWnd, IDC_EDIT_MESSAGE, buf, sizeof(buf));
            if (strlen(buf) > 0)
            {
                char sendBuf[256] = "SEND ALL ";
                strcat(sendBuf, buf);
                send(client, sendBuf, strlen(sendBuf), 0);
                SetDlgItemTextA(hWnd, IDC_EDIT_MESSAGE, "");
            }
        }
        break;
        case IDC_LIST_CLIENT:
        {
            int code = HIWORD(wParam);
            if (code == LBN_DBLCLK)
            {
                char targetId[32];

                int idx = SendDlgItemMessageA(hWnd, IDC_LIST_CLIENT, LB_GETCURSEL, 0, 0);
                SendDlgItemMessageA(hWnd, IDC_LIST_CLIENT, LB_GETTEXT, idx, (LPARAM)targetId);

                int n = strlen(targetId);
                if (targetId[n - 1] == '\n')
                    targetId[n - 1] = 0;

                if (strcmp(clientId, targetId) == 0)
                    return 0;

                int i = 0;
                for (; i < numChatDlgs; i++)
                    if (strcmp(chatDlgs[i].id, targetId) == 0)
                        break;

                if (i < numChatDlgs)
                    return 0;
                HWND hDlg = CreateDialog(hInst, MAKEINTRESOURCE(IDD_DIALOG_CHAT), hWnd, ChatDlgProc);
                ShowWindow(hDlg, SW_SHOW);

                char buf[64] = "Chat with ";
                strcat(buf, targetId);

                SetWindowTextA(hDlg, buf);

                chatDlgs[numChatDlgs].hDlg = hDlg;
                memcpy(chatDlgs[numChatDlgs].id, targetId, strlen(targetId) + 1);
                numChatDlgs++;
            }
        }
        break;
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, AboutDlgProc);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    return 0;
}
INT_PTR CALLBACK AboutDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

INT_PTR CALLBACK ConnectDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
        case IDOK:
        {
            char buf[32];
            GetDlgItemTextA(hDlg, IDC_EDIT_CLIENT_ID, buf, sizeof(buf));
            if (strlen(buf) > 0)
            {
                memcpy(clientId, buf, strlen(buf) + 1);
                char cmd[64] = "CONNECT ";
                strcat(cmd, buf);
                send(client, cmd, strlen(cmd), 0);
            }
        }
        break;
        case IDCANCEL:
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
        }
    }
    }
    return (INT_PTR)FALSE;
}
INT_PTR CALLBACK ChatDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_BUTTON_SEND)
        {
            int i = 0;
            for (; i < numChatDlgs; i++)
                if (chatDlgs[i].hDlg == hDlg)
                    break;

            if (i < numChatDlgs)
            {
                char buf[256];
                GetDlgItemTextA(hDlg, IDC_EDIT_MESSAGE_PRIVATE, buf, sizeof(buf));
                if (strlen(buf) > 0)
                {
                    char sendBuf[256];
                    sprintf(sendBuf, "SEND %s %s\n", chatDlgs[i].id, buf);
                    send(client, sendBuf, strlen(sendBuf), 0);
                    SetDlgItemTextA(hDlg, IDC_EDIT_MESSAGE_PRIVATE, "");
                    SendDlgItemMessageA(hDlg, IDC_LIST_MESSAGE, LB_ADDSTRING, 0, (LPARAM)buf);
                }
            }

            return (INT_PTR)TRUE;
        }
        break;
    case WM_PRIVATE_MESSAGE:
    {
        int i = 0;
        for (; i < numChatDlgs; i++)
            if (chatDlgs[i].hDlg == hDlg)
                break;

        if (i < numChatDlgs)
        {
            char buf[256];
            sprintf(buf, "%s: %s", chatDlgs[i].id, wParam);
            SendDlgItemMessageA(hDlg, IDC_LIST_MESSAGE, LB_ADDSTRING, 0, (LPARAM)buf);
        }
    }
    break;
    case WM_CLOSE:
    {
        int i = 0;
        for (; i < numChatDlgs; i++)
            if (chatDlgs[i].hDlg == hDlg)
                break;

        if (i < numChatDlgs)
        {
            if (i < numChatDlgs - 1)
                chatDlgs[i] = chatDlgs[numChatDlgs - 1];
            numChatDlgs--;
        }
    }
    return (INT_PTR)FALSE;
    }

    return (INT_PTR)FALSE;
}
