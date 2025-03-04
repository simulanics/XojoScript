// GUIPlugin.cpp
// Cross-platform XGUI Plugin for Windows and Linux/macOS (Windows API / GTK)
// Build (Windows):
//   windres -O coff -i XGUI.rc -o XGUI.res
//   g++ -shared -o XGUI.dll XGUI.cpp XGUI.res -mwindows -Wl,--subsystem,windows -luxtheme -lcomctl32 -ldwmapi
// Build (macOS/Linux):
//   g++ -shared -fPIC -o XGUI.so XGUI.cpp $(pkg-config --cflags --libs gtk+-3.0)

#define UNICODE
#define _UNICODE

#include <map>
#include <string>
#include <functional>
#include <vector>
#include <sstream>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#include <uxtheme.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "user32.lib")
#define XPLUGIN_API __declspec(dllexport)

// If not already defined, define the wide versions of the class names:
#ifndef WC_COMBOBOXW
#define WC_COMBOBOXW L"ComboBox"
#endif
#ifndef WC_LISTBOXW
#define WC_LISTBOXW  L"ListBox"
#endif

// Define CB_SETMINVISIBLE if not defined (value from Windows 10 SDK)
#ifndef CB_SETMINVISIBLE
#define CB_SETMINVISIBLE 0x1702
#endif

// Global dark-mode colors/brush.
static HBRUSH  gDarkBrush     = NULL;
static COLORREF gDarkBkgColor  = RGB(32, 32, 32);
static COLORREF gDarkTextColor = RGB(255, 255, 255);

// Use CP_UTF8 to match the interpreter's std::string data (which is in UTF-8).
// This ensures that text passed from the interpreter is converted properly.
std::wstring toWString(const char* s) {
    if (!s) return L"";
    // Convert from UTF-8 to wide string:
    int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    std::wstring ws(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s, -1, &ws[0], len);
    if (!ws.empty() && ws.back() == L'\0')
        ws.pop_back();
    return ws;
}

// Convert wide string back to UTF-8.
std::string toString(const std::wstring& ws) {
    if(ws.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, NULL, 0, NULL, NULL);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), -1, &s[0], len, NULL, NULL);
    if (!s.empty() && s.back() == '\0')
        s.pop_back();
    return s;
}

// Helper: split wide string by a delimiter.
std::vector<std::wstring> split(const std::wstring &s, wchar_t delim) {
    std::vector<std::wstring> elems;
    std::wstringstream ss(s);
    std::wstring item;
    while (std::getline(ss, item, delim))
        elems.push_back(item);
    return elems;
}

// Helper: join vector of wide strings with a delimiter.
std::wstring join(const std::vector<std::wstring>& elems, wchar_t delim) {
    std::wstring result;
    for (size_t i = 0; i < elems.size(); i++) {
        result += elems[i];
        if (i + 1 < elems.size())
            result += delim;
    }
    return result;
}

// Subclass procedure for the main combo control (dropdown list area) to paint border & background in dark mode.
LRESULT CALLBACK ComboSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
    switch(msg)
    {
        case WM_NCPAINT:
        {
            HDC hdc = GetWindowDC(hwnd);
            if(hdc)
            {
                RECT rc;
                GetWindowRect(hwnd, &rc);
                OffsetRect(&rc, -rc.left, -rc.top);
                HBRUSH brush = CreateSolidBrush(gDarkBkgColor);
                FillRect(hdc, &rc, brush);
                DeleteObject(brush);
                FrameRect(hdc, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));
                ReleaseDC(hwnd, hdc);
            }
            return 0;
        }
        default:
            break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

