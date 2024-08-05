#include "framework.h"
#include "BUS_MAN.h"
#include "Traffic.h"
#include "Support.h"
#include "drawing.h"

#define MAX_LOADSTRING 100
#define STATIONS_NUM 15
#define BUS_NUM 10

#define StartOneButton_id 0x001
#define StartAllButton_id 0x002
#define StopOneButton_id 0x003
#define StopAllButton_id 0x004
#define AddButton_id 0x005
#define RemoveButton_id 0x006
#define SeeAllCB_id 0x007
#define IDT_TIMER1 0x101

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND    hMainWindow,
        hPathWindow,
        hBusLB,
        hStationLB,
        hSeeAllCB,
        hStartOneB,
        hStartAllB,
        hStopOneB,
        hStopAllB,
        hRemoveB,
        hAddB,
        hStatusEB,
        hSpeedEB,
        hCoordEB,
        hBusArriveTimeL,
        hBusDepartTimeL,
        hBusArriveTimeEB,
        hDirectionEB,
        hClstLeftEB,
        hClstRightEB,
        hArriveTimeLeftEB,
        hArriveTimeRightEB;

RECT drawRect;
HDC buffDC;
HBITMAP buffBMP;
HANDLE hStopDrawingEvent;

HBRUSH blackBrush = CreateSolidBrush(COLOR_ACTIVEBORDER);
HBRUSH whiteBrush = CreateSolidBrush(RGB(255, 255, 255));
HBRUSH greenBrush = CreateSolidBrush(RGB(0, 128, 0));
HBRUSH orangeBrush = CreateSolidBrush(RGB(255, 140, 0));
HBRUSH blueBrush = CreateSolidBrush(RGB(0, 200, 0));

