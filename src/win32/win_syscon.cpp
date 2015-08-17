// win_syscon.h
// this include must remain at the top of every CPP file
#include "../client/client.h"
#include "win_local.h"



#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>

#define COPY_ID			1
#define QUIT_ID			2
#define CLEAR_ID		3

#define ERRORBOX_ID		10
#define ERRORTEXT_ID	11

#define EDIT_ID			100
#define INPUT_ID		101

typedef struct
{
	HWND		hWnd;
	HWND		hwndBuffer;

	HWND		hwndButtonClear;
	HWND		hwndButtonCopy;
	HWND		hwndButtonQuit;

	HWND		hwndErrorBox;
	HWND		hwndErrorText;

	HBITMAP		hbmLogo;
	HBITMAP		hbmClearBitmap;

	HBRUSH		hbrEditBackground;
	HBRUSH		hbrErrorBackground;

	HFONT		hfBufferFont;
	HFONT		hfButtonFont;

	HWND		hwndInputLine;

	char		errorString[80];

	char		consoleText[512], returnedText[512];
	int			visLevel;
	qboolean	quitOnClose;
	int			windowWidth, windowHeight;

	WNDPROC		SysInputLineWndProc;

} WinConData;

static WinConData s_wcd;

static LRESULT WINAPI ConWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const char *cmdString;
	static qboolean s_timePolarity;

	switch (uMsg)
	{
	case WM_ACTIVATE:
		if ( LOWORD( wParam ) != WA_INACTIVE )
		{
			SetFocus( s_wcd.hwndInputLine );
		}

		if ( com_viewlog && ( com_dedicated && !com_dedicated->integer ) )
		{
			// if the viewlog is open, check to see if it's being minimized
			if ( com_viewlog->integer == 1 )
			{
				if ( HIWORD( wParam ) )		// minimized flag
				{
					Cvar_Set( "viewlog", "2" );
				}
			}
			else if ( com_viewlog->integer == 2 )
			{
				if ( !HIWORD( wParam ) )		// minimized flag
				{
					Cvar_Set( "viewlog", "1" );
				}
			}
		}
		break;

	case WM_CLOSE:
		if ( ( com_dedicated && com_dedicated->integer ) )
		{
			cmdString = CopyString( "quit" );
			Sys_QueEvent(0, SE_CONSOLE, 0, 0, (int)strlen(cmdString) + 1, (void *)cmdString);
		}
		else if ( s_wcd.quitOnClose )
		{
			PostQuitMessage( 0 );
		}
		else
		{
			Sys_ShowConsole( 0, qfalse );
			Cvar_Set( "viewlog", "0" );
		}
		return 0;
	case WM_CTLCOLORSTATIC:
		if ( ( HWND ) lParam == s_wcd.hwndBuffer )
		{
			SetBkColor( ( HDC ) wParam, RGB( 0, 0, 0 ) );
			SetTextColor( ( HDC ) wParam, RGB( 249, 249, 000 ) );
			return (LRESULT)s_wcd.hbrEditBackground;
		}
		else if ( ( HWND ) lParam == s_wcd.hwndErrorBox )
		{
			if ( s_timePolarity & 1 )
			{
				SetBkColor(   ( HDC ) wParam, RGB( 0x80, 0x80, 0x80 ) );
				SetTextColor( ( HDC ) wParam, RGB( 0xff, 0x00, 0x00 ) );
			}
			else
			{
				SetBkColor(   ( HDC ) wParam, RGB( 0x80, 0x80, 0x80 ) );
				SetTextColor( ( HDC ) wParam, RGB( 0x00, 0x00, 0x00 ) );
			}
			return (LRESULT)s_wcd.hbrErrorBackground;
		}
		return FALSE;
		break;

	case WM_COMMAND:
		if ( wParam == COPY_ID )
		{
			SendMessageA( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
			SendMessageA( s_wcd.hwndBuffer, WM_COPY, 0, 0 );
		}
		else if ( wParam == QUIT_ID )
		{
			if ( s_wcd.quitOnClose )
			{
				PostQuitMessage( 0 );
			}
			else
			{
				cmdString = CopyString( "quit" );
				Sys_QueEvent(0, SE_CONSOLE, 0, 0, (int)strlen(cmdString) + 1, (void *)cmdString);
			}
		}
		else if ( wParam == CLEAR_ID )
		{
			SendMessageA( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
			SendMessageA( s_wcd.hwndBuffer, EM_REPLACESEL, FALSE, ( LPARAM ) "" );
			UpdateWindow( s_wcd.hwndBuffer );
		}
		break;
	case WM_CREATE:
		s_wcd.hbrEditBackground =  CreateSolidBrush( RGB( 0x00, 0x00, 0x00 ) );
		s_wcd.hbrErrorBackground = CreateSolidBrush( RGB( 0x80, 0x80, 0x80 ) );
		SetTimer( hWnd, 1, 1000, NULL );
		break;
	case WM_ERASEBKGND:
	    return DefWindowProcA( hWnd, uMsg, wParam, lParam );
	case WM_TIMER:
		if ( wParam == 1 )
		{
			s_timePolarity = (qboolean)!s_timePolarity;
			if ( s_wcd.hwndErrorBox )
			{
				InvalidateRect( s_wcd.hwndErrorBox, NULL, FALSE );
			}
		}
		break;
    }

    return DefWindowProcA( hWnd, uMsg, wParam, lParam );
}

//extern	void CompleteCommand( void ) ;

LRESULT APIENTRY InputLineWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char inputBuffer[1024];

	switch ( uMsg )
	{
		case WM_KILLFOCUS:
			if ( ( HWND ) wParam == s_wcd.hWnd ||
				 ( HWND ) wParam == s_wcd.hwndErrorBox )
			{
				SetFocus( hWnd );
				return 0;
			}
			break;

		case WM_CHAR:
			if ( wParam == 13 )
			{
				GetWindowTextA( s_wcd.hwndInputLine, inputBuffer, sizeof( inputBuffer ) );
				strncat( s_wcd.consoleText, inputBuffer, sizeof( s_wcd.consoleText ) - strlen( s_wcd.consoleText ) - 5 );
				strcat( s_wcd.consoleText, "\n" );
				SetWindowTextA( s_wcd.hwndInputLine, "" );

				Sys_Print( va( "]%s\n", inputBuffer ) );

				strcpy(kg.g_consoleField.buffer, inputBuffer);
				kg.historyEditLines[kg.nextHistoryLine % COMMAND_HISTORY] = kg.g_consoleField;
				kg.nextHistoryLine++;
				kg.historyLine = kg.nextHistoryLine;

				return 0;
			}
			else if (wParam == 9 )
			{
				GetWindowTextA( s_wcd.hwndInputLine, inputBuffer, sizeof( inputBuffer ) );
				strcpy(kg.g_consoleField.buffer, inputBuffer);
//				CompleteCommand();
				Field_AutoComplete( &kg.g_consoleField ); // Daggolin: for auto-complete (copied from OpenJK)
				SetWindowTextA( s_wcd.hwndInputLine, kg.g_consoleField.buffer);
				SendMessage(s_wcd.hwndInputLine, EM_SETSEL, strlen(kg.g_consoleField.buffer) , MAKELONG(0xffff, 0xffff) );
			}

		case WM_KEYDOWN:
			if (wParam == VK_UP)
			{
				if ( kg.nextHistoryLine - kg.historyLine < COMMAND_HISTORY && kg.historyLine > 0 )
				{
					kg.historyLine--;
				}
				kg.g_consoleField = kg.historyEditLines[ kg.historyLine % COMMAND_HISTORY ];
				SetWindowTextA( s_wcd.hwndInputLine, kg.g_consoleField.buffer);
				SendMessageA(s_wcd.hwndInputLine, EM_SETSEL, strlen(kg.g_consoleField.buffer) , MAKELONG(0xffff, 0xffff) );
				return 0;
			}
			else if (wParam == VK_DOWN)
			{
				if (kg.historyLine == kg.nextHistoryLine)
				{
					return 0;
				}
				kg.historyLine++;
				kg.g_consoleField = kg.historyEditLines[ kg.historyLine % COMMAND_HISTORY ];
				SetWindowTextA( s_wcd.hwndInputLine, kg.g_consoleField.buffer);
				SendMessageA(s_wcd.hwndInputLine, EM_SETSEL, strlen(kg.g_consoleField.buffer) , MAKELONG(0xffff, 0xffff) );
				return 0;
			}
			break;
	}

	return CallWindowProc( s_wcd.SysInputLineWndProc, hWnd, uMsg, wParam, lParam );
}