// Subclass procedure for the static (selected) portion of combo boxes, so the displayed text is also drawn in dark mode.
LRESULT CALLBACK ComboStaticProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR)
{
    switch(msg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            // Fill background with dark color:
            FillRect(hdc, &rc, gDarkBrush);

            // Retrieve the text from the static portion and draw it in white:
            wchar_t text[256] = {0};
            GetWindowTextW(hwnd, text, 256);
            SetTextColor(hdc, gDarkTextColor);
            SetBkMode(hdc, TRANSPARENT);
            DrawTextW(hdc, text, -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
            EndPaint(hwnd, &ps);
            return 0;
        }
        default:
            break;
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

#else
#include <gtk/gtk.h>
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

// Define UIObject struct.
struct UIObject {
#ifdef _WIN32
    HWND hwnd;
#else
    GtkWidget* widget;
#endif
    std::function<void()> eventHandler;
};

// Global map for controls.
std::map<std::string, UIObject> uiObjects;

#ifdef _WIN32
// Apply dark mode attribute (Windows 10+).
void ApplyDarkMode(HWND hwnd) {
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
}

// Main window procedure.
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            if (!gDarkBrush)
                gDarkBrush = CreateSolidBrush(gDarkBkgColor);
            break;
        case WM_ERASEBKGND: {
            HDC hdc = (HDC)wParam;
            RECT rc;
            GetClientRect(hwnd, &rc);
            FillRect(hdc, &rc, gDarkBrush);
            return 1;
        }
        case WM_SIZE:
        case WM_SIZING:
        case WM_MOVE:
        case WM_WINDOWPOSCHANGED:
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            break;
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
            // Owner-drawn button
            if (dis->CtlType == ODT_BUTTON) {
                COLORREF btnColor = (dis->itemState & ODS_SELECTED) ? RGB(96,96,96) : RGB(64,64,64);
                HBRUSH buttonBrush = CreateSolidBrush(btnColor);
                FillRect(dis->hDC, &dis->rcItem, buttonBrush);
                FrameRect(dis->hDC, &dis->rcItem, (HBRUSH)GetStockObject(BLACK_BRUSH));
                DeleteObject(buttonBrush);
                SetTextColor(dis->hDC, gDarkTextColor);
                SetBkColor(dis->hDC, btnColor);
                SetBkMode(dis->hDC, TRANSPARENT);
                wchar_t text[256] = {0};
                GetWindowTextW(dis->hwndItem, text, 256);
                SIZE sz;
                GetTextExtentPoint32W(dis->hDC, text, (int)wcslen(text), &sz);
                int x = (dis->rcItem.right - dis->rcItem.left - sz.cx) / 2;
                int y = (dis->rcItem.bottom - dis->rcItem.top - sz.cy) / 2;
                TextOutW(dis->hDC, dis->rcItem.left + x, dis->rcItem.top + y, text, (int)wcslen(text));
                if (dis->itemState & ODS_FOCUS)
                    DrawFocusRect(dis->hDC, &dis->rcItem);
                return TRUE;
            }
            // Owner-drawn listbox items
            else if (dis->CtlType == ODT_LISTBOX) {
                int index = dis->itemID;
                if (index == -1) break;
                wchar_t itemText[256] = {0};
                SendMessageW(dis->hwndItem, LB_GETTEXT, index, (LPARAM)itemText);
                COLORREF bgColor = (dis->itemState & ODS_SELECTED) ? RGB(0,0,0) : gDarkBkgColor;
                HBRUSH brush = CreateSolidBrush(bgColor);
                FillRect(dis->hDC, &dis->rcItem, brush);
                DeleteObject(brush);
                SetTextColor(dis->hDC, gDarkTextColor);
                SetBkMode(dis->hDC, TRANSPARENT);
                TextOutW(dis->hDC, dis->rcItem.left + 2, dis->rcItem.top, itemText, (int)wcslen(itemText));
                return TRUE;
            }
            // Owner-drawn combo box items
            else if (dis->CtlType == ODT_COMBOBOX) {
                int index = dis->itemID;
                if (index == -1) break;
                wchar_t itemText[256] = {0};
                SendMessageW(dis->hwndItem, CB_GETLBTEXT, index, (LPARAM)itemText);
                COLORREF bgColor = (dis->itemState & ODS_SELECTED) ? RGB(0,0,0) : gDarkBkgColor;
                HBRUSH brush = CreateSolidBrush(bgColor);
                FillRect(dis->hDC, &dis->rcItem, brush);
                DeleteObject(brush);
                SetTextColor(dis->hDC, gDarkTextColor);
                SetBkMode(dis->hDC, TRANSPARENT);
                TextOutW(dis->hDC, dis->rcItem.left + 2, dis->rcItem.top, itemText, (int)wcslen(itemText));
                if (dis->itemState & ODS_FOCUS)
                    DrawFocusRect(dis->hDC, &dis->rcItem);
                return TRUE;
            }
            break;
        }
        // Dark mode text for standard controls
        case WM_CTLCOLORLISTBOX:
        case WM_CTLCOLORBTN:
        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC:
        case WM_CTLCOLORDLG:
        case WM_CTLCOLORSCROLLBAR: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, gDarkTextColor);
            SetBkColor(hdc, gDarkBkgColor);
            SetBkMode(hdc, OPAQUE);
            return (LRESULT)gDarkBrush;
        }
        default:
            break;
    }
    // Dispatch custom event handlers if any exist
    for (auto& obj : uiObjects) {
        if (obj.second.hwnd == hwnd && obj.second.eventHandler)
            obj.second.eventHandler();
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
#else
// GTK on macOS/Linux
static gboolean on_configure_event(GtkWidget* widget, GdkEvent* event, gpointer user_data) {
    gtk_widget_queue_draw(widget);
    return FALSE;
}
#endif

// XCreateWindow: creates the main window.
XPLUGIN_API bool CreateWindowX(const char* objname, int left, int top, int width, int height,
                               bool hasMin, bool hasMax, bool hasClose) {
#ifdef _WIN32
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = L"XojoPluginWindow";
    RegisterClassW(&wc);

    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!hasMin)  style &= ~WS_MINIMIZEBOX;
    if (!hasMax)  style &= ~WS_MAXIMIZEBOX;
    if (!hasClose) style &= ~WS_SYSMENU;

    std::wstring wTitle = toWString(objname);
    HWND hwnd = CreateWindowExW(0, L"XojoPluginWindow", wTitle.c_str(), style,
                                left, top, width, height,
                                NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!hwnd) return false;
    ShowWindow(hwnd, SW_SHOW);
    ApplyDarkMode(hwnd);
    uiObjects[objname] = { hwnd, nullptr };
    return true;
#else
    gtk_init(NULL, NULL);
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), objname);
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "configure-event", G_CALLBACK(on_configure_event), NULL);
    gtk_widget_show_all(window);
    uiObjects[objname] = { window, nullptr };
    return true;
