
#include <windows.h>
#include "strsafe.h" // how to scroll text

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK scrollWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MyBitmapWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK MyTextWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR szCmdLine, int iCmdShow)
{
    static TCHAR szAppName[] = TEXT("scrollcalctest");
    HWND         hwnd;
    MSG          msg;
    WNDCLASS     wndclass;

    wndclass.style         = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc   = MyTextWindowProc; // MyBitmapWindowProc; // WndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = hInstance;
    wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName  = NULL;
    wndclass.lpszClassName = szAppName;

    if (!RegisterClass(&wndclass))
        return 0;

    // scroll class
    wndclass.lpfnWndProc   = scrollWndProc;
    wndclass.lpszClassName = TEXT("myscroll");
    wndclass.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
    if (!RegisterClass(&wndclass))
        return 0;


    hwnd = CreateWindow(szAppName, TEXT("测试滚动条"),
                        WS_OVERLAPPEDWINDOW | WS_VSCROLL,
                        200, 200,
                        600, 500,
                        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hwndScroll;
    static int  cxChar, cxCaps, cyChar, cxClient, cyClient;
    HDC         hdc;
    int         i, x, y, iVertPos, iPaintBeg, iPaintEnd;
    PAINTSTRUCT ps;
    SCROLLINFO  si;
    TCHAR       szBuffer[100];
    TEXTMETRIC  tm;
    static      int rowCount;

    switch (message)
    {
    case WM_CREATE:

        hdc = GetDC(hwnd);

        // 测试当前数据行数，用于测试滚动量
        rowCount = 100000;  

        GetTextMetrics(hdc, &tm);
        cxChar = tm.tmAveCharWidth;
        cxCaps = (tm.tmPitchAndFamily & 1 ? 3 : 2) * cxChar / 2;
        cyChar = tm.tmHeight + tm.tmExternalLeading;
        ReleaseDC(hwnd, hdc);

        hwndScroll = CreateWindow(TEXT("myscroll"), TEXT("myscroll"), WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                  350, 10, 20, 480,
                                  hwnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

       
        return 0;

    case WM_SIZE:
        cxClient = LOWORD(lParam);
        cyClient = HIWORD(lParam);

        MoveWindow(hwndScroll, 350, 10, 20, cyClient - 20, true);

        // 设置滚动条

        si.cbSize = sizeof(si);
        si.fMask  = SIF_RANGE | SIF_PAGE;
        si.nMin   = 0;
        si.nMax   = rowCount - 1 + cyClient / cyChar - 1;
        si.nPage  = cyClient / cyChar;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        SetScrollInfo(hwndScroll, SB_VERT, &si, TRUE);

        return 0;

    case WM_VSCROLL:
        // 获取竖滚动条状态
        si.cbSize = sizeof(si);
        si.fMask  = SIF_ALL;
        GetScrollInfo(hwnd, SB_VERT, &si);

        // 保存原来的位置，用于计算滚动当前画布量
        iVertPos = si.nPos;

        // 设置滚动量
        switch (LOWORD(wParam))
        {
        case SB_TOP:        si.nPos = si.nMin;      break;
        case SB_BOTTOM:     si.nPos = si.nMax;      break;
        case SB_LINEUP:     si.nPos -= 1;           break;
        case SB_LINEDOWN:   si.nPos += 1;           break;
        case SB_PAGEUP:     si.nPos -= si.nPage;    break;
        case SB_PAGEDOWN:   si.nPos += si.nPage;    break;
        case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
        default:    break;
        }

        // 更新系统滚动条状态
        si.fMask = SIF_POS;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        GetScrollInfo(hwnd, SB_VERT, &si);

        // 滚动界面画布
        if (si.nPos != iVertPos) {
            ScrollWindow(hwnd, 0, cyChar * (iVertPos - si.nPos), NULL, NULL);
            UpdateWindow(hwnd);
        }

        MoveWindow(hwndScroll, 350, 10, 20, cyClient - 20, true);
        PostMessage(hwndScroll, message, wParam, lParam);

        return 0;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);

        // 获取竖滚动条状态
        si.cbSize = sizeof(si);
        si.fMask  = SIF_ALL;
        GetScrollInfo(hwnd, SB_VERT, &si);
        iVertPos = si.nPos;

        // 重绘修改的位置（减少重绘量）
        iPaintBeg = max(0, iVertPos + ps.rcPaint.top / cyChar);
        iPaintEnd = min(rowCount - 1, iVertPos + ps.rcPaint.bottom / cyChar);

        SetTextAlign(hdc, TA_LEFT);
        for (i = iPaintBeg; i <= iPaintEnd; i++)
        {
            y = cyChar * (i - iVertPos);
            TextOut(hdc, 22 , y, szBuffer, wsprintf(szBuffer, TEXT("%5d"), i));
        }

        EndPaint(hwnd, &ps);

        wsprintf(szBuffer, TEXT("nPos:%5d    nTrackPos:%5d"), si.nPos, si.nTrackPos);
        SetWindowText(hwnd, szBuffer);


        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}


LRESULT CALLBACK scrollWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static SCROLLINFO  si;

    switch (message)
    {
    default:
        break;

    }

    return DefWindowProc(hwnd, message, wParam, lParam);
}



LRESULT CALLBACK MyBitmapWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // 
    // How to Scroll a Bitmap
    // https://msdn.microsoft.com/en-us/library/windows/desktop/hh298420(v=vs.85).aspx
    //

    HDC hdc;
    PAINTSTRUCT ps;
    SCROLLINFO si;

    // These variables are required by BitBlt. 
    static HDC hdcWin;           // window DC 
    static HDC hdcScreen;        // DC for entire screen 
    static HDC hdcScreenCompat;  // memory DC for screen 
    static HBITMAP hbmpCompat;   // bitmap handle to old DC 
    static BITMAP bmp;           // bitmap data structure 
    static BOOL fBlt;            // TRUE if BitBlt occurred 
    static BOOL fScroll;         // TRUE if scrolling occurred 
    static BOOL fSize;           // TRUE if fBlt & WM_SIZE 

                                 // These variables are required for horizontal scrolling. 
    static int xMinScroll;       // minimum horizontal scroll value 
    static int xCurrentScroll;   // current horizontal scroll value 
    static int xMaxScroll;       // maximum horizontal scroll value 

                                 // These variables are required for vertical scrolling. 
    static int yMinScroll;       // minimum vertical scroll value 
    static int yCurrentScroll;   // current vertical scroll value 
    static int yMaxScroll;       // maximum vertical scroll value 

    switch (uMsg)
    {
    case WM_CREATE:

        // Create a normal DC and a memory DC for the entire 
        // screen. The normal DC provides a snapshot of the 
        // screen contents. The memory DC keeps a copy of this 
        // snapshot in the associated bitmap. 
        hdcScreen = CreateDC(TEXT("DISPLAY"), (PCTSTR)NULL, (PCTSTR)NULL, (CONST DEVMODE *) NULL);
        hdcScreenCompat = CreateCompatibleDC(hdcScreen);

        // Retrieve the metrics for the bitmap associated with the 
        // regular device context. 
        bmp.bmBitsPixel =
            (BYTE)GetDeviceCaps(hdcScreen, BITSPIXEL);
        bmp.bmPlanes = (BYTE)GetDeviceCaps(hdcScreen, PLANES);
        bmp.bmWidth = GetDeviceCaps(hdcScreen, HORZRES);
        bmp.bmHeight = GetDeviceCaps(hdcScreen, VERTRES);

        // The width must be byte-aligned. 
        bmp.bmWidthBytes = ((bmp.bmWidth + 15) &~15) / 8;

        // Create a bitmap for the compatible DC. 
        hbmpCompat = CreateBitmap(bmp.bmWidth, bmp.bmHeight,
                                  bmp.bmPlanes, bmp.bmBitsPixel, (CONST VOID *) NULL);

        // Select the bitmap for the compatible DC. 
        SelectObject(hdcScreenCompat, hbmpCompat);

        // Initialize the flags. 
        fBlt = FALSE;
        fScroll = FALSE;
        fSize = FALSE;

        // Initialize the horizontal scrolling variables. 
        xMinScroll = 0;
        xCurrentScroll = 0;
        xMaxScroll = 0;

        // Initialize the vertical scrolling variables. 
        yMinScroll = 0;
        yCurrentScroll = 0;
        yMaxScroll = 0;

        break;

    case WM_SIZE:
    {
        int xNewSize;
        int yNewSize;

        xNewSize = LOWORD(lParam);
        yNewSize = HIWORD(lParam);

        if (fBlt)
            fSize = TRUE;

        // The horizontal scrolling range is defined by 
        // (bitmap_width) - (client_width). The current horizontal 
        // scroll value remains within the horizontal scrolling range. 
        xMaxScroll = max(bmp.bmWidth - xNewSize, 0);
        xCurrentScroll = min(xCurrentScroll, xMaxScroll);
        si.cbSize = sizeof(si);
        si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin   = xMinScroll;
        si.nMax   = bmp.bmWidth;
        si.nPage  = xNewSize;
        si.nPos   = xCurrentScroll;
        SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

        // The vertical scrolling range is defined by 
        // (bitmap_height) - (client_height). The current vertical 
        // scroll value remains within the vertical scrolling range. 
        yMaxScroll = max(bmp.bmHeight - yNewSize, 0);
        yCurrentScroll = min(yCurrentScroll, yMaxScroll);
        si.cbSize = sizeof(si);
        si.fMask  = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin   = yMinScroll;
        si.nMax   = bmp.bmHeight;
        si.nPage  = yNewSize;
        si.nPos   = yCurrentScroll;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

        break;
    }

    case WM_PAINT:
    {
        PRECT prect;

        hdc = BeginPaint(hwnd, &ps);

        // If the window has been resized and the user has 
        // captured the screen, use the following call to 
        // BitBlt to paint the window's client area. 
        if (fSize)
        {
            BitBlt(ps.hdc,
                   0, 0,
                   bmp.bmWidth, bmp.bmHeight,
                   hdcScreenCompat,
                   xCurrentScroll, yCurrentScroll,
                   SRCCOPY);

            fSize = FALSE;
        }

        // If scrolling has occurred, use the following call to 
        // BitBlt to paint the invalid rectangle. 
        // 
        // The coordinates of this rectangle are specified in the 
        // RECT structure to which prect points. 
        // 
        // Note that it is necessary to increment the seventh 
        // argument (prect->left) by xCurrentScroll and the 
        // eighth argument (prect->top) by yCurrentScroll in 
        // order to map the correct pixels from the source bitmap. 
        if (fScroll)
        {
            prect = &ps.rcPaint;

            BitBlt(ps.hdc,
                   prect->left, prect->top,
                   (prect->right - prect->left),
                   (prect->bottom - prect->top),
                   hdcScreenCompat,
                   prect->left + xCurrentScroll,
                   prect->top + yCurrentScroll,
                   SRCCOPY);

            fScroll = FALSE;
        }

        EndPaint(hwnd, &ps);

        break;
    }

    case WM_HSCROLL:
    {
        int xDelta;     // xDelta = new_pos - current_pos  
        int xNewPos;    // new position 
        int yDelta = 0;

        switch (LOWORD(wParam))
        {
            // User clicked the scroll bar shaft left of the scroll box. 
        case SB_PAGEUP:
            xNewPos = xCurrentScroll - 50;
            break;

            // User clicked the scroll bar shaft right of the scroll box. 
        case SB_PAGEDOWN:
            xNewPos = xCurrentScroll + 50;
            break;

            // User clicked the left arrow. 
        case SB_LINEUP:
            xNewPos = xCurrentScroll - 5;
            break;

            // User clicked the right arrow. 
        case SB_LINEDOWN:
            xNewPos = xCurrentScroll + 5;
            break;

            // User dragged the scroll box. 
        case SB_THUMBPOSITION:
            xNewPos = HIWORD(wParam);
            break;

        default:
            xNewPos = xCurrentScroll;
        }

        // New position must be between 0 and the screen width. 
        xNewPos = max(0, xNewPos);
        xNewPos = min(xMaxScroll, xNewPos);

        // If the current position does not change, do not scroll.
        if (xNewPos == xCurrentScroll)
            break;

        // Set the scroll flag to TRUE. 
        fScroll = TRUE;

        // Determine the amount scrolled (in pixels). 
        xDelta = xNewPos - xCurrentScroll;

        // Reset the current scroll position. 
        xCurrentScroll = xNewPos;

        // Scroll the window. (The system repaints most of the 
        // client area when ScrollWindowEx is called; however, it is 
        // necessary to call UpdateWindow in order to repaint the 
        // rectangle of pixels that were invalidated.) 
        ScrollWindowEx(hwnd, -xDelta, -yDelta, (CONST RECT *) NULL,
                       (CONST RECT *) NULL, (HRGN)NULL, (PRECT)NULL,
                       SW_INVALIDATE);
        UpdateWindow(hwnd);

        // Reset the scroll bar. 
        si.cbSize = sizeof(si);
        si.fMask  = SIF_POS;
        si.nPos   = xCurrentScroll;
        SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

        break;
    }

    case WM_VSCROLL:
    {
        int xDelta = 0;
        int yDelta;     // yDelta = new_pos - current_pos 
        int yNewPos;    // new position 

        switch (LOWORD(wParam))
        {
            // User clicked the scroll bar shaft above the scroll box. 
        case SB_PAGEUP:
            yNewPos = yCurrentScroll - 50;
            break;

            // User clicked the scroll bar shaft below the scroll box. 
        case SB_PAGEDOWN:
            yNewPos = yCurrentScroll + 50;
            break;

            // User clicked the top arrow. 
        case SB_LINEUP:
            yNewPos = yCurrentScroll - 5;
            break;

            // User clicked the bottom arrow. 
        case SB_LINEDOWN:
            yNewPos = yCurrentScroll + 5;
            break;

            // User dragged the scroll box. 
        case SB_THUMBPOSITION:
            yNewPos = HIWORD(wParam);
            break;

        default:
            yNewPos = yCurrentScroll;
        }

        // New position must be between 0 and the screen height. 
        yNewPos = max(0, yNewPos);
        yNewPos = min(yMaxScroll, yNewPos);

        // If the current position does not change, do not scroll.
        if (yNewPos == yCurrentScroll)
            break;

        // Set the scroll flag to TRUE. 
        fScroll = TRUE;

        // Determine the amount scrolled (in pixels). 
        yDelta = yNewPos - yCurrentScroll;

        // Reset the current scroll position. 
        yCurrentScroll = yNewPos;

        // Scroll the window. (The system repaints most of the 
        // client area when ScrollWindowEx is called; however, it is 
        // necessary to call UpdateWindow in order to repaint the 
        // rectangle of pixels that were invalidated.) 
        ScrollWindowEx(hwnd, -xDelta, -yDelta, (CONST RECT *) NULL,
                       (CONST RECT *) NULL, (HRGN)NULL, (PRECT)NULL,
                       SW_INVALIDATE);
        UpdateWindow(hwnd);

        // Reset the scroll bar. 
        si.cbSize = sizeof(si);
        si.fMask  = SIF_POS;
        si.nPos   = yCurrentScroll;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

        break;
    }

    case WM_RBUTTONDOWN:
    {
        // Get the compatible DC of the client area. 
        hdcWin = GetDC(hwnd);

        // Fill the client area to remove any existing contents. 
        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(hdcWin, &rect, (HBRUSH)(COLOR_WINDOW + 1));

        // Copy the contents of the current screen 
        // into the compatible DC. 
        BitBlt(hdcScreenCompat, 0, 0, bmp.bmWidth,
               bmp.bmHeight, hdcScreen, 0, 0, SRCCOPY);

        // Copy the compatible DC to the client area.
        BitBlt(hdcWin, 0, 0, bmp.bmWidth, bmp.bmHeight,
               hdcScreenCompat, 0, 0, SRCCOPY);

        ReleaseDC(hwnd, hdcWin);
        fBlt = TRUE;
        break;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}




LRESULT CALLBACK MyTextWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //
    // How to Scroll Text
    // https://msdn.microsoft.com/en-us/library/windows/desktop/hh298421(v=vs.85).aspx
    // 

    HDC hdc;
    PAINTSTRUCT ps;
    TEXTMETRIC tm;
    SCROLLINFO si;

    // These variables are required to display text. 
    static int xClient;     // width of client area 
    static int yClient;     // height of client area 
    static int xClientMax;  // maximum width of client area 

    static int xChar;       // horizontal scrolling unit 
    static int yChar;       // vertical scrolling unit 
    static int xUpper;      // average width of uppercase letters 

    static int xPos;        // current horizontal scrolling position 
    static int yPos;        // current vertical scrolling position 

    int i;                  // loop counter 
    int x, y;               // horizontal and vertical coordinates

    int FirstLine;          // first line in the invalidated area 
    int LastLine;           // last line in the invalidated area 
    HRESULT hr;
    size_t abcLength;        // length of an abc[] item 

                             // Create an array of lines to display. 
#define LINES 28 
    static TCHAR *abc[] ={
        TEXT("anteater"),  TEXT("bear"),      TEXT("cougar"),
        TEXT("dingo"),     TEXT("elephant"),  TEXT("falcon"),
        TEXT("gazelle"),   TEXT("hyena"),     TEXT("iguana"),
        TEXT("jackal"),    TEXT("kangaroo"),  TEXT("llama"),
        TEXT("moose"),     TEXT("newt"),      TEXT("octopus"),
        TEXT("penguin"),   TEXT("quail"),     TEXT("rat"),
        TEXT("squid"),     TEXT("tortoise"),  TEXT("urus"),
        TEXT("vole"),      TEXT("walrus"),    TEXT("xylophone"),
        TEXT("yak"),       TEXT("zebra"),
        TEXT("This line contains words, but no character. Go figure."),
        TEXT("")
    };

    switch (uMsg)
    {
    case WM_CREATE:
        // Get the handle to the client area's device context. 
        hdc = GetDC(hwnd);

        // Extract font dimensions from the text metrics. 
        GetTextMetrics(hdc, &tm);
        xChar = tm.tmAveCharWidth;
        xUpper = (tm.tmPitchAndFamily & 1 ? 3 : 2) * xChar / 2;
        yChar = tm.tmHeight + tm.tmExternalLeading;

        // Free the device context. 
        ReleaseDC(hwnd, hdc);

        // Set an arbitrary maximum width for client area. 
        // (xClientMax is the sum of the widths of 48 average 
        // lowercase letters and 12 uppercase letters.) 
        xClientMax = 48 * xChar + 12 * xUpper;

        return 0;

    case WM_SIZE:

        // Retrieve the dimensions of the client area. 
        yClient = HIWORD(lParam);
        xClient = LOWORD(lParam);

        // Set the vertical scrolling range and page size
        si.cbSize = sizeof(si);
        si.fMask  = SIF_RANGE | SIF_PAGE;
        si.nMin   = 0;
        si.nMax   = LINES - 1;
        si.nPage  = yClient / yChar;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);

        // Set the horizontal scrolling range and page size. 
        si.cbSize = sizeof(si);
        si.fMask  = SIF_RANGE | SIF_PAGE;
        si.nMin   = 0;
        si.nMax   = 2 + xClientMax / xChar;
        si.nPage  = xClient / xChar;
        SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);

        return 0;
    case WM_HSCROLL:
        // Get all the vertial scroll bar information.
        si.cbSize = sizeof(si);
        si.fMask  = SIF_ALL;

        // Save the position for comparison later on.
        GetScrollInfo(hwnd, SB_HORZ, &si);
        xPos = si.nPos;
        switch (LOWORD(wParam))
        {
            // User clicked the left arrow.
        case SB_LINELEFT:
            si.nPos -= 1;
            break;

            // User clicked the right arrow.
        case SB_LINERIGHT:
            si.nPos += 1;
            break;

            // User clicked the scroll bar shaft left of the scroll box.
        case SB_PAGELEFT:
            si.nPos -= si.nPage;
            break;

            // User clicked the scroll bar shaft right of the scroll box.
        case SB_PAGERIGHT:
            si.nPos += si.nPage;
            break;

            // User dragged the scroll box.
        case SB_THUMBTRACK:
            si.nPos = si.nTrackPos;
            break;

        default:
            break;
        }

        // Set the position and then retrieve it.  Due to adjustments
        // by Windows it may not be the same as the value set.
        si.fMask = SIF_POS;
        SetScrollInfo(hwnd, SB_HORZ, &si, TRUE);
        GetScrollInfo(hwnd, SB_HORZ, &si);

        // If the position has changed, scroll the window.
        if (si.nPos != xPos)
        {
            ScrollWindow(hwnd, xChar * (xPos - si.nPos), 0, NULL, NULL);
        }

        return 0;

    case WM_VSCROLL:
        // Get all the vertial scroll bar information.
        si.cbSize = sizeof(si);
        si.fMask  = SIF_ALL;
        GetScrollInfo(hwnd, SB_VERT, &si);

        // Save the position for comparison later on.
        yPos = si.nPos;
        switch (LOWORD(wParam))
        {

            // User clicked the HOME keyboard key.
        case SB_TOP:
            si.nPos = si.nMin;
            break;

            // User clicked the END keyboard key.
        case SB_BOTTOM:
            si.nPos = si.nMax;
            break;

            // User clicked the top arrow.
        case SB_LINEUP:
            si.nPos -= 1;
            break;

            // User clicked the bottom arrow.
        case SB_LINEDOWN:
            si.nPos += 1;
            break;

            // User clicked the scroll bar shaft above the scroll box.
        case SB_PAGEUP:
            si.nPos -= si.nPage;
            break;

            // User clicked the scroll bar shaft below the scroll box.
        case SB_PAGEDOWN:
            si.nPos += si.nPage;
            break;

            // User dragged the scroll box.
        case SB_THUMBTRACK:
            si.nPos = si.nTrackPos;
            break;

        default:
            break;
        }

        // Set the position and then retrieve it.  Due to adjustments
        // by Windows it may not be the same as the value set.
        si.fMask = SIF_POS;
        SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        GetScrollInfo(hwnd, SB_VERT, &si);

        // If the position has changed, scroll window and update it.
        if (si.nPos != yPos)
        {
            ScrollWindow(hwnd, 0, yChar * (yPos - si.nPos), NULL, NULL);
            UpdateWindow(hwnd);
        }

        return 0;

    case WM_PAINT:
        // Prepare the window for painting.
        hdc = BeginPaint(hwnd, &ps);

        // Get vertical scroll bar position.
        si.cbSize = sizeof(si);
        si.fMask  = SIF_POS;
        GetScrollInfo(hwnd, SB_VERT, &si);
        yPos = si.nPos;

        // Get horizontal scroll bar position.
        GetScrollInfo(hwnd, SB_HORZ, &si);
        xPos = si.nPos;

        // Find painting limits.
        FirstLine = max(0, yPos + ps.rcPaint.top / yChar);
        LastLine = min(LINES - 1, yPos + ps.rcPaint.bottom / yChar);

        for (i = FirstLine; i <= LastLine; i++)
        {
            x = xChar * (1 - xPos);
            y = yChar * (i - yPos);

            // Note that "55" in the following depends on the 
            // maximum size of an abc[] item. Also, you must include
            // strsafe.h to use the StringCchLength function.
            hr = StringCchLength(abc[i], 55, &abcLength);
            if ((FAILED(hr)) | (abcLength == NULL))
            {
                //
                // TODO: write error handler
                //
            }

            // Write a line of text to the client area.
            TextOut(hdc, x, y, abc[i], abcLength);
        }

        // Indicate that painting is finished.
        EndPaint(hwnd, &ps);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}