/*
** Sys_CreateConsole
*/
void Sys_CreateConsole( void )
{
	HDC hDC;
	WNDCLASSA wc;
	RECT rect;
	const char *DEDCLASS = "JK2MP WinConsole";
	int nHeight;
	int swidth, sheight;
	int DEDSTYLE = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX;

	memset( &wc, 0, sizeof( wc ) );

	wc.style         = 0;
	wc.lpfnWndProc   = (WNDPROC) ConWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = g_wv.hInstance;
	wc.hIcon         = LoadIcon( g_wv.hInstance, MAKEINTRESOURCE(IDI_ICON1));
	wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
	wc.hbrBackground = (HBRUSH__ *)COLOR_INACTIVEBORDER;
	wc.lpszMenuName  = 0;
	wc.lpszClassName = DEDCLASS;

	if ( !RegisterClassA(&wc) )	{
		return;
	}

	rect.left = 0;
	rect.right = 600;
	rect.top = 0;
	rect.bottom = 450;
	AdjustWindowRect( &rect, DEDSTYLE, FALSE );

	hDC = GetDC( GetDesktopWindow() );
	swidth = GetDeviceCaps( hDC, HORZRES );
	sheight = GetDeviceCaps( hDC, VERTRES );
	ReleaseDC( GetDesktopWindow(), hDC );

	s_wcd.windowWidth = rect.right - rect.left + 1;
	s_wcd.windowHeight = rect.bottom - rect.top + 1;

	s_wcd.hWnd = CreateWindowExA( 0,
							   DEDCLASS,
							   "JK2MV Console",
							   DEDSTYLE,
							   ( swidth - 600 ) / 2, ( sheight - 450 ) / 2 , rect.right - rect.left + 1, rect.bottom - rect.top + 1,
							   NULL,
							   NULL,
							   g_wv.hInstance,
							   NULL );

	if ( s_wcd.hWnd == NULL )
	{
		return;
	}

	//
	// create fonts
	//
	hDC = GetDC( s_wcd.hWnd );
	nHeight = -MulDiv( 8, GetDeviceCaps( hDC, LOGPIXELSY), 72);

	s_wcd.hfBufferFont = CreateFontA( nHeight,
									  0,
									  0,
									  0,
									  FW_LIGHT,
									  0,
									  0,
									  0,
									  DEFAULT_CHARSET,
									  OUT_DEFAULT_PRECIS,
									  CLIP_DEFAULT_PRECIS,
									  DEFAULT_QUALITY,
									  FF_MODERN | FIXED_PITCH,
									  "Courier New" );

	ReleaseDC( s_wcd.hWnd, hDC );

	//
	// create the input line
	//
	s_wcd.hwndInputLine = CreateWindowA( "edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
												ES_LEFT | ES_AUTOHSCROLL | WS_TABSTOP,
												6, 400, s_wcd.windowWidth-20, 20,
												s_wcd.hWnd,
												( HMENU ) INPUT_ID,	// child window ID
												g_wv.hInstance, NULL );

	//
	// create the buttons
	//
	s_wcd.hwndButtonCopy = CreateWindowA( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP,
												5, 425, 72, 24,
												s_wcd.hWnd,
												( HMENU ) COPY_ID,	// child window ID
												g_wv.hInstance, NULL );
	SendMessageA( s_wcd.hwndButtonCopy, WM_SETTEXT, 0, ( LPARAM ) "Copy" );

	s_wcd.hwndButtonClear = CreateWindowA( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP,
												82, 425, 72, 24,
												s_wcd.hWnd,
												( HMENU ) CLEAR_ID,	// child window ID
												g_wv.hInstance, NULL );
	SendMessageA( s_wcd.hwndButtonClear, WM_SETTEXT, 0, ( LPARAM ) "Clear" );

	s_wcd.hwndButtonQuit = CreateWindowA( "button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP,
												s_wcd.windowWidth-92, 425, 72, 24,
												s_wcd.hWnd,
												( HMENU ) QUIT_ID,	// child window ID
												g_wv.hInstance, NULL );
	SendMessageA( s_wcd.hwndButtonQuit, WM_SETTEXT, 0, ( LPARAM ) "Quit" );


	//
	// create the scrollbuffer
	//
	s_wcd.hwndBuffer = CreateWindowA( "edit", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
												ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_TABSTOP,
												6, 40, s_wcd.windowWidth-20, 354,
												s_wcd.hWnd,
												( HMENU ) EDIT_ID,	// child window ID
												g_wv.hInstance, NULL );
	SendMessageA( s_wcd.hwndBuffer, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );

	s_wcd.SysInputLineWndProc = (WNDPROC)SetWindowLongPtr(s_wcd.hwndInputLine, GWLP_WNDPROC, (LONG_PTR)InputLineWndProc);
	SendMessageA( s_wcd.hwndInputLine, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );
	SendMessageA( s_wcd.hwndBuffer, EM_LIMITTEXT, ( WPARAM ) 0x7fff, 0 );

	ShowWindow( s_wcd.hWnd, SW_SHOWDEFAULT);
	UpdateWindow( s_wcd.hWnd );
	SetForegroundWindow( s_wcd.hWnd );
	SetFocus( s_wcd.hwndInputLine );

	s_wcd.visLevel = 1;
}