#endif
}

// XDestroyControl: destroys a control by name.
XPLUGIN_API void XDestroyControl(const char* objname) {
    if (uiObjects.count(objname)) {
#ifdef _WIN32
        DestroyWindow(uiObjects[objname].hwnd);
#else
        gtk_widget_destroy(uiObjects[objname].widget);
#endif
        uiObjects.erase(objname);
    }
}

// XAddButton: creates an owner-drawn button.
XPLUGIN_API bool XAddButton(const char* objname, const char* parentname,
                            int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    std::wstring wObj = toWString(objname);
    HWND button = CreateWindowW(L"BUTTON", wObj.c_str(),
                                WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                left, top, width, height,
                                parent, NULL, GetModuleHandle(NULL), NULL);
    if (!button) return false;
    ApplyDarkMode(button);
    uiObjects[objname] = { button, nullptr };
    return true;
#else
    GtkWidget* parent = uiObjects[parentname].widget;
    GtkWidget* button = gtk_button_new_with_label(objname);
    gtk_fixed_put(GTK_FIXED(parent), button, left, top);
    gtk_widget_set_size_request(button, width, height);
    gtk_widget_show(button);
    uiObjects[objname] = { button, nullptr };
    return true;
#endif
}

// XAddTextField: creates a single-line text field.
XPLUGIN_API bool XAddTextField(const char* objname, const char* parentname,
                               int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND edit = CreateWindowW(L"EDIT", L"",
                              WS_CHILD | WS_VISIBLE | WS_BORDER,
                              left, top, width, height,
                              parent, NULL, GetModuleHandle(NULL), NULL);
    if (!edit) return false;
    ApplyDarkMode(edit);
    uiObjects[objname] = { edit, nullptr };
    return true;
#else
    GtkWidget* parent = uiObjects[parentname].widget;
    GtkWidget* entry = gtk_entry_new();
    gtk_fixed_put(GTK_FIXED(parent), entry, left, top);
    gtk_widget_set_size_request(entry, width, height);
    gtk_widget_show(entry);
    uiObjects[objname] = { entry, nullptr };
    return true;
#endif
}

// XAddTextArea: creates a multi-line text area.
XPLUGIN_API bool XAddTextArea(const char* objname, const char* parentname,
                              int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND edit = CreateWindowW(L"EDIT", L"",
                              WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE,
                              left, top, width, height,
                              parent, NULL, GetModuleHandle(NULL), NULL);
    if (!edit) return false;
    ApplyDarkMode(edit);
    uiObjects[objname] = { edit, nullptr };
    return true;
#else
    GtkWidget* parent = uiObjects[parentname].widget;
    GtkWidget* textview = gtk_text_view_new();
    gtk_fixed_put(GTK_FIXED(parent), textview, left, top);
    gtk_widget_set_size_request(textview, width, height);
    gtk_widget_show(textview);
    uiObjects[objname] = { textview, nullptr };
    return true;
#endif
}

