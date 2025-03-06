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
#include <cwchar>

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

// Define CB_SETMINVISIBLE if not defined (value from Windows 10 SDK)
#ifndef CB_SETMINVISIBLE
#define CB_SETMINVISIBLE 0x1702
#endif

// For progress bar control.
#ifndef PROGRESS_CLASS
#define PROGRESS_CLASS PROGRESS_CLASSW
#endif

// For status bar control.
#ifndef STATUSCLASSNAME
#define STATUSCLASSNAME L"msctls_statusbar32"
#endif

// For up-down control.
#ifndef UPDOWN_CLASS
#define UPDOWN_CLASS L"msctls_updown32"
#endif

// Global tooltip control (shared).
static HWND gTooltip = NULL;

// Helper: convert narrow (char*) to wide (std::wstring)
std::wstring toWString(const char* s) {
    if (!s) return L"";
    int len = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
    std::wstring ws(len, L'\0');
    MultiByteToWideChar(CP_ACP, 0, s, -1, &ws[0], len);
    if (!ws.empty() && ws.back() == L'\0')
        ws.pop_back();
    return ws;
}

// Helper: convert wide (std::wstring) to narrow (std::string)
std::string toString(const std::wstring& ws) {
    if(ws.empty()) return "";
    int len = WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1, NULL, 0, NULL, NULL);
    std::string s(len, '\0');
    WideCharToMultiByte(CP_ACP, 0, ws.c_str(), -1, &s[0], len, NULL, NULL);
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
#else
#include <gtk/gtk.h>
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

// Updated UIObject struct now includes a control type string.
struct UIObject {
#ifdef _WIN32
    HWND hwnd;
#else
    GtkWidget* widget;
#endif
    std::function<void()> eventHandler;
    std::string type; // e.g., "statusbar", "datepicker", etc.
};

// Global map for controls.
std::map<std::string, UIObject> uiObjects;

#ifdef _WIN32
// Global dark-mode colors/brush.
static HBRUSH  gDarkBrush     = NULL;
static COLORREF gDarkBkgColor  = RGB(32, 32, 32);
static COLORREF gDarkTextColor = RGB(255, 255, 255);

// Apply dark mode attribute.
void ApplyDarkMode(HWND hwnd) {
    BOOL darkMode = TRUE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));
}

// Window procedure.
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
        case WM_WINDOWPOSCHANGED: {
            InvalidateRect(hwnd, NULL, TRUE);
            UpdateWindow(hwnd);
            // Reposition status bar controls to dock at the bottom.
            RECT rc;
            GetClientRect(hwnd, &rc);
            for (auto& obj : uiObjects) {
                if (obj.second.type == "statusbar" && GetParent(obj.second.hwnd) == hwnd) {
                    SendMessageW(obj.second.hwnd, WM_SIZE, 0, 0);
                }
            }
            break;
        }
        case WM_DRAWITEM: {
            LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
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
            break;
        }
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
    for (auto& obj : uiObjects) {
        if (obj.second.hwnd == hwnd && obj.second.eventHandler)
            obj.second.eventHandler();
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}
#else
static gboolean on_configure_event(GtkWidget* widget, GdkEvent* event, gpointer user_data) {
    gtk_widget_queue_draw(widget);
    return FALSE;
}
#endif

// XCreateWindow: creates the main window.
XPLUGIN_API bool CreateWindowX(const char* objname, int left, int top, int width, int height,
                               bool hasMin, bool hasMax, bool hasClose) {
#ifdef _WIN32
    WNDCLASS wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = GetModuleHandle(NULL);
    wc.lpszClassName = L"XojoPluginWindow";
    RegisterClass(&wc);
    
    DWORD style = WS_OVERLAPPEDWINDOW;
    if (!hasMin)  style &= ~WS_MINIMIZEBOX;
    if (!hasMax)  style &= ~WS_MAXIMIZEBOX;
    if (!hasClose) style &= ~WS_SYSMENU;
    
    std::wstring title = toWString(objname);
    HWND hwnd = CreateWindowExW(0, L"XojoPluginWindow", title.c_str(), style,
                                left, top, width, height,
                                NULL, NULL, GetModuleHandle(NULL), NULL);
    if (!hwnd) return false;
    ShowWindow(hwnd, SW_SHOW);
    ApplyDarkMode(hwnd);
    uiObjects[objname] = { hwnd, nullptr, "" };
    return true;
#else
    gtk_init(NULL, NULL);
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), objname);
    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(window, "configure-event", G_CALLBACK(on_configure_event), NULL);
    gtk_widget_show_all(window);
    uiObjects[objname] = { window, nullptr, "" };
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
    uiObjects[objname] = { button, nullptr, "" };
    return true;