tr::Traffic traffic(BUS_NUM, STATIONS_NUM);
dr::Drawing drawing;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void UpdateBusUI(int bus_id);
void UpdateStationUI(int station_id);
void AddControl(HWND hWnd);
DWORD DrawFrame(LPVOID lpParam);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    
    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_BUSMAN, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_BUSMAN));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_BUSMAN));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hMainWindow = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,
      CW_USEDEFAULT, 0, 745, 455, nullptr, nullptr, hInstance, nullptr);

   if (!hMainWindow)
   {
      return FALSE;
   }

   //   fill list with buses
   for(int i = 0;i<traffic.BusCount(); i++)
       SendMessage(hBusLB, LB_ADDSTRING, 0, (LPARAM)traffic.Roster(i).Name());
   SendMessage(hBusLB, LB_SETCURSEL, (WPARAM)0, NULL);

   //   fill list with stations
   int k = 15;
   WCHAR* string = new WCHAR[k];
   for (int i = 0; i < traffic.Route().Count(); i++)
   {
       _snwprintf_s(string, k, k, L"Станция №%d\0", i);
       SendMessage(hStationLB, LB_ADDSTRING, 0, (LPARAM)string);
   }
   delete[] string;
   SendMessage(hStationLB, LB_SETCURSEL, (WPARAM)0, NULL);

   ShowWindow(hMainWindow, nCmdShow);
   UpdateWindow(hMainWindow);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        AddControl(hWnd);
        drawRect.left = 15;
        drawRect.top = 15;
        drawRect.right = 715;
        drawRect.bottom = 125;
        traffic.UpdateClstBus();
        drawing.Init(traffic, drawRect);
        hStopDrawingEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        HANDLE hThread = CreateThread(NULL, NULL, &DrawFrame, 0, NULL, NULL);
        if (hThread == NULL)
        {
            MessageBox(hWnd, L"Не удалось запустить отрисовку карты.", L"Error", MB_ICONERROR);
            PostMessage(hWnd, WM_DESTROY, 0, 0);
            DestroyWindow(hWnd);
            return 0;
        }
        CloseHandle(hThread);
        SetTimer(hWnd, IDT_TIMER1, 100, NULL);
        UpdateBusUI(0);
        UpdateStationUI(0);
        break;
    }
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case StartOneButton_id:
        {
            int id = SendMessage(hBusLB, LB_GETCURSEL, 0, 0);
            if (id >= 0 && id < traffic.BusCount())
                traffic.Roster(id).RunBus();
            return 0;
        }
        case StartAllButton_id:
        {
            for (int i = 0; i < traffic.BusCount(); i++)
                traffic.Roster(i).RunBus();
            return 0;
        }
        case StopOneButton_id:
        {
            int id = SendMessage(hBusLB, LB_GETCURSEL, 0, 0);
            if(id >= 0 && id < traffic.BusCount())
                traffic.Roster(id).TurnOff();
            return 0;
        }
        case StopAllButton_id:
        {
            for (int i = 0; i < traffic.BusCount(); i++)
                traffic.Roster(i).TurnOff();
            return 0;
        }
        case AddButton_id:
        {
            traffic.AddBus();
            int id = traffic.BusCount() - 1;
            SendMessage(hBusLB, LB_ADDSTRING, 0, (LPARAM)traffic.Roster(id).Name());
            SendMessage(hBusLB, LB_SETCURSEL, (WPARAM)id, 0);
            return 0;
        }
        case RemoveButton_id:
        {
            int id = SendMessage(hBusLB, LB_GETCURSEL, 0, 0);
            if (id < 0) 
            {
                MessageBox(hWnd, L"Автобусов не осталось!", L"Warning", MB_ICONWARNING);
                return 0;
            }
            SendMessage(hBusLB, LB_DELETESTRING, (WPARAM)id, 0);
            if (id < traffic.BusCount())
            {
                traffic.RemoveBus(id);
                if (id == traffic.BusCount())
                    --id;
                SendMessage(hBusLB, LB_SETCURSEL, (WPARAM)(id), 0);
            }
            UpdateBusUI(id);
            id = SendMessage(hStationLB, LB_GETCURSEL, 0, 0);
            UpdateStationUI(id);
            return 0;
        }
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    }
    case WM_TIMER:
    {
        int busId = SendMessage(hBusLB, LB_GETCURSEL, 0, 0);
        int stationId = SendMessage(hStationLB, LB_GETCURSEL, 0, 0);
        if (busId < 0 || stationId < 0)    break;
        if (traffic.Roster(busId).IsUpdated())
            UpdateBusUI(busId);
        traffic.UpdateClstBus(stationId);
        UpdateStationUI(stationId);
        break;
    }
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcEdit = (HDC)wParam;
        HWND hwnd = (HWND)lParam;
        if (hwnd == hStatusEB)
        {
            int bus_id = SendMessage(hBusLB, LB_GETCURSEL, 0, 0);
            COLORREF bkColor = RGB(255, 255, 255);
            RECT RectEB;
            GetClientRect(hStatusEB, &RectEB);

            if (bus_id >= 0 && bus_id < traffic.BusCount())
                switch (traffic.Roster(bus_id).Status())
                {
                case 0:
                {
                    bkColor = RGB(255, 255, 255);
                    break;
                }
                case 1:
                {
                    bkColor = RGB(0, 128, 0);
                    break;
                }
                case 2:
                {
                    bkColor = RGB(255, 140, 0);
                    break;
                }
                }
            SetBkColor(hdcEdit, bkColor);
            return (INT_PTR)GetStockObject(bkColor);
        }
        break;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps1;

        HDC hdc = BeginPaint(hWnd, &ps1);
        BitBlt(hdc, drawRect.left, drawRect.top, drawRect.right, drawRect.bottom, buffDC, 0, 0, SRCCOPY);
        EndPaint(hWnd, &ps1);
        break;
    }
    case WM_DESTROY:
        SetEvent(hStopDrawingEvent);
        CloseHandle(hStopDrawingEvent);
        KillTimer(hMainWindow, IDT_TIMER1);
        for (int i = 0; i < traffic.BusCount(); i++)
            traffic.Roster(i).TurnOff();
        DeleteObject(whiteBrush);
        DeleteObject(blackBrush);
        DeleteObject(greenBrush);
        DeleteObject(orangeBrush);
        DeleteObject(blueBrush);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