// XAddComboBox: creates an editable dropdown using owner-draw, in wide-character mode, then subclasses it for dark mode.
XPLUGIN_API bool XAddComboBox(const char* objname, const char* parentname,
                              int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    std::wstring wObj = toWString(objname);
    // Use WC_COMBOBOXW with CBS_DROPDOWN and owner-draw fixed:
    HWND combo = CreateWindowW(WC_COMBOBOXW, L"",
                               WS_CHILD | WS_VISIBLE | CBS_DROPDOWN | CBS_OWNERDRAWFIXED,
                               left, top, width, height,
                               parent, NULL, GetModuleHandle(NULL), NULL);
    if (!combo) return false;
    ApplyDarkMode(combo);
    // Subclass the main combo
    SetWindowSubclass(combo, ComboSubclassProc, 1, 0);
    // Also subclass the static portion so text is displayed in dark mode properly:
    COMBOBOXINFO cbi = {0};
    cbi.cbSize = sizeof(cbi);
    if(GetComboBoxInfo(combo, &cbi)) {
        if(cbi.hwndItem)
            SetWindowSubclass(cbi.hwndItem, ComboStaticProc, 1, 0);
    }
    uiObjects[objname] = { combo, nullptr };
    return true;
#else
    GtkWidget* parent = uiObjects[parentname].widget;
    GtkWidget* combo = gtk_combo_box_text_new();
    gtk_fixed_put(GTK_FIXED(parent), combo, left, top);
    gtk_widget_set_size_request(combo, width, height);
    gtk_widget_show(combo);
    uiObjects[objname] = { combo, nullptr };
    return true;
#endif
}

// XAddPopupBox: creates a read-only dropdown using owner-draw, in wide-character mode, then subclasses it for dark mode.
XPLUGIN_API bool XAddPopupBox(const char* objname, const char* parentname,
                              int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    std::wstring wObj = toWString(objname);
    // Use WC_COMBOBOXW with CBS_DROPDOWNLIST and owner-draw fixed:
    HWND combo = CreateWindowW(WC_COMBOBOXW, L"",
                               WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED,
                               left, top, width, height,
                               parent, NULL, GetModuleHandle(NULL), NULL);
    if (!combo) return false;
    ApplyDarkMode(combo);
    // Subclass the main combo
    SetWindowSubclass(combo, ComboSubclassProc, 1, 0);
    // Subclass the static portion
    COMBOBOXINFO cbi = {0};
    cbi.cbSize = sizeof(cbi);
    if(GetComboBoxInfo(combo, &cbi)) {
        if(cbi.hwndItem)
            SetWindowSubclass(cbi.hwndItem, ComboStaticProc, 1, 0);
    }
    uiObjects[objname] = { combo, nullptr };
    return true;
#else
    GtkWidget* parent = uiObjects[parentname].widget;
    GtkWidget* combo = gtk_combo_box_text_new();
    gtk_fixed_put(GTK_FIXED(parent), combo, left, top);
    gtk_widget_set_size_request(combo, width, height);
    gtk_widget_show(combo);
    uiObjects[objname] = { combo, nullptr };
    return true;
#endif
}

// XAddListBox: creates an owner-drawn listbox with border, in wide-character mode, then applies dark mode.
XPLUGIN_API bool XAddListBox(const char* objname, const char* parentname,
                             int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND listbox = CreateWindowW(WC_LISTBOXW, L"",
                                 WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_OWNERDRAWFIXED |
                                 LBS_HASSTRINGS | WS_BORDER,
                                 left, top, width, height,
                                 parent, NULL, GetModuleHandle(NULL), NULL);
    if (!listbox) return false;
    ApplyDarkMode(listbox);
    uiObjects[objname] = { listbox, nullptr };
    return true;
#else
    GtkWidget* parent = uiObjects[parentname].widget;
    GtkWidget* listbox = gtk_list_box_new();
    gtk_fixed_put(GTK_FIXED(parent), listbox, left, top);
    gtk_widget_set_size_request(listbox, width, height);
    gtk_widget_show(listbox);
    uiObjects[objname] = { listbox, nullptr };
    return true;
#endif
}

// Listbox_Add: adds an item to the listbox.
XPLUGIN_API void Listbox_Add(const char* objname, const char* item) {
#ifdef _WIN32
    HWND listbox = uiObjects[objname].hwnd;
    SendMessageW(listbox, LB_ADDSTRING, 0, (LPARAM)toWString(item).c_str());
#else
    GtkWidget* listbox = uiObjects[objname].widget;
    GtkWidget* row = gtk_label_new(item);
    gtk_container_add(GTK_CONTAINER(listbox), row);
    gtk_widget_show_all(row);
#endif
}