#else
    GtkWidget* parent = uiObjects[parentname].widget;
    GtkWidget* button = gtk_button_new_with_label(objname);
    gtk_fixed_put(GTK_FIXED(parent), button, left, top);
    gtk_widget_set_size_request(button, width, height);
    gtk_widget_show(button);
    uiObjects[objname] = { button, nullptr, "" };
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
    uiObjects[objname] = { edit, nullptr, "" };
    return true;
#else
    GtkWidget* parent = uiObjects[parentname].widget;
    GtkWidget* entry = gtk_entry_new();
    gtk_fixed_put(GTK_FIXED(parent), entry, left, top);
    gtk_widget_set_size_request(entry, width, height);
    gtk_widget_show(entry);
    uiObjects[objname] = { entry, nullptr, "" };
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
    uiObjects[objname] = { edit, nullptr, "" };
    return true;
#else
    GtkWidget* parent = uiObjects[parentname].widget;
    GtkWidget* textview = gtk_text_view_new();
    gtk_fixed_put(GTK_FIXED(parent), textview, left, top);
    gtk_widget_set_size_request(textview, width, height);
    gtk_widget_show(textview);
    uiObjects[objname] = { textview, nullptr, "" };
    return true;
#endif
}

// XAddComboBox: creates an editable dropdown.
XPLUGIN_API bool XAddComboBox(const char* objname, const char* parentname,
                              int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND combo = CreateWindowW(L"COMBOBOX", L"",
                               WS_CHILD | WS_VISIBLE | CBS_DROPDOWN,
                               left, top, width, height,
                               parent, NULL, GetModuleHandle(NULL), NULL);
    if (!combo) return false;
    ApplyDarkMode(combo);
    uiObjects[objname] = { combo, nullptr, "" };
    return true;
#else
    GtkWidget* parent = uiObjects[parentname].widget;
    GtkWidget* combo = gtk_combo_box_text_new();
    gtk_fixed_put(GTK_FIXED(parent), combo, left, top);
    gtk_widget_set_size_request(combo, width, height);
    gtk_widget_show(combo);
    uiObjects[objname] = { combo, nullptr, "" };
    return true;
#endif
}

// XAddPopupBox: creates a read-only dropdown.
XPLUGIN_API bool XAddPopupBox(const char* objname, const char* parentname,
                              int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND combo = CreateWindowW(L"COMBOBOX", L"",
                               WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
                               left, top, width, height,
                               parent, NULL, GetModuleHandle(NULL), NULL);
    if (!combo) return false;
    ApplyDarkMode(combo);
    uiObjects[objname] = { combo, nullptr, "" };
    return true;
#else
    GtkWidget* parent = uiObjects[parentname].widget;
    GtkWidget* combo = gtk_combo_box_text_new();
    gtk_fixed_put(GTK_FIXED(parent), combo, left, top);
    gtk_widget_set_size_request(combo, width, height);
    gtk_widget_show(combo);
    uiObjects[objname] = { combo, nullptr, "" };
    return true;
#endif
}

// XAddListBox: creates an owner-drawn listbox with border.
XPLUGIN_API bool XAddListBox(const char* objname, const char* parentname,
                             int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND listbox = CreateWindowW(L"LISTBOX", L"",
                                 WS_CHILD | WS_VISIBLE | LBS_NOTIFY | LBS_OWNERDRAWFIXED |
                                 LBS_HASSTRINGS | WS_BORDER,
                                 left, top, width, height,
                                 parent, NULL, GetModuleHandle(NULL), NULL);
    if (!listbox) return false;
    ApplyDarkMode(listbox);
    uiObjects[objname] = { listbox, nullptr, "" };
    return true;
#else
    GtkWidget* parent = uiObjects[parentname].widget;
    GtkWidget* listbox = gtk_list_box_new();
    gtk_fixed_put(GTK_FIXED(parent), listbox, left, top);
    gtk_widget_set_size_request(listbox, width, height);
    gtk_widget_show(listbox);
    uiObjects[objname] = { listbox, nullptr, "" };
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

// Listbox_RemoveAt: removes an item at index.
XPLUGIN_API bool Listbox_RemoveAt(const char* objname, int index) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    LRESULT res = SendMessageW(it->second.hwnd, LB_DELETESTRING, index, 0);
    return (res != LB_ERR);
}