/*
** Sys_DestroyConsole
*/
void Sys_DestroyConsole( void )
{
	if ( s_wcd.hWnd )
	{
		DeleteObject(s_wcd.hbrEditBackground);
		DeleteObject(s_wcd.hbrErrorBackground);
		DeleteObject(s_wcd.hfBufferFont);

		ShowWindow( s_wcd.hWnd, SW_HIDE );
		CloseWindow( s_wcd.hWnd );
		DestroyWindow( s_wcd.hWnd );
		s_wcd.hWnd = 0;
	}
}

/*
** Sys_ShowConsole
*/
void Sys_ShowConsole( int visLevel, qboolean quitOnClose )
{
	s_wcd.quitOnClose = quitOnClose;

	if ( visLevel == s_wcd.visLevel )
	{
		return;
	}

	s_wcd.visLevel = visLevel;

	if ( !s_wcd.hWnd )
	{
		return;
	}

	switch ( visLevel )
	{
	case 0:
		ShowWindow( s_wcd.hWnd, SW_HIDE );
		break;
	case 1:
		ShowWindow( s_wcd.hWnd, SW_SHOWNORMAL );
		SendMessageA( s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff );
		break;
	case 2:
		ShowWindow( s_wcd.hWnd, SW_MINIMIZE );
		break;
	default:
		Sys_Error( "Invalid visLevel %d sent to Sys_ShowConsole\n", visLevel );
		break;
	}
}