#ifdef _WIN32
// SetFontName: sets the font name for a control.
XPLUGIN_API bool SetFontName(const char* objname, const char* fontname) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    HWND hwnd = it->second.hwnd;
    LOGFONT lf = {0};
    HFONT hOldFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
    if (hOldFont)
        GetObject(hOldFont, sizeof(lf), &lf);
    else
        lf.lfHeight = -12;
    std::wstring wFontName = toWString(fontname);
    wcsncpy_s(lf.lfFaceName, wFontName.c_str(), LF_FACESIZE);
    HFONT hFont = CreateFontIndirect(&lf);
    if (!hFont) return false;
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
    return true;
}

// SetFontSize: sets the font size for a control.
XPLUGIN_API bool SetFontSize(const char* objname, int size) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    HWND hwnd = it->second.hwnd;
    LOGFONT lf = {0};
    HFONT hOldFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
    if (hOldFont)
        GetObject(hOldFont, sizeof(lf), &lf);
    lf.lfHeight = -size;
    HFONT hFont = CreateFontIndirect(&lf);
    if (!hFont) return false;
    SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(FALSE, 0));
    return true;
}

// Listbox_RemoveAt: removes an item at index from a listbox.
XPLUGIN_API bool Listbox_RemoveAt(const char* objname, int index) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    LRESULT res = SendMessageW(it->second.hwnd, LB_DELETESTRING, index, 0);
    return (res != LB_ERR);
}

// Listbox_InsertAt: inserts an item at index in a listbox.
XPLUGIN_API bool Listbox_InsertAt(const char* objname, int index, const char* text) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    std::wstring wText = toWString(text);
    LRESULT res = SendMessageW(it->second.hwnd, LB_INSERTSTRING, index, (LPARAM)wText.c_str());
    return (res != LB_ERR);
}

// Listbox_DeleteAll: deletes all items from a listbox.
XPLUGIN_API bool Listbox_DeleteAll(const char* objname) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    LRESULT res = SendMessageW(it->second.hwnd, LB_RESETCONTENT, 0, 0);
    return (res != LB_ERR);
}

// Listbox_GetCellTextAt: retrieves text from a listbox item (columns delimited by tab).
XPLUGIN_API std::string Listbox_GetCellTextAt(const char* objname, int row, int col) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return "";
    wchar_t buffer[512] = {0};
    LRESULT res = SendMessageW(it->second.hwnd, LB_GETTEXT, row, (LPARAM)buffer);
    if (res == LB_ERR) return "";
    std::wstring ws(buffer);
    std::vector<std::wstring> parts = split(ws, L'\t');
    if (col < 0 || col >= (int)parts.size()) return "";
    return toString(parts[col]);
}

// Listbox_SetCellTextAt: sets text in a specific cell (by rewriting the entire row with tabs).
XPLUGIN_API std::string Listbox_SetCellTextAt(const char* objname, int row, int col, const char* text) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return "";
    wchar_t buffer[512] = {0};
    LRESULT res = SendMessageW(it->second.hwnd, LB_GETTEXT, row, (LPARAM)buffer);
    if (res == LB_ERR) return "";
    std::wstring ws(buffer);
    std::vector<std::wstring> parts = split(ws, L'\t');
    while (col >= (int)parts.size())
        parts.push_back(L"");
    parts[col] = toWString(text);
    std::wstring newItem = join(parts, L'\t');
    SendMessageW(it->second.hwnd, LB_DELETESTRING, row, 0);
    LRESULT ins = SendMessageW(it->second.hwnd, LB_INSERTSTRING, row, (LPARAM)newItem.c_str());
    if (ins == LB_ERR) return "";
    return toString(newItem);
}

// Set_ListBoxRowHeight: sets the row height in a listbox (owner-drawn).
XPLUGIN_API bool Set_ListBoxRowHeight(const char* objname, int rowHeight) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    LRESULT res = SendMessageW(it->second.hwnd, LB_SETITEMHEIGHT, 0, (LPARAM)rowHeight);
    return (res != LB_ERR);
}