// Listbox_InsertAt: inserts an item at index.
XPLUGIN_API bool Listbox_InsertAt(const char* objname, int index, const char* text) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    std::wstring wText = toWString(text);
    LRESULT res = SendMessageW(it->second.hwnd, LB_INSERTSTRING, index, (LPARAM)wText.c_str());
    return (res != LB_ERR);
}

// Listbox_DeleteAll: deletes all items.
XPLUGIN_API bool Listbox_DeleteAll(const char* objname) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    LRESULT res = SendMessageW(it->second.hwnd, LB_RESETCONTENT, 0, 0);
    return (res != LB_ERR);
}

// Listbox_GetCellTextAt: retrieves text from an item (columns delimited by tab).
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

// Listbox_SetCellTextAt: sets text in a specific cell.
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

// Popupbox functions for read-only dropdown.
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

// Popupbox_Add: adds an item to a read-only dropdown and sets it as selected if none exists.
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

// ComboBox functions for editable dropdown.
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

// SetDropdownHeight: sets the dropdown (list) height for popupboxes and comboboxes.
XPLUGIN_API bool SetDropdownHeight(const char* objname, int height) {
    auto it = uiObjects.find(objname);
    if (it == uiObjects.end()) return false;
    LRESULT itemHeight = SendMessageW(it->second.hwnd, CB_GETITEMHEIGHT, 0, 0);
    if (itemHeight <= 0) return false;
    int nItems = height / itemHeight;
    LRESULT res = SendMessageW(it->second.hwnd, CB_SETMINVISIBLE, (WPARAM)nItems, 0);
    return (res != CB_ERR);
}

// SetPopupBoxSelectedIndex: sets the selected index for a popupbox and updates its text.
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

// GetPopupBoxSelectedIndex: returns the text of the selected index if it matches.
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
// GTK stubs.
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

// NEW FUNCTIONS: Additional Controls with Dark Mode Applied

// Checkbox
XPLUGIN_API bool XAddCheckBox(const char* objname, const char* parentname, const char* text, int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    std::wstring wText = toWString(text);
    HWND checkbox = CreateWindowW(L"BUTTON", wText.c_str(), WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
                                  left, top, width, height, parent, NULL, GetModuleHandle(NULL), NULL);
    if (!checkbox) return false;
    ApplyDarkMode(checkbox);
    uiObjects[objname] = { checkbox, nullptr, "checkbox" };
    return true;
#else
    return false;
#endif
}