void UpdateBusUI(int bus_id)
{
    //  cleaning fields
    SendMessage(hStatusEB, EM_SETSEL, 0, -1);
    SendMessage(hStatusEB, WM_CLEAR, 0, 0);

    SendMessage(hSpeedEB, EM_SETSEL, 0, -1);
    SendMessage(hSpeedEB, WM_CLEAR, 0, 0);

    SendMessage(hBusArriveTimeEB, EM_SETSEL, 0, -1);
    SendMessage(hBusArriveTimeEB, WM_CLEAR, 0, 0);

    SendMessage(hCoordEB, EM_SETSEL, 0, -1);
    SendMessage(hCoordEB, WM_CLEAR, 0, 0);

    SendMessage(hDirectionEB, EM_SETSEL, 0, -1);
    SendMessage(hDirectionEB, WM_CLEAR, 0, 0);

    HDC hdc = GetDC(hStatusEB);
    RECT RectEB;
    GetClientRect(hStatusEB, &RectEB);
    FillRect(hdc, &RectEB, whiteBrush);

    if (bus_id < 0 || traffic.BusCount() == 0)
    {
        FrameRect(hdc, &RectEB, blackBrush);
        SendMessage(hStatusEB, EM_REPLACESEL, 0, (LPARAM)L"");
        SendMessage(hSpeedEB, EM_REPLACESEL, 0, (LPARAM)L"");
        SendMessage(hBusArriveTimeEB, EM_REPLACESEL, 0, (LPARAM)L"");
        SendMessage(hCoordEB, EM_REPLACESEL, 0, (LPARAM)L"");
        SendMessage(hDirectionEB, EM_REPLACESEL, 0, (LPARAM)L"");
        return;
    }

    switch (traffic.Roster(bus_id).Status())
    {
    case 0:
    {
        ShowWindow(hBusDepartTimeL, SW_HIDE);
        ShowWindow(hBusArriveTimeL, SW_SHOW);
        SendMessage(hStatusEB, EM_REPLACESEL, 0, (LPARAM)L"Выключен");
        SendMessage(hSpeedEB, EM_REPLACESEL, 0, (LPARAM)L"");
        SendMessage(hBusArriveTimeEB, EM_REPLACESEL, 0, (LPARAM)L"");
        SendMessage(hCoordEB, EM_REPLACESEL, 0, (LPARAM)L"");
        SendMessage(hDirectionEB, EM_REPLACESEL, 0, (LPARAM)L"");
        break;
    }
    case 1:
    {
        ShowWindow(hBusDepartTimeL, SW_HIDE);
        ShowWindow(hBusArriveTimeL, SW_SHOW);
        FillRect(hdc, &RectEB, greenBrush);
        SendMessage(hStatusEB, EM_REPLACESEL, 0, (LPARAM)L"В пути");
        SendMessage(hSpeedEB, EM_REPLACESEL, 0, (LPARAM)to_string(traffic.Roster(bus_id).Speed()).get());
        SendMessage(hBusArriveTimeL, EM_REPLACESEL, 0, (LPARAM)L"До след. станции");
        SendMessage(hBusArriveTimeEB, EM_REPLACESEL, 0, (LPARAM)to_string(traffic.Roster(bus_id).TimeToArrive() / (double)10, 1).get());
        SendMessage(hCoordEB, EM_REPLACESEL, 0, (LPARAM)to_string(traffic.Roster(bus_id).LocCoord()).get());

        int k = 11;
        WCHAR* string = new WCHAR[k];
        _snwprintf_s(string, k, k, L"Из %d в %d\0", traffic.Roster(bus_id).Depart(), traffic.Roster(bus_id).Dest());
        SendMessage(hDirectionEB, EM_REPLACESEL, 0, (LPARAM)string);
        delete[] string;
        break;
    }
    case 2:
    {

        ShowWindow(hBusArriveTimeL, SW_HIDE);
        ShowWindow(hBusDepartTimeL, SW_SHOW);
        FillRect(hdc, &RectEB, orangeBrush);
        SendMessage(hStatusEB, EM_REPLACESEL, 0, (LPARAM)L"Ожидает");
        SendMessage(hSpeedEB, EM_REPLACESEL, 0, (LPARAM)to_string(traffic.Roster(bus_id).Speed()).get());
        SendMessage(hBusArriveTimeEB, EM_REPLACESEL, 0, (LPARAM)to_string(traffic.Roster(bus_id).TimeToStay() / (double)10, 1).get());
        SendMessage(hCoordEB, EM_REPLACESEL, 0, (LPARAM)to_string(traffic.Roster(bus_id).LocCoord()).get());

        int k = 11;
        WCHAR* string = new WCHAR[k];
        _snwprintf_s(string, k, k, L"Из %d в %d\0", traffic.Roster(bus_id).Depart(), traffic.Roster(bus_id).Dest());
        SendMessage(hDirectionEB, EM_REPLACESEL, 0, (LPARAM)string);
        delete[] string;
        break;
    }
    }
    FrameRect(hdc, &RectEB, blackBrush);
    ReleaseDC(hStatusEB, hdc);
}