// Popupbox (read-only dropdown) expansions:
XPLUGIN_API bool Popupbox_RemoveAt(const char* objname, int index) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    LRESULT res = SendMessageW(it->second.hwnd, CB_DELETESTRING, index, 0);
    return (res != CB_ERR);
}
XPLUGIN_API bool Popupbox_InsertAt(const char* objname, int index, const char* text) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    std::wstring wText = toWString(text);
    LRESULT res = SendMessageW(it->second.hwnd, CB_INSERTSTRING, index, (LPARAM)wText.c_str());
    return (res != CB_ERR);
}
XPLUGIN_API bool Popupbox_DeleteAll(const char* objname) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    LRESULT res = SendMessageW(it->second.hwnd, CB_RESETCONTENT, 0, 0);
    return (res != CB_ERR);
}
XPLUGIN_API std::string Popupbox_GetRowValueAt(const char* objname, int index) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return "";
    wchar_t buffer[512] = {0};
    LRESULT res = SendMessageW(it->second.hwnd, CB_GETLBTEXT, index, (LPARAM)buffer);
    if (res == CB_ERR) return "";
    return toString(buffer);
}
XPLUGIN_API std::string Popupbox_SetRowValueAt(const char* objname, int index, const char* text) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return "";
    LRESULT resDel = SendMessageW(it->second.hwnd, CB_DELETESTRING, index, 0);
    if (resDel == CB_ERR) return "";
    std::wstring wText = toWString(text);
    LRESULT resIns = SendMessageW(it->second.hwnd, CB_INSERTSTRING, index, (LPARAM)wText.c_str());
    if (resIns == CB_ERR) return "";
    return toString(wText);
}
// Popupbox_Add: adds an item to a read-only dropdown and selects it if none is selected.
XPLUGIN_API bool Popupbox_Add(const char* objname, const char* text) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    std::wstring wText = toWString(text);
    LRESULT index = SendMessageW(it->second.hwnd, CB_ADDSTRING, 0, (LPARAM)wText.c_str());
    if (index == CB_ERR) return false;
    if (SendMessageW(it->second.hwnd, CB_GETCURSEL, 0, 0) == CB_ERR)
        SendMessageW(it->second.hwnd, CB_SETCURSEL, index, 0);
    return true;
}

// ComboBox (editable dropdown) expansions:
XPLUGIN_API bool ComboBox_RemoveAt(const char* objname, int index) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    LRESULT res = SendMessageW(it->second.hwnd, CB_DELETESTRING, index, 0);
    return (res != CB_ERR);
}
XPLUGIN_API bool ComboBox_InsertAt(const char* objname, int index, const char* text) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    std::wstring wText = toWString(text);
    LRESULT res = SendMessageW(it->second.hwnd, CB_INSERTSTRING, index, (LPARAM)wText.c_str());
    return (res != CB_ERR);
}
XPLUGIN_API bool ComboBox_DeleteAll(const char* objname) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    LRESULT res = SendMessageW(it->second.hwnd, CB_RESETCONTENT, 0, 0);
    return (res != CB_ERR);
}
XPLUGIN_API std::string ComboBox_GetRowValueAt(const char* objname, int index) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return "";
    wchar_t buffer[512] = {0};
    LRESULT res = SendMessageW(it->second.hwnd, CB_GETLBTEXT, index, (LPARAM)buffer);
    if (res == CB_ERR) return "";
    return toString(buffer);
}
XPLUGIN_API std::string ComboBox_SetRowValueAt(const char* objname, int index, const char* text) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return "";
    LRESULT resDel = SendMessageW(it->second.hwnd, CB_DELETESTRING, index, 0);
    if (resDel == CB_ERR) return "";
    std::wstring wText = toWString(text);
    LRESULT resIns = SendMessageW(it->second.hwnd, CB_INSERTSTRING, index, (LPARAM)wText.c_str());
    if (resIns == CB_ERR) return "";
    return toString(wText);
}
// ComboBox_Add: adds an item to an editable dropdown and selects it if none selected.
XPLUGIN_API bool ComboBox_Add(const char* objname, const char* text) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    std::wstring wText = toWString(text);
    LRESULT index = SendMessageW(it->second.hwnd, CB_ADDSTRING, 0, (LPARAM)wText.c_str());
    if (index == CB_ERR) return false;
    if (SendMessageW(it->second.hwnd, CB_GETCURSEL, 0, 0) == CB_ERR)
        SendMessageW(it->second.hwnd, CB_SETCURSEL, index, 0);
    return true;
}

// SetDropdownHeight: sets the dropdown (list) height by specifying how many items are visible.
XPLUGIN_API bool SetDropdownHeight(const char* objname, int height) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    LRESULT itemHeight = SendMessageW(it->second.hwnd, CB_GETITEMHEIGHT, 0, 0);
    if (itemHeight <= 0) return false;
    int nItems = height / itemHeight;
    LRESULT res = SendMessageW(it->second.hwnd, CB_SETMINVISIBLE, (WPARAM)nItems, 0);
    return (res != CB_ERR);
}