XPLUGIN_API bool CheckBox_SetChecked(const char* objname, bool checked) {
#ifdef _WIN32
    auto it = uiObjects.find(objname);
    if(it == uiObjects.end()) return false;
    SendMessageW(it->second.hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    return true;
#else
    return false;
#endif
}

XPLUGIN_API bool CheckBox_GetChecked(const char* objname) {
#ifdef _WIN32
    auto it = uiObjects.find(objname);
    if(it == uiObjects.end()) return false;
    LRESULT res = SendMessageW(it->second.hwnd, BM_GETCHECK, 0, 0);
    return (res == BST_CHECKED);
#else
    return false;
#endif
}

// Radio Button
XPLUGIN_API bool XAddRadioButton(const char* objname, const char* parentname, const char* text, int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    std::wstring wText = toWString(text);
    HWND radio = CreateWindowW(L"BUTTON", wText.c_str(), WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                               left, top, width, height, parent, NULL, GetModuleHandle(NULL), NULL);
    if (!radio) return false;
    ApplyDarkMode(radio);
    uiObjects[objname] = { radio, nullptr, "radiobutton" };
    return true;
#else
    return false;
#endif
}

XPLUGIN_API bool RadioButton_SetChecked(const char* objname, bool checked) {
#ifdef _WIN32
    auto it = uiObjects.find(objname);
    if(it == uiObjects.end()) return false;
    SendMessageW(it->second.hwnd, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    return true;
#else
    return false;
#endif
}

XPLUGIN_API bool RadioButton_GetChecked(const char* objname) {
#ifdef _WIN32
    auto it = uiObjects.find(objname);
    if(it == uiObjects.end()) return false;
    LRESULT res = SendMessageW(it->second.hwnd, BM_GETCHECK, 0, 0);
    return (res == BST_CHECKED);
#else
    return false;
#endif
}

// Line (Horizontal)
XPLUGIN_API bool XAddLine(const char* objname, const char* parentname, int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND line = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ,
                              left, top, width, height, parent, NULL, GetModuleHandle(NULL), NULL);
    if (!line) return false;
    ApplyDarkMode(line);
    uiObjects[objname] = { line, nullptr, "line" };
    return true;
#else
    return false;
#endif
}

// GroupBox
XPLUGIN_API bool XAddGroupBox(const char* objname, const char* parentname, const char* text, int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    std::wstring wText = toWString(text);
    HWND group = CreateWindowW(L"BUTTON", wText.c_str(), WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                               left, top, width, height, parent, NULL, GetModuleHandle(NULL), NULL);
    if (!group) return false;
    ApplyDarkMode(group);
    uiObjects[objname] = { group, nullptr, "groupbox" };
    return true;
#else
    return false;
#endif
}

// Slider (Trackbar)
XPLUGIN_API bool XAddSlider(const char* objname, const char* parentname, int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND slider = CreateWindowW(TRACKBAR_CLASSW, L"",
                                WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
                                left, top, width, height,
                                parent, NULL, GetModuleHandle(NULL), NULL);
    if (!slider) return false;
    ApplyDarkMode(slider);
    uiObjects[objname] = { slider, nullptr, "slider" };
    return true;
#else
    return false;
#endif
}

XPLUGIN_API bool Slider_SetValue(const char* objname, int value) {
#ifdef _WIN32
    auto it = uiObjects.find(objname);
    if(it == uiObjects.end()) return false;
    SendMessageW(it->second.hwnd, TBM_SETPOS, TRUE, value);
    return true;
#else
    return false;
#endif
}

XPLUGIN_API int Slider_GetValue(const char* objname) {
#ifdef _WIN32
    auto it = uiObjects.find(objname);
    if(it == uiObjects.end()) return -1;
    return (int)SendMessageW(it->second.hwnd, TBM_GETPOS, 0, 0);
#else
    return -1;
#endif
}

// Color Picker
XPLUGIN_API bool XAddColorPicker(const char* objname, const char* parentname, int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND cp = CreateWindowW(L"SysColorPicker32", L"",
                            WS_CHILD | WS_VISIBLE,
                            left, top, width, height,
                            parent, NULL, GetModuleHandle(NULL), NULL);
    if (!cp) {
        cp = CreateWindowW(L"BUTTON", L"Color", WS_CHILD | WS_VISIBLE,
                           left, top, width, height,
                           parent, NULL, GetModuleHandle(NULL), NULL);
    }
    if (!cp) return false;
    ApplyDarkMode(cp);
    uiObjects[objname] = { cp, nullptr, "colorpicker" };
    return true;
#else
    return false;
#endif
}

// Windows Chart (simulated)
XPLUGIN_API bool XAddChart(const char* objname, const char* parentname, int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND chart = CreateWindowW(L"STATIC", L"Chart", WS_CHILD | WS_VISIBLE | SS_BLACKFRAME,
                                 left, top, width, height,
                                 parent, NULL, GetModuleHandle(NULL), NULL);
    if (!chart) return false;
    ApplyDarkMode(chart);
    uiObjects[objname] = { chart, nullptr, "chart" };
    return true;
#else
    return false;
#endif
}

// Movie Player using MCIWnd
XPLUGIN_API bool XAddMoviePlayer(const char* objname, const char* parentname, int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND movie = CreateWindowW(L"MCIWndClass", L"",
                               WS_CHILD | WS_VISIBLE,
                               left, top, width, height,
                               parent, NULL, GetModuleHandle(NULL), NULL);
    if (!movie) return false;
    ApplyDarkMode(movie);
    uiObjects[objname] = { movie, nullptr, "movieplayer" };
    return true;
#else
    return false;
#endif
}

// HTML Viewer (non-IE based; simulated as a static control)
XPLUGIN_API bool XAddHTMLViewer(const char* objname, const char* parentname, int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND html = CreateWindowW(L"STATIC", L"HTML Viewer", WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER,
                                left, top, width, height,
                                parent, NULL, GetModuleHandle(NULL), NULL);
    if (!html) return false;
    ApplyDarkMode(html);
    uiObjects[objname] = { html, nullptr, "htmlviewer" };
    return true;
#else
    return false;
#endif
}

// Date Picker (date only)
XPLUGIN_API bool XAddDatePicker(const char* objname, const char* parentname,
                                int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND dtp = CreateWindowW(L"SysDateTimePick32", L"",
                             WS_CHILD | WS_VISIBLE | DTS_SHORTDATEFORMAT,
                             left, top, width, height,
                             parent, NULL, GetModuleHandle(NULL), NULL);
    if (!dtp) return false;
    ApplyDarkMode(dtp);
    uiObjects[objname] = { dtp, nullptr, "datepicker" };
    return true;
#else
    return false;
#endif
}

// Time Picker (time only)
XPLUGIN_API bool XAddTimePicker(const char* objname, const char* parentname,
                                int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND tp = CreateWindowW(L"SysDateTimePick32", L"",
                            WS_CHILD | WS_VISIBLE | DTS_TIMEFORMAT,
                            left, top, width, height,
                            parent, NULL, GetModuleHandle(NULL), NULL);
    if (!tp) return false;
    ApplyDarkMode(tp);
    uiObjects[objname] = { tp, nullptr, "timepicker" };
    return true;
#else
    return false;
#endif
}

// Calendar Control
XPLUGIN_API bool XAddCalendarControl(const char* objname, const char* parentname,
                                     int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND cal = CreateWindowW(L"SysMonthCal32", L"",
                             WS_CHILD | WS_VISIBLE,
                             left, top, width, height,
                             parent, NULL, GetModuleHandle(NULL), NULL);
    if (!cal) return false;
    ApplyDarkMode(cal);
    uiObjects[objname] = { cal, nullptr, "calendar" };
    return true;
#else
    return false;
#endif
}

// Label (transparent non-editable)
XPLUGIN_API bool XAddLabel(const char* objname, const char* parentname, const char* text,
                           int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    std::wstring wText = toWString(text);
    HWND label = CreateWindowW(L"STATIC", wText.c_str(),
                               WS_CHILD | WS_VISIBLE | SS_LEFT,
                               left, top, width, height,
                               parent, NULL, GetModuleHandle(NULL), NULL);
    if (!label) return false;
    ApplyDarkMode(label);
    uiObjects[objname] = { label, nullptr, "label" };
    return true;
#else
    return false;
#endif
}

// Progress Bar
XPLUGIN_API bool XAddProgressBar(const char* objname, const char* parentname,
                                 int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND pb = CreateWindowW(PROGRESS_CLASS, L"",
                            WS_CHILD | WS_VISIBLE,
                            left, top, width, height,
                            parent, NULL, GetModuleHandle(NULL), NULL);
    if (!pb) return false;
    ApplyDarkMode(pb);
    uiObjects[objname] = { pb, nullptr, "progressbar" };
    return true;
#else
    return false;
#endif
}

XPLUGIN_API bool ProgressBar_SetValue(const char* objname, int value) {
#ifdef _WIN32
    auto it = uiObjects.find(objname);
    if(it == uiObjects.end()) return false;
    HWND pb = it->second.hwnd;
    LRESULT res = SendMessageW(pb, PBM_SETPOS, (WPARAM)value, 0);
    return (res != 0);
#else
    return false;
#endif
}

XPLUGIN_API int ProgressBar_GetValue(const char* objname) {
#ifdef _WIN32
    auto it = uiObjects.find(objname);
    if(it == uiObjects.end()) return -1;
    HWND pb = it->second.hwnd;
    LRESULT pos = SendMessageW(pb, PBM_GETPOS, 0, 0);
    return (int)pos;
#else
    return -1;
#endif
}

// Status Bar (docked)
XPLUGIN_API bool XAddStatusBar(const char* objname, const char* parentname) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND sb = CreateWindowW(STATUSCLASSNAME, L"",
                            WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                            0, 0, 0, 0,
                            parent, NULL, GetModuleHandle(NULL), NULL);
    if(!sb) return false;
    ApplyDarkMode(sb);
    uiObjects[objname] = { sb, nullptr, "statusbar" };
    return true;
#else
    return false;
#endif
}