void UpdateStationUI(int station_id)
{
    //  cleaning fields
    SendMessage(hClstLeftEB, EM_SETSEL, 0, -1);
    SendMessage(hClstLeftEB, WM_CLEAR, 0, 0);

    SendMessage(hArriveTimeLeftEB, EM_SETSEL, 0, -1);
    SendMessage(hArriveTimeLeftEB, WM_CLEAR, 0, 0);

    SendMessage(hClstRightEB, EM_SETSEL, 0, -1);
    SendMessage(hClstRightEB, WM_CLEAR, 0, 0);

    SendMessage(hArriveTimeRightEB, EM_SETSEL, 0, -1);
    SendMessage(hArriveTimeRightEB, WM_CLEAR, 0, 0);

    if (traffic.BusCount() == 0)
    {
        SendMessage(hClstLeftEB, EM_REPLACESEL, 0, (LPARAM)L"");
        SendMessage(hArriveTimeLeftEB, EM_REPLACESEL, 0, (LPARAM)L"");
        SendMessage(hClstRightEB, EM_REPLACESEL, 0, (LPARAM)L"");
        SendMessage(hArriveTimeRightEB, EM_REPLACESEL, 0, (LPARAM)L"");
        return;
    }
    SendMessage(hClstLeftEB, EM_REPLACESEL, 0, (LPARAM)traffic.Route()[station_id].GetClstBusL().Name());
    SendMessage(hArriveTimeLeftEB, EM_REPLACESEL, 0, (LPARAM)to_string(traffic.Route()[station_id].ArrivingTimeL() / (double)10, 1).get());
    SendMessage(hClstRightEB, EM_REPLACESEL, 0, (LPARAM)traffic.Route()[station_id].GetClstBusR().Name());
    SendMessage(hArriveTimeRightEB, EM_REPLACESEL, 0, (LPARAM)to_string(traffic.Route()[station_id].ArrivingTimeR() / (double)10, 1).get());
}