// SetPopupBoxSelectedIndex: sets the selected index for a popupbox and updates its text if needed.
XPLUGIN_API bool SetPopupBoxSelectedIndex(const char* objname, int index, const char* text) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    LRESULT resDel = SendMessageW(it->second.hwnd, CB_DELETESTRING, index, 0);
    if (resDel == CB_ERR) return false;
    std::wstring wText = toWString(text);
    LRESULT resIns = SendMessageW(it->second.hwnd, CB_INSERTSTRING, index, (LPARAM)wText.c_str());
    if (resIns == CB_ERR) return false;
    LRESULT selRes = SendMessageW(it->second.hwnd, CB_SETCURSEL, index, 0);
    return (selRes != CB_ERR);
}

// GetPopupBoxSelectedIndex: returns the text of the selected index if it matches 'index', otherwise empty.
XPLUGIN_API std::string GetPopupBoxSelectedIndex(const char* objname, int index) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return "";
    LRESULT curSel = SendMessageW(it->second.hwnd, CB_GETCURSEL, 0, 0);
    if (curSel != index) return "";
    wchar_t buffer[512] = {0};
    LRESULT res = SendMessageW(it->second.hwnd, CB_GETLBTEXT, index, (LPARAM)buffer);
    if (res == CB_ERR) return "";
    return toString(buffer);
}

#else
// GTK stubs (for macOS/Linux). If on Linux/macOS, none of the advanced Windows logic applies:
XPLUGIN_API bool SetFontName(const char* objname, const char* fontname) { return false; }
XPLUGIN_API bool SetFontSize(const char* objname, int size) { return false; }
XPLUGIN_API bool Listbox_RemoveAt(const char* objname, int index) { return false; }
XPLUGIN_API bool Listbox_InsertAt(const char* objname, int index, const char* text) { return false; }
XPLUGIN_API bool Listbox_DeleteAll(const char* objname) { return false; }
XPLUGIN_API std::string Listbox_GetCellTextAt(const char* objname, int row, int col) { return ""; }
XPLUGIN_API std::string Listbox_SetCellTextAt(const char* objname, int row, int col, const char* text) { return ""; }
XPLUGIN_API bool Set_ListBoxRowHeight(const char* objname, int rowHeight) { return false; }

XPLUGIN_API bool Popupbox_RemoveAt(const char* objname, int index) { return false; }
XPLUGIN_API bool Popupbox_InsertAt(const char* objname, int index, const char* text) { return false; }
XPLUGIN_API bool Popupbox_DeleteAll(const char* objname) { return false; }
XPLUGIN_API std::string Popupbox_GetRowValueAt(const char* objname, int index) { return ""; }
XPLUGIN_API std::string Popupbox_SetRowValueAt(const char* objname, int index, const char* text) { return ""; }
XPLUGIN_API bool Popupbox_Add(const char* objname, const char* text) { return false; }

XPLUGIN_API bool ComboBox_RemoveAt(const char* objname, int index) { return false; }
XPLUGIN_API bool ComboBox_InsertAt(const char* objname, int index, const char* text) { return false; }
XPLUGIN_API bool ComboBox_DeleteAll(const char* objname) { return false; }
XPLUGIN_API std::string ComboBox_GetRowValueAt(const char* objname, int index) { return ""; }
XPLUGIN_API std::string ComboBox_SetRowValueAt(const char* objname, int index, const char* text) { return ""; }
XPLUGIN_API bool ComboBox_Add(const char* objname, const char* text) { return false; }
XPLUGIN_API bool SetDropdownHeight(const char* objname, int height) { return false; }
XPLUGIN_API bool SetPopupBoxSelectedIndex(const char* objname, int index, const char* text) { return false; }
XPLUGIN_API std::string GetPopupBoxSelectedIndex(const char* objname, int index) { return ""; }
#endif

// XRefresh: forces refresh/invalidate of all controls.
XPLUGIN_API void XRefresh(bool updatenow) {
#ifdef _WIN32
    for (auto& pair : uiObjects) {
        if (pair.second.hwnd) {
            InvalidateRect(pair.second.hwnd, NULL, TRUE);
            if (updatenow)
                UpdateWindow(pair.second.hwnd);
        }
    }
#else
    for (auto& pair : uiObjects) {
        if (pair.second.widget) {
            gtk_widget_queue_draw(pair.second.widget);
            if (updatenow) {
                while (gtk_events_pending())
                    gtk_main_iteration();
            }
        }
    }
#endif
}