// Tab Control
XPLUGIN_API bool XAddTabControl(const char* objname, const char* parentname,
                                int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND tab = CreateWindowW(WC_TABCONTROL, L"",
                             WS_CHILD | WS_VISIBLE | TCS_TABS,
                             left, top, width, height,
                             parent, NULL, GetModuleHandle(NULL), NULL);
    if (!tab) return false;
    ApplyDarkMode(tab);
    uiObjects[objname] = { tab, nullptr, "tabcontrol" };
    return true;
#else
    return false;
#endif
}

// Horizontal Scroll Bar
XPLUGIN_API bool XAddHScrollBar(const char* objname, const char* parentname,
                                int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND hScroll = CreateWindowW(L"SCROLLBAR", L"",
                                 WS_CHILD | WS_VISIBLE | SBS_HORZ,
                                 left, top, width, height,
                                 parent, NULL, GetModuleHandle(NULL), NULL);
    if(!hScroll) return false;
    ApplyDarkMode(hScroll);
    uiObjects[objname] = { hScroll, nullptr, "hscroll" };
    return true;
#else
    return false;
#endif
}

// Vertical Scroll Bar
XPLUGIN_API bool XAddVScrollBar(const char* objname, const char* parentname,
                                int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND vScroll = CreateWindowW(L"SCROLLBAR", L"",
                                 WS_CHILD | WS_VISIBLE | SBS_VERT,
                                 left, top, width, height,
                                 parent, NULL, GetModuleHandle(NULL), NULL);
    if(!vScroll) return false;
    ApplyDarkMode(vScroll);
    uiObjects[objname] = { vScroll, nullptr, "vscroll" };
    return true;
#else
    return false;
#endif
}