void AddControl(HWND hWnd)
{
    hBusLB = CreateWindow(L"ListBox", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_STANDARD, 15, 140, \
        150, 220, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hSeeAllCB = CreateWindow(L"Button", L"Показать всех", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_AUTOCHECKBOX, 30, 365, \
        120, 23, hWnd, (HMENU)SeeAllCB_id, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    
    HANDLE hStatusL = CreateWindow(L"Static", L"Статус", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_SUNKEN, 175, 140, \
        125, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hStatusEB = CreateWindow(L"Edit", L"На стоянке", WS_BORDER | WS_VISIBLE | WS_CHILD | ES_CENTER | ES_READONLY, 192, 165, \
        90, 23, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);

    HANDLE hSpeedL = CreateWindow(L"Static", L"Скорость", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_SUNKEN, 175, 193, \
        125, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hSpeedEB = CreateWindow(L"Edit", L"0", WS_BORDER | WS_VISIBLE | WS_CHILD | ES_CENTER | ES_READONLY, 192, 218, \
        90, 23, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);

    hBusArriveTimeL = CreateWindow(L"Static", L"До след. станции", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_SUNKEN, 175, 246, \
        125, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hBusDepartTimeL = CreateWindow(L"Static", L"До отправления", WS_CHILD | SS_CENTER | SS_SUNKEN, 175, 246, \
        125, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hBusArriveTimeEB = CreateWindow(L"Edit", L"0", WS_BORDER | WS_VISIBLE | WS_CHILD | ES_CENTER | ES_READONLY, 192, 271, \
        90, 23, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);

    HANDLE hCoordL = CreateWindow(L"Static", L"Координата", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_SUNKEN, 175, 299, \
        125, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hCoordEB = CreateWindow(L"Edit", L"0", WS_BORDER | WS_VISIBLE | WS_CHILD | ES_CENTER | ES_READONLY, 192, 324, \
        90, 23, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);

    HANDLE hDirectionL = CreateWindow(L"Static", L"Направление", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_SUNKEN, 175, 352, \
        125, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hDirectionEB = CreateWindow(L"Edit", NULL, WS_BORDER | WS_VISIBLE | WS_CHILD | ES_CENTER | ES_READONLY, 192, 377, \
        90, 23, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    
    hStartOneB = CreateWindow(L"Button", L"Запустить", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 310, 140, \
        110, 30, hWnd, (HMENU)StartOneButton_id, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hStartAllB = CreateWindow(L"Button", L"Запустить все", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 310, 185, \
        110, 30, hWnd, (HMENU)StartAllButton_id, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hStopOneB = CreateWindow(L"Button", L"Остановить", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 310, 230, \
        110, 30, hWnd, (HMENU)StopOneButton_id, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hStopAllB = CreateWindow(L"Button", L"Остановить все", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 310, 275, \
        110, 30, hWnd, (HMENU)StopAllButton_id, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hAddB = CreateWindow(L"Button", L"Добавить", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 310, 320, \
        110, 30, hWnd, (HMENU)AddButton_id, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hRemoveB = CreateWindow(L"Button", L"Убрать", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON, 310, 365, \
        110, 30, hWnd, (HMENU)RemoveButton_id, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);

    HANDLE hClstLeftL = CreateWindow(L"Static", L"Ближний слева", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_SUNKEN, 430, 140, \
        125, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hClstLeftEB = CreateWindow(L"Edit", NULL, WS_BORDER | WS_VISIBLE | WS_CHILD | ES_CENTER | ES_READONLY, 447, 165, \
        90, 23, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    HANDLE hArriveTimeLeftL = CreateWindow(L"Static", L"До прибытия ->", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_SUNKEN, 430, 193, \
        125, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hArriveTimeLeftEB = CreateWindow(L"Edit", L"0", WS_BORDER | WS_VISIBLE | WS_CHILD | ES_CENTER | ES_READONLY, 447, 218, \
        90, 23, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    HANDLE hClstRightL = CreateWindow(L"Static", L"Ближний справа", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_SUNKEN, 430, 246, \
        125, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hClstRightEB = CreateWindow(L"Edit", NULL, WS_BORDER | WS_VISIBLE | WS_CHILD | ES_CENTER | ES_READONLY, 447, 271, \
        90, 23, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    HANDLE hArriveTimeRightL = CreateWindow(L"Static", L"До прибытия <-", WS_VISIBLE | WS_CHILD | SS_CENTER | SS_SUNKEN, 430, 299, \
        125, 20, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
    hArriveTimeRightEB = CreateWindow(L"Edit", L"0", WS_BORDER | WS_VISIBLE | WS_CHILD | ES_CENTER | ES_READONLY, 447, 324, \
        90, 23, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);

    hStationLB = CreateWindow(L"ListBox", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | LBS_STANDARD ^ LBS_SORT, 565, 140, \
        150, 220, hWnd, NULL, (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE), NULL);
}

DWORD WINAPI DrawFrame(LPVOID lpParam)
{
    HDC hdc = GetDC(hMainWindow);
    buffDC = CreateCompatibleDC(hdc);
    buffBMP = CreateCompatibleBitmap(hdc, drawRect.right - drawRect.left, drawRect.bottom - drawRect.top);
    HGDIOBJ oldBMP = SelectObject(buffDC, buffBMP);
    RECT buffRect;
    buffRect.left = 0;
    buffRect.right = drawRect.right - drawRect.left;
    buffRect.top = 0;
    buffRect.bottom = drawRect.bottom - drawRect.top;
    ReleaseDC(hMainWindow, hdc);

    while (true) 
    {
        SelectObject(buffDC, whiteBrush);
        FillRect(buffDC, &buffRect, whiteBrush);
        FrameRect(buffDC, &buffRect, blackBrush);

        int station_id = SendMessage(hStationLB, LB_GETCURSEL, 0, 0);
        drawing.DrawPath(buffDC, traffic, station_id);

        int bus_id = SendMessage(hBusLB, LB_GETCURSEL, 0, 0);
        bool checked = IsDlgButtonChecked(hMainWindow, SeeAllCB_id);    // show all buses
        if(checked)
            for (int i = 0; i < traffic.BusCount(); i++)
                if(i != bus_id)
                    drawing.DrawMark(buffDC, traffic, i, false);
        drawing.DrawMark(buffDC, traffic, bus_id, true);

        RedrawWindow(hMainWindow, &drawRect, NULL, RDW_INVALIDATE);
        if (WaitForSingleObject(hStopDrawingEvent, 50) == WAIT_OBJECT_0)
            break;
    }
    SelectObject(buffDC, oldBMP);
    DeleteObject(buffBMP);
    DeleteDC(buffDC);
    return 0;
}