#ifdef _WIN32
// DllMain for Windows
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch(ul_reason_for_call) {
        case DLL_PROCESS_ATTACH: {
            INITCOMMONCONTROLSEX icex;
            icex.dwSize = sizeof(icex);
            icex.dwICC = ICC_WIN95_CLASSES;
            InitCommonControlsEx(&icex);
            break;
        }
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
#endif

// PluginEntry structure and function table.
typedef struct PluginEntry {
    const char* name;
    void* funcPtr;
    int arity;
    const char* paramTypes[10];
    const char* retType;
} PluginEntry;

// The plugin's function table
static PluginEntry pluginEntries[] = {
    { "XCreateWindow",    (void*)CreateWindowX,    8,  {"string","integer","integer","integer","integer","boolean","boolean","boolean"}, "boolean" },
    { "XDestroyControl",  (void*)XDestroyControl,  1,  {"string"}, "boolean" },
    { "XAddButton",       (void*)XAddButton,       6,  {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddTextField",    (void*)XAddTextField,    6,  {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddTextArea",     (void*)XAddTextArea,     6,  {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddComboBox",     (void*)XAddComboBox,     6,  {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddPopupBox",     (void*)XAddPopupBox,     6,  {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddListBox",      (void*)XAddListBox,      6,  {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "Listbox_Add",      (void*)Listbox_Add,      2,  {"string","string"}, "boolean" },
    { "XRefresh",         (void*)XRefresh,         1,  {"boolean"}, "boolean" },
    { "Set_FontName",     (void*)SetFontName,      2,  {"string","string"}, "boolean" },
    { "Set_FontSize",     (void*)SetFontSize,      2,  {"string","integer"}, "boolean" },
    { "Listbox_RemoveAt", (void*)Listbox_RemoveAt, 2,  {"string","integer"}, "boolean" },
    { "Listbox_InsertAt", (void*)Listbox_InsertAt, 3,  {"string","integer","string"}, "boolean" },
    { "Listbox_DeleteAll",(void*)Listbox_DeleteAll,1,  {"string"}, "boolean" },
    { "Listbox_GetCellTextAt", (void*)Listbox_GetCellTextAt, 3, {"string","integer","integer"}, "string" },
    { "Listbox_SetCellTextAt", (void*)Listbox_SetCellTextAt, 4, {"string","integer","integer","string"}, "string" },
    { "Set_ListBoxRowHeight", (void*)Set_ListBoxRowHeight, 2, {"string","integer"}, "boolean" },
    { "Popupbox_RemoveAt", (void*)Popupbox_RemoveAt, 2, {"string","integer"}, "boolean" },
    { "Popupbox_InsertAt", (void*)Popupbox_InsertAt, 3, {"string","integer","string"}, "boolean" },
    { "Popupbox_DeleteAll", (void*)Popupbox_DeleteAll, 1, {"string"}, "boolean" },
    { "Popupbox_GetRowValueAt", (void*)Popupbox_GetRowValueAt, 2, {"string","integer"}, "string" },
    { "Popupbox_SetRowValueAt", (void*)Popupbox_SetRowValueAt, 3, {"string","integer","string"}, "string" },
    { "Popupbox_Add",     (void*)Popupbox_Add,     2, {"string","string"}, "boolean" },
    { "ComboBox_RemoveAt", (void*)ComboBox_RemoveAt, 2, {"string","integer"}, "boolean" },
    { "ComboBox_InsertAt", (void*)ComboBox_InsertAt, 3, {"string","integer","string"}, "boolean" },
    { "ComboBox_DeleteAll", (void*)ComboBox_DeleteAll, 1, {"string"}, "boolean" },
    { "ComboBox_GetRowValueAt", (void*)ComboBox_GetRowValueAt, 2, {"string","integer"}, "string" },
    { "ComboBox_SetRowValueAt", (void*)ComboBox_SetRowValueAt, 3, {"string","integer","string"}, "string" },
    { "ComboBox_Add",     (void*)ComboBox_Add,     2, {"string","string"}, "boolean" },
    { "Set_DropdownHeight", (void*)SetDropdownHeight, 2, {"string","integer"}, "boolean" },
    { "Set_PopupBoxSelectedIndex", (void*)SetPopupBoxSelectedIndex, 3, {"string","integer","string"}, "boolean" },
    { "Get_PopupBoxSelectedIndex", (void*)GetPopupBoxSelectedIndex, 2, {"string","integer"}, "string" }
};

// Exported function for the host (bytecode interpreter) to retrieve the plugin's function table.
extern "C" XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    if (count) {
        *count = sizeof(pluginEntries) / sizeof(PluginEntry);
    }
    return pluginEntries;
}