// ToolTip
XPLUGIN_API bool XAddToolTip(const char* attachControl, const char* tipText) {
#ifdef _WIN32
    auto it = uiObjects.find(attachControl);
    if(it == uiObjects.end()) return false;
    HWND ctrl = it->second.hwnd;
    HWND parent = GetParent(ctrl);
    if(!gTooltip) {
        gTooltip = CreateWindowExW(0, TOOLTIPS_CLASSW, L"",
                                   WS_POPUP | TTS_ALWAYSTIP,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   CW_USEDEFAULT, CW_USEDEFAULT,
                                   parent, NULL, GetModuleHandle(NULL), NULL);
        if(!gTooltip) return false;
    }
    TOOLINFOW ti = {0};
    ti.cbSize = sizeof(ti);
    ti.uFlags = TTF_SUBCLASS;
    ti.hwnd = parent;
    ti.hinst = GetModuleHandle(NULL);
    GetClientRect(ctrl, &ti.rect);
    MapWindowPoints(ctrl, parent, (LPPOINT)&ti.rect, 2);
    std::wstring wsTip = toWString(tipText);
    wchar_t* tipBuffer = new wchar_t[wsTip.size()+1];
    wcscpy(tipBuffer, wsTip.c_str());
    ti.lpszText = tipBuffer;
    ti.uId = (UINT_PTR)ctrl;
    SendMessageW(gTooltip, TTM_ADDTOOL, 0, (LPARAM)&ti);
    return true;
#else
    return false;
#endif
}

// Up-Down Control (Spin)
XPLUGIN_API bool XAddUpDownControl(const char* objname, const char* parentname,
                                   int left, int top, int width, int height) {
#ifdef _WIN32
    HWND parent = uiObjects[parentname].hwnd;
    HWND updown = CreateWindowW(UPDOWN_CLASS, L"",
                                WS_CHILD | WS_VISIBLE | UDS_ALIGNRIGHT | UDS_ARROWKEYS,
                                left, top, width, height,
                                parent, NULL, GetModuleHandle(NULL), NULL);
    if (!updown) return false;
    ApplyDarkMode(updown);
    uiObjects[objname] = { updown, nullptr, "updown" };
    return true;
#else
    return false;
#endif
}

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