/*
** Sys_ConsoleInput
*/
char *Sys_ConsoleInput( void )
{
	if ( s_wcd.consoleText[0] == 0 )
	{
		return NULL;
	}

	strcpy( s_wcd.returnedText, s_wcd.consoleText );
	s_wcd.consoleText[0] = 0;

	return s_wcd.returnedText;
}

/*
** Conbuf_AppendText
*/
void Conbuf_AppendText( const char *pMsg )
{
#define CONSOLE_BUFFER_SIZE		16384*2 // ouned: increased
	if ( !s_wcd.hWnd ) {
		return;
	}
	char buffer[CONSOLE_BUFFER_SIZE*4];
	char *b = buffer;
	const char *msg;
	int bufLen;
	int i = 0;
	static unsigned long s_totalChars;

	//
	// if the message is REALLY long, use just the last portion of it
	//
	if ( strlen( pMsg ) > CONSOLE_BUFFER_SIZE - 1 )
	{
		msg = pMsg + strlen( pMsg ) - CONSOLE_BUFFER_SIZE + 1;
	}
	else
	{
		msg = pMsg;
	}

	//
	// copy into an intermediate buffer
	//
	while ( msg[i] && ( ( b - buffer ) < sizeof( buffer ) - 1 ) )
	{
		if ( msg[i] == '\n' && msg[i+1] == '\r' )
		{
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
			i++;
		}
		else if ( msg[i] == '\r' )
		{
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
		}
		else if ( msg[i] == '\n' )
		{
			b[0] = '\r';
			b[1] = '\n';
			b += 2;
		}
		else if ( Q_IsColorString( &msg[i] ) || (use102col && Q_IsColorString_1_02( &msg[i] )) ) 
		{
			i++;
		}
		else
		{
			*b= msg[i];
			b++;
		}
		i++;
	}
	*b = 0;
	bufLen = b - buffer;

	s_totalChars += bufLen;

	//
	// replace selection instead of appending if we're overflowing
	//
	if ( s_totalChars > 0x7fff )
	{
		SendMessageA( s_wcd.hwndBuffer, EM_SETSEL, 0, -1 );
		s_totalChars = bufLen;
	}

	//
	// put this text into the windows console
	//
	SendMessageA( s_wcd.hwndBuffer, EM_LINESCROLL, 0, 0xffff );
	SendMessageA( s_wcd.hwndBuffer, EM_SCROLLCARET, 0, 0 );
	SendMessageA( s_wcd.hwndBuffer, EM_REPLACESEL, 0, (LPARAM) buffer );
}

/*
** Sys_SetErrorText
*/
void Sys_SetErrorText( const char *buf )
{
	Q_strncpyz( s_wcd.errorString, buf, sizeof( s_wcd.errorString ) );

	if ( !s_wcd.hwndErrorBox )
	{
		s_wcd.hwndErrorBox = CreateWindowA( "static", NULL, WS_CHILD | WS_VISIBLE | SS_SUNKEN,
													6, 5, s_wcd.windowWidth-20, 30,
													s_wcd.hWnd,
													( HMENU ) ERRORBOX_ID,	// child window ID
													g_wv.hInstance, NULL );
		SendMessageA( s_wcd.hwndErrorBox, WM_SETFONT, ( WPARAM ) s_wcd.hfBufferFont, 0 );
		SetWindowTextA( s_wcd.hwndErrorBox, s_wcd.errorString );

		DestroyWindow( s_wcd.hwndInputLine );
		s_wcd.hwndInputLine = NULL;
	}
}
