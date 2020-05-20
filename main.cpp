#include <windows.h>
#include <stdio.h>
#include "dxfunc.h"
#include "resource.h"

#define    XSIZE     900
#define    YSIZE     600

LRESULT CALLBACK WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK RenderWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK DialogProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void InitDialog (HWND hWnd, HINSTANCE hInst);


char szClassName[] = "Basic class";    //윈도우 클래스
char szRenderClassName[] = "Render class";
BOOL		g_bActive = TRUE;
HWND        g_hWnd  = NULL;
HWND        g_hRenderWnd = NULL;
HINSTANCE   g_hInst = NULL;
HWND        g_hDlg = NULL;

int gMouseX = 0;
int gMouseY = 0;

ATOM InitApp (HINSTANCE hInst)
{
	{
		WNDCLASSEX wc;
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style =  CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = RenderWndProc;  
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 0;
		wc.hInstance = hInst;
		wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
		wc.lpszMenuName = NULL;   
		wc.lpszClassName = (LPCSTR) szRenderClassName;
		wc.hIconSm  = NULL;
		RegisterClassEx( &wc );
	}

    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style =  CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;  
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInst;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    wc.lpszMenuName = "MAINMENU";    
    wc.lpszClassName = (LPCSTR) szClassName;
    wc.hIconSm  = NULL;
    return (RegisterClassEx(&wc));
}

BOOL InitInstance (HINSTANCE hInst, int nCmdShow)
{
    HWND hWnd;

    hWnd = CreateWindow(szClassName,
            "DX 테스트",
            WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
            CW_USEDEFAULT,    
            CW_USEDEFAULT,    
            SCREEN_SIZEX,    
            SCREEN_SIZEY,   
            NULL, 
            NULL, //메뉴 핸들, 클래스 메뉴를 사용할 때는 NULL
            hInst, 
            NULL);
    if (! hWnd)
        return FALSE;

	g_hWnd = hWnd;

    g_hRenderWnd = CreateWindow(szRenderClassName, 
			NULL, 
			WS_CHILD | WS_VISIBLE, 
			10,        
			10,        
			100,
			100,
			hWnd,
			NULL,    
			hInst,
			NULL);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    return TRUE;
}

int WINAPI WinMain (HINSTANCE hCurInst, HINSTANCE hPrevInst, LPSTR lpsCmdLine, int nCmdShow)
{
    MSG msg;
    
    g_hInst = hCurInst;
    if (! InitApp(hCurInst))
        return FALSE;

    if (! InitInstance(hCurInst, nCmdShow)) 
        return FALSE;

	if(InitD3D(g_hWnd)!= S_OK )
		return -1;
	SetupCamera(g_hWnd);

	if(!RenderInit())
		return -1;

	BOOL bGotMsg;

	msg.message = WM_NULL;
	PeekMessage( &msg, NULL, 0U, 0U, PM_NOREMOVE );
    while( WM_QUIT != msg.message  )
    {
        // Use PeekMessage() if the app is active, so we can use idle time to
        // render the scene. Else, use GetMessage() to avoid eating CPU time.
        if( g_bActive )
            bGotMsg = PeekMessage( &msg, NULL, 0U, 0U, PM_REMOVE );
        else
            bGotMsg = GetMessage( &msg, NULL, 0U, 0U );

        if( bGotMsg )
        {
            // Translate and dispatch the message
            //if( 0 == TranslateAccelerator( g_hWnd, hAccel, &msg ) )
            {
                TranslateMessage( &msg );
                DispatchMessage( &msg );
            }
        }
        else
        {
            // Render a frame during idle time (no messages are waiting)
            if( g_bActive )//&& m_bReady )
            {
				Update();
                if( FAILED( RenderLoop()))
                    SendMessage( g_hWnd, WM_CLOSE, 0, 0 );
            }
        }
    }

/*
    while (GetMessage(&msg, NULL, 0, 0))
	{
		if ((g_hDlg== 0) || ! IsDialogMessage(g_hDlg, &msg)) 
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
    }
*/

	UnregisterClass( szRenderClassName, hCurInst);
	UnregisterClass( szClassName, hCurInst);

    return 0;
}


LRESULT CALLBACK RenderWndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) 
	{
		case WM_MOUSEMOVE:
			{
				gMouseX = (int)LOWORD(lParam);
				gMouseY = (int)HIWORD(lParam);
			}
			break;

        case WM_CREATE:
			{
				RECT rc, dlgrc;
				GetWindowRect(g_hWnd, &rc);	
				int xSize = rc.right - rc.left - 17;
				int ySize = rc.bottom - rc.top - 63;

				GetWindowRect(g_hDlg, &dlgrc);
				MoveWindow(hWnd, 5, 5, xSize - (dlgrc.right - dlgrc.left), ySize, TRUE);			
			}
            break;
        default:
            return (DefWindowProc(hWnd, msg, wParam, lParam));
    }
    return 0;
}


LRESULT CALLBACK WndProc (HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) 
	{
		case WM_ACTIVATE:
			g_bActive = LOWORD(wParam);
			return 0;
        case WM_CREATE:
			InitDialog(hWnd, g_hInst);
            break;
        case WM_COMMAND:
            switch (LOWORD(wParam)) 
			{
                case IDM_END:
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    break;
                default:
					break;
            }
			break;
/*
		case WM_PAINT:
			{
				HDC    hdc;
				PAINTSTRUCT ps;
				hdc=BeginPaint(hWnd, &ps);
				SetTextColor(hdc, RGB(255, 50, 50));
				SetBkColor(hdc, RGB(50, 255, 50));
				TextOut(hdc, 360, 200, "WM_PAINT 출력입니다...", (int)strlen("WM_PAINT 출력입니다..."));
				EndPaint(hWnd, &ps);
			    return 0;
			}
*/
        case WM_CLOSE:
			DestroyDirect();
            DestroyWindow(hWnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
          
        default:
            return (DefWindowProc(hWnd, msg, wParam, lParam));
    }
    return 0;
}
//------------------------------------------------------------------------------------------
bool Do_WM_COMMAND (WPARAM wParam, LPARAM lParam)
{
/*
	int id;
	id = LOWORD(wParam);

    switch (id) 
	{
	case IDC_XXX:
		{
            return true;
		}
	}
*/
    return false;
}
//------------------------------------------------------------------------------------------
LRESULT CALLBACK DialogProc (HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) 
	{
    case WM_INITDIALOG:
		return TRUE;
    case WM_COMMAND:
		Do_WM_COMMAND(wParam, lParam);
		return TRUE;
	default:
		break;
	}
	return FALSE;
}
//------------------------------------------------------------------------------------------
void InitDialog (HWND hWnd, HINSTANCE hInst)
{
	RECT rc;
	int  xSize, ySize;
    
	if (g_hDlg)
		   return;
    g_hDlg = CreateDialog(hInst, "MAINDLG", hWnd, (DLGPROC) DialogProc);
    if (g_hDlg) 
	{
          ShowWindow(g_hDlg, SW_SHOW);
          GetWindowRect(g_hDlg, &rc);
          xSize = rc.right - rc.left + 1;
          ySize = rc.bottom - rc.top + 1;
          GetClientRect(hWnd, &rc);
          MoveWindow(g_hDlg, rc.right - xSize + 1, rc.top, xSize, rc.bottom, TRUE);	
    }

	SetDlgItemText(g_hDlg, IDC_XAXIS, "1");
	SetDlgItemText(g_hDlg, IDC_YAXIS, "0");
	SetDlgItemText(g_hDlg, IDC_ZAXIS, "0");
}