static PluginEntry pluginEntries[] = {
    { "XCreateWindow",    (void*)CreateWindowX,    8, {"string","integer","integer","integer","integer","boolean","boolean","boolean"}, "boolean" },
    { "XDestroyControl",  (void*)XDestroyControl,  1, {"string"}, "boolean" },
    { "XAddButton",       (void*)XAddButton,       6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddTextField",    (void*)XAddTextField,    6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddTextArea",     (void*)XAddTextArea,     6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddComboBox",     (void*)XAddComboBox,     6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddPopupBox",     (void*)XAddPopupBox,     6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddListBox",      (void*)XAddListBox,      6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "Listbox_Add",      (void*)Listbox_Add,      2, {"string","string"}, "boolean" },
    { "XRefresh",         (void*)XRefresh,         1, {"boolean"}, "boolean" },
    { "Set_FontName",     (void*)SetFontName,      2, {"string","string"}, "boolean" },
    { "Set_FontSize",     (void*)SetFontSize,      2, {"string","integer"}, "boolean" },
    { "Listbox_RemoveAt", (void*)Listbox_RemoveAt, 2, {"string","integer"}, "boolean" },
    { "Listbox_InsertAt", (void*)Listbox_InsertAt, 3, {"string","integer","string"}, "boolean" },
    { "Listbox_DeleteAll",(void*)Listbox_DeleteAll,1, {"string"}, "boolean" },
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
    { "Get_PopupBoxSelectedIndex", (void*)GetPopupBoxSelectedIndex, 2, {"string","integer"}, "string" },
    // New control entries:
    { "XAddDatePicker", (void*)XAddDatePicker, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddTimePicker", (void*)XAddTimePicker, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddCalendarControl", (void*)XAddCalendarControl, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddLabel", (void*)XAddLabel, 7, {"string","string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddProgressBar", (void*)XAddProgressBar, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "ProgressBar_SetValue", (void*)ProgressBar_SetValue, 2, {"string","integer"}, "boolean" },
    { "ProgressBar_GetValue", (void*)ProgressBar_GetValue, 1, {"string"}, "integer" },
    { "XAddStatusBar", (void*)XAddStatusBar, 2, {"string","string"}, "boolean" },
    { "XAddTabControl", (void*)XAddTabControl, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddHScrollBar", (void*)XAddHScrollBar, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddVScrollBar", (void*)XAddVScrollBar, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddToolTip", (void*)XAddToolTip, 2, {"string","string"}, "boolean" },
    { "XAddUpDownControl", (void*)XAddUpDownControl, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddCheckBox", (void*)XAddCheckBox, 7, {"string","string","string","integer","integer","integer","integer"}, "boolean" },
    { "CheckBox_SetChecked", (void*)CheckBox_SetChecked, 2, {"string","boolean"}, "boolean" },
    { "CheckBox_GetChecked", (void*)CheckBox_GetChecked, 1, {"string"}, "boolean" },
    { "XAddRadioButton", (void*)XAddRadioButton, 7, {"string","string","string","integer","integer","integer","integer"}, "boolean" },
    { "RadioButton_SetChecked", (void*)RadioButton_SetChecked, 2, {"string","boolean"}, "boolean" },
    { "RadioButton_GetChecked", (void*)RadioButton_GetChecked, 1, {"string"}, "boolean" },
    { "XAddLine", (void*)XAddLine, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddGroupBox", (void*)XAddGroupBox, 7, {"string","string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddSlider", (void*)XAddSlider, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "Slider_SetValue", (void*)Slider_SetValue, 2, {"string","integer"}, "boolean" },
    { "Slider_GetValue", (void*)Slider_GetValue, 1, {"string"}, "integer" },
    { "XAddColorPicker", (void*)XAddColorPicker, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddChart", (void*)XAddChart, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddMoviePlayer", (void*)XAddMoviePlayer, 6, {"string","string","integer","integer","integer","integer"}, "boolean" },
    { "XAddHTMLViewer", (void*)XAddHTMLViewer, 6, {"string","string","integer","integer","integer","integer"}, "boolean" }
};

extern "C" XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    if (count) {
        *count = sizeof(pluginEntries) / sizeof(PluginEntry);
    }
    return pluginEntries;
}
