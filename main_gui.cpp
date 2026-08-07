#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <sstream>
#include <regex>
#include <functional>
#include <iostream>
#include <fstream>
#include <algorithm>

#define main friendnet_original_main
#include "SocialNetwork.cpp"   
#undef main

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "dwmapi.lib")


namespace Colors {
    const COLORREF Background = RGB(32, 32, 32);
    const COLORREF Text       = RGB(220, 220, 220);
    const COLORREF InputBg    = RGB(255, 255, 255);
    const COLORREF InputText  = RGB(0, 0, 0);
}

static std::wstring u2w(const std::string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

static std::string w2u(const std::wstring& w) {
    if (w.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &s[0], n, nullptr, nullptr);
    return s;
}

// Control IDs

enum {
    IDC_USERLIST = 1000,
    IDC_OUTPUT,
    IDC_EDIT_IDS,
    IDC_EDIT_NAMES,
    IDC_EDIT_CNAME,
    IDC_EDIT_CMEMBERS,

    BTN_SHOW_FRIENDS,
    BTN_SUGGEST,
    BTN_DISTANCES,
    BTN_ARE_CONN,
    BTN_PATH,
    BTN_MUTUAL,
    BTN_GROUPS,
    BTN_POPULAR,
    BTN_STATS,
    BTN_CIRCLES,
    BTN_ADD_USER,
    BTN_REMOVE_USER,
    BTN_RENAME,
    BTN_ADD_LINK,
    BTN_REMOVE_LINK,
    BTN_ADD_CIRCLE,
    BTN_REMOVE_CIRCLE,
    BTN_REFRESH,
    BTN_CLEAR_OUTPUT,
    BTN_KEY_USERS,
    BTN_COMMUNITIES,
    BTN_SPREADERS,

    IDM_FILE_LOAD,
    IDM_FILE_SAVE,
    IDM_FILE_CLEAR,
    IDM_FILE_EXIT
};

static const wchar_t* kClass = L"FriendNetDarkGUI";

// Main window class

class MainWindow {
public:
    MainWindow(HINSTANCE hInst)
        : m_instance(hInst)
    {
    }

    bool create() {
        WNDCLASSW wc = {};
        wc.lpfnWndProc   = WndProcStatic;
        wc.hInstance     = m_instance;
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = m_hBackgroundBrush;
        wc.lpszClassName = kClass;
        wc.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);

        if (!RegisterClassW(&wc))
            return false;

        // Create main window
        m_hwnd = CreateWindowW(
            kClass,
            L"FriendNet - Social Network Analyzer",
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT, 1200, 800,
            nullptr, nullptr, m_instance, this
        );
        if (!m_hwnd)
            return false;

        BOOL dark = TRUE;
        DwmSetWindowAttribute(m_hwnd, 20, &dark, sizeof(dark));

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        return true;
    }

    int run() {
        MSG msg;
        while (GetMessage(&msg, nullptr, 0, 0)) {
            if (m_hAccel && TranslateAcceleratorW(m_hwnd, m_hAccel, &msg))
                continue;
            if (IsDialogMessageW(m_hwnd, &msg))
                continue;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return (int)msg.wParam;
    }

private:
    HINSTANCE m_instance = nullptr;
    HWND      m_hwnd     = nullptr;
    HFONT     m_font     = nullptr;
    HFONT     m_monoFont = nullptr;
    HACCEL    m_hAccel   = nullptr;

    // Brushes
    HBRUSH m_hBackgroundBrush = CreateSolidBrush(Colors::Background);
    HBRUSH m_hInputBgBrush    = CreateSolidBrush(Colors::InputBg);

    // Child controls
    HWND m_userList   = nullptr;
    HWND m_output     = nullptr;
    HWND m_editIds    = nullptr;
    HWND m_editNames  = nullptr;
    HWND m_editCName  = nullptr;
    HWND m_editCMembers = nullptr;
    HWND m_labelIds   = nullptr;
    HWND m_labelNames = nullptr;
    HWND m_labelCName = nullptr;
    HWND m_labelCMembers = nullptr;

    FriendNet m_network;

    static LRESULT CALLBACK WndProcStatic(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        MainWindow* self = nullptr;
        if (msg == WM_CREATE) {
            auto cs = (CREATESTRUCTW*)lParam;
            self = (MainWindow*)cs->lpCreateParams;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)self);
            self->m_hwnd = hWnd;
        } else {
            self = (MainWindow*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);
        }
        if (self)
            return self->WndProc(msg, wParam, lParam);
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }

    LRESULT WndProc(UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
        case WM_CREATE:
            onCreate();
            return 0;

        case WM_SIZE:
            onSize(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_COMMAND:
            onCommand(LOWORD(wParam), HIWORD(wParam), (HWND)lParam);
            return 0;

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORLISTBOX: {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, Colors::InputText);
            SetBkColor(hdc, Colors::InputBg);
            return (LRESULT)m_hInputBgBrush;
        }

        case WM_CTLCOLORSTATIC: {
            HDC hdc = (HDC)wParam;
            HWND ctrl = (HWND)lParam;
            if (ctrl == m_output) {
                SetTextColor(hdc, Colors::InputText);
                SetBkColor(hdc, Colors::InputBg);
                return (LRESULT)m_hInputBgBrush;
            } else {
                SetTextColor(hdc, Colors::Text);
                SetBkColor(hdc, Colors::Background);
                return (LRESULT)m_hBackgroundBrush;
            }
        }

        case WM_GETMINMAXINFO: {
            auto mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 950;
            mmi->ptMinTrackSize.y = 600;
            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProcW(m_hwnd, msg, wParam, lParam);
    }

    HWND createButton(const wchar_t* text, int id) {
        HWND btn = CreateWindowW(
            L"BUTTON", text,
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
            0, 0, 10, 10,
            m_hwnd, (HMENU)(INT_PTR)id, m_instance, nullptr
        );
        SendMessageW(btn, WM_SETFONT, (WPARAM)m_font, TRUE);
        return btn;
    }

    HWND createLabel(const wchar_t* text) {
        HWND lbl = CreateWindowW(
            L"STATIC", text,
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            0, 0, 10, 10,
            m_hwnd, nullptr, m_instance, nullptr
        );
        SendMessageW(lbl, WM_SETFONT, (WPARAM)m_font, TRUE);
        return lbl;
    }

    HWND createEdit(int id, DWORD extraStyle = 0) {
        HWND ed = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP | extraStyle,
            0, 0, 10, 10,
            m_hwnd, (HMENU)(INT_PTR)id, m_instance, nullptr
        );
        SendMessageW(ed, WM_SETFONT, (WPARAM)m_font, TRUE);
        return ed;
    }

    void onCreate() {
        m_font = CreateFontW(
            15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FF_DONTCARE, L"Segoe UI"
        );
        m_monoFont = CreateFontW(
            14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, FF_DONTCARE, L"Consolas"
        );

        // menu
        HMENU hFile = CreateMenu();
        AppendMenuW(hFile, MF_STRING, IDM_FILE_LOAD,  L"&Load JSON...\tCtrl+O");
        AppendMenuW(hFile, MF_STRING, IDM_FILE_SAVE,  L"&Save JSON...\tCtrl+S");
        AppendMenuW(hFile, MF_STRING, IDM_FILE_CLEAR, L"&Clear Network");
        AppendMenuW(hFile, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(hFile, MF_STRING, IDM_FILE_EXIT,  L"E&xit");

        HMENU hBar = CreateMenu();
        AppendMenuW(hBar, MF_POPUP, (UINT_PTR)hFile, L"&File");
        SetMenu(m_hwnd, hBar);

        ACCEL acc[] = {
            { FCONTROL | FVIRTKEY, 'O', IDM_FILE_LOAD },
            { FCONTROL | FVIRTKEY, 'S', IDM_FILE_SAVE }
        };
        m_hAccel = CreateAcceleratorTableW(acc, 2);

        // User list 
        m_userList = CreateWindowExW(
            0, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_BORDER,
            0, 0, 10, 10,
            m_hwnd, (HMENU)(INT_PTR)IDC_USERLIST, m_instance, nullptr
        );
        ListView_SetExtendedListViewStyle(m_userList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LVCOLUMNW col = {};
        col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        col.iSubItem = 0;
        col.cx = 140;
        col.pszText = (LPWSTR)L"ID";
        ListView_InsertColumn(m_userList, 0, &col);
        col.iSubItem = 1;
        col.cx = 260;
        col.pszText = (LPWSTR)L"Name";
        ListView_InsertColumn(m_userList, 1, &col);
        SendMessageW(m_userList, WM_SETFONT, (WPARAM)m_font, TRUE);

        ListView_SetBkColor(m_userList, Colors::InputBg);
        ListView_SetTextBkColor(m_userList, Colors::InputBg);
        ListView_SetTextColor(m_userList, Colors::InputText);

        // Output 
        m_output = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
            ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_WANTRETURN,
            0, 0, 10, 10,
            m_hwnd, (HMENU)(INT_PTR)IDC_OUTPUT, m_instance, nullptr
        );
        SendMessageW(m_output, WM_SETFONT, (WPARAM)m_monoFont, TRUE);

        // Labels and input fields
        m_labelIds      = createLabel(L"IDs (comma separated):");
        m_editIds       = createEdit(IDC_EDIT_IDS);

        m_labelNames    = createLabel(L"Names / K value:");  
        m_editNames     = createEdit(IDC_EDIT_NAMES);

        m_labelCName    = createLabel(L"Circle name:");
        m_editCName     = createEdit(IDC_EDIT_CNAME);

        m_labelCMembers = createLabel(L"Circle members (comma separated IDs):");
        m_editCMembers  = createEdit(IDC_EDIT_CMEMBERS);

        createButton(L"Show Friends",    BTN_SHOW_FRIENDS);
        createButton(L"Suggest Friends", BTN_SUGGEST);
        createButton(L"Show Distances",  BTN_DISTANCES);
        createButton(L"Are Connected?",  BTN_ARE_CONN);
        createButton(L"Shortest Path",   BTN_PATH);
        createButton(L"Mutual Friends",  BTN_MUTUAL);
        createButton(L"Find Groups",     BTN_GROUPS);
        createButton(L"Most Popular",    BTN_POPULAR);
        createButton(L"Statistics",      BTN_STATS);
        createButton(L"List Circles",    BTN_CIRCLES);
        createButton(L"Add User(s)",     BTN_ADD_USER);
        createButton(L"Remove User(s)",  BTN_REMOVE_USER);
        createButton(L"Rename User(s)",  BTN_RENAME);
        createButton(L"Add Link(s)",     BTN_ADD_LINK);
        createButton(L"Remove Link(s)",  BTN_REMOVE_LINK);
        createButton(L"Add Circle",      BTN_ADD_CIRCLE);
        createButton(L"Remove Circle",   BTN_REMOVE_CIRCLE);
        createButton(L"Refresh Users",   BTN_REFRESH);
        createButton(L"Clear Output",    BTN_CLEAR_OUTPUT);
        createButton(L"Key Users",       BTN_KEY_USERS);
        createButton(L"Communities",     BTN_COMMUNITIES);
        createButton(L"Best Spreaders",  BTN_SPREADERS);

        appendOutput("FriendNet GUI ready.\r\n");
        appendOutput("Use File > Load JSON to load a network, or add users manually.\r\n");
        appendOutput("TIP: You can enter multiple IDs separated by commas (e.g., A1, A2, A3) for bulk operations!\r\n\r\n");

        refreshUserList();

        RECT rc;
        GetClientRect(m_hwnd, &rc);
        onSize(rc.right, rc.bottom);
    }

    // Layout helper
    void setPos(HWND hCtrl, int x, int y, int w, int h) {
        MoveWindow(hCtrl, x, y, w, h, TRUE);
    }

    void onSize(int width, int height) {
        if (width <= 0 || height <= 0)
            return;

        const int margin = 10;
        const int editH = 24;
        const int btnH  = 32;
        const int gap   = 8;

        int y = margin;
        int x = margin;

        auto placeRowFields = [&](HWND lbl, int labelW, HWND edit, int editW) {
            setPos(lbl, x, y + 4, labelW, editH);
            x += labelW + 4;
            setPos(edit, x, y, editW, editH + 4);
            x += editW + 12;
        };

        placeRowFields(m_labelIds, 150, m_editIds, 200);
        placeRowFields(m_labelNames, 100, m_editNames, 200);
        placeRowFields(m_labelCName, 90, m_editCName, 150);

        setPos(m_labelCMembers, x, y + 4, 220, editH);
        x += 220 + 4;
        setPos(m_editCMembers, x, y, width - x - margin, editH + 4);

        y += editH + 15;

        auto placeButtonRow = [&](const std::vector<int>& ids, int yy) {
            int count = (int)ids.size();
            int bw = (width - 2 * margin - (count - 1) * gap) / count;
            int xx = margin;
            for (int id : ids) {
                HWND btn = GetDlgItem(m_hwnd, id);
                if (btn)
                    setPos(btn, xx, yy, bw, btnH);
                xx += bw + gap;
            }
        };

        placeButtonRow({ BTN_SHOW_FRIENDS, BTN_ARE_CONN, BTN_PATH, BTN_GROUPS }, y);
        y += btnH + gap;
        placeButtonRow({ BTN_SUGGEST, BTN_MUTUAL, BTN_DISTANCES, BTN_POPULAR }, y);
        y += btnH + gap;
        placeButtonRow({ BTN_STATS, BTN_CIRCLES, BTN_REFRESH, BTN_CLEAR_OUTPUT }, y);
        y += btnH + gap + 10;

        placeButtonRow({
            BTN_ADD_USER, BTN_REMOVE_USER, BTN_RENAME,
            BTN_ADD_LINK, BTN_REMOVE_LINK,
            BTN_ADD_CIRCLE, BTN_REMOVE_CIRCLE,
            BTN_KEY_USERS, BTN_COMMUNITIES, BTN_SPREADERS
        }, y);
        y += btnH + 15;

        int listH = 200;
        setPos(m_userList, margin, y, width - 2 * margin, listH);
        y += listH + 10;

        setPos(m_output, margin, y, width - 2 * margin, height - y - margin);
    }

    std::string getCtrlText(HWND hCtrl) {
        int len = GetWindowTextLengthW(hCtrl);
        if (len <= 0)
            return "";
        std::wstring w(len, 0);
        GetWindowTextW(hCtrl, &w[0], len + 1);
        return w2u(w);
    }

    // Append text to the output box 
    void appendOutput(const std::string& text) {
        std::string cleaned;
        cleaned.reserve(text.size() + 8);
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '\n' && (i == 0 || text[i - 1] != '\r'))
                cleaned += "\r\n";
            else
                cleaned += text[i];
        }
        std::wstring wide = u2w(cleaned);
        int len = GetWindowTextLengthW(m_output);
        SendMessageW(m_output, EM_SETSEL, len, len);
        SendMessageW(m_output, EM_REPLACESEL, FALSE, (LPARAM)wide.c_str());
    }

    // Capture cout/cerr output 
    // forward it to the output box
    void captureAndShow(std::function<void()> func) {
        std::ostringstream ss;
        std::streambuf* old_cout = std::cout.rdbuf(ss.rdbuf());
        std::streambuf* old_cerr = std::cerr.rdbuf(ss.rdbuf());
        try {
            func();
        } catch (const std::exception& e) {
            appendOutput(std::string("Exception: ") + e.what() + "\n");
        }
        std::cout.rdbuf(old_cout);
        std::cerr.rdbuf(old_cerr);
        if (!ss.str().empty())
            appendOutput(ss.str());
    }

    std::vector<std::string> getIds() {
        return splitCsv(getCtrlText(m_editIds));
    }

    std::vector<std::string> getNames() {
        return splitCsv(getCtrlText(m_editNames));
    }

    std::string getCircleName() {
        return getCtrlText(m_editCName);
    }

    std::string getCircleMembersRaw() {
        return getCtrlText(m_editCMembers);
    }

    static std::vector<std::string> splitCsv(const std::string& s) {
        std::vector<std::string> result;
        std::string cur;
        auto trim = [](std::string t) {
            size_t a = t.find_first_not_of(" \t");
            size_t b = t.find_last_not_of(" \t");
            if (a == std::string::npos)
                return std::string();
            return t.substr(a, b - a + 1);
        };
        for (char c : s) {
            if (c == ',' || c == ';') {
                auto t = trim(cur);
                if (!t.empty())
                    result.push_back(t);
                cur.clear();
            } else {
                cur += c;
            }
        }
        auto t = trim(cur);
        if (!t.empty())
            result.push_back(t);
        return result;
    }

    void refreshUserList() {
        SendMessageW(m_userList, WM_SETREDRAW, FALSE, 0);
        ListView_DeleteAllItems(m_userList);

        std::ostringstream ss;
        std::streambuf* old = std::cout.rdbuf(ss.rdbuf());
        m_network.show_all();
        std::cout.rdbuf(old);
        std::string output = ss.str();

        std::istringstream iss(output);
        std::string line;
        std::vector<std::pair<std::string, std::string>> users;
        std::regex re(R"(^\s*(\S+)\s+\((.*)\)\s+-\s+\d+\s+friends\s*$)");
        while (std::getline(iss, line)) {
            std::smatch match;
            if (std::regex_match(line, match, re)) {
                users.push_back({ match[1].str(), match[2].str() });
            }
        }

        std::sort(
            users.begin(), 
            users.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; }
        );

        int idx = 0;
        for (auto& u : users) {
            std::wstring idW   = u2w(u.first);
            std::wstring nameW = u2w(u.second);
            LVITEMW it = {};
            it.mask = LVIF_TEXT;
            it.iItem = idx;
            it.iSubItem = 0;
            it.pszText = (LPWSTR)idW.c_str();
            ListView_InsertItem(m_userList, &it);
            ListView_SetItemText(m_userList, idx, 1, (LPWSTR)nameW.c_str());
            ++idx;
        }

        SendMessageW(m_userList, WM_SETREDRAW, TRUE, 0);
        InvalidateRect(m_userList, nullptr, TRUE);
    }

    // File menu handlers
    void doLoad() {
        wchar_t buf[MAX_PATH] = { 0 };
        OPENFILENAMEW ofn = {};
        ofn.lStructSize     = sizeof(ofn);
        ofn.hwndOwner       = m_hwnd;
        ofn.lpstrFilter     = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile       = buf;
        ofn.nMaxFile        = MAX_PATH;
        ofn.Flags           = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt     = L"json";

        if (GetOpenFileNameW(&ofn)) {
            std::string filename = w2u(buf);
            captureAndShow([&]() { m_network.load_from_file(filename); });
            refreshUserList();
        }
    }

    void doSave() {
        wchar_t buf[MAX_PATH] = { 0 };
        OPENFILENAMEW ofn = {};
        ofn.lStructSize     = sizeof(ofn);
        ofn.hwndOwner       = m_hwnd;
        ofn.lpstrFilter     = L"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile       = buf;
        ofn.nMaxFile        = MAX_PATH;
        ofn.Flags           = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        ofn.lpstrDefExt     = L"json";

        if (GetSaveFileNameW(&ofn)) {
            std::string filename = w2u(buf);
            captureAndShow([&]() { m_network.save_to_file(filename); });
        }
    }

    void onCommand(int id, int code, HWND hCtrl) {
        switch (id) {
        case IDM_FILE_LOAD:
            doLoad();
            return;
        case IDM_FILE_SAVE:
            doSave();
            return;
        case IDM_FILE_CLEAR:
            captureAndShow([&]() {
                m_network.clear_all();
                std::cout << "[OK] Network cleared.\n";
            });
            refreshUserList();
            return;
        case IDM_FILE_EXIT:
            DestroyWindow(m_hwnd);
            return;
        }

        if (code != BN_CLICKED)
            return;

        auto ids   = getIds();
        auto names = getNames();

        switch (id) {
        case BTN_SHOW_FRIENDS:
            captureAndShow([&]() {
                for (const auto& id : ids)
                    m_network.show_friends(id);
            });
            break;

        case BTN_ARE_CONN:
            captureAndShow([&]() {
                for (size_t i = 0; i + 1 < ids.size(); i += 2)
                    m_network.check_connectiv_ity(ids[i], ids[i + 1]);
            });
            break;

        case BTN_PATH:
            captureAndShow([&]() {
                for (size_t i = 0; i + 1 < ids.size(); i += 2)
                    m_network.find_path(ids[i], ids[i + 1]);
            });
            break;

        case BTN_SUGGEST:
            captureAndShow([&]() {
                for (const auto& id : ids)
                    m_network.suggest_friends(id);
            });
            break;

        case BTN_GROUPS:
            captureAndShow([&]() {
                m_network.display_groups();
            });
            break;

        case BTN_POPULAR:
            captureAndShow([&]() {
                m_network.show_popular();
            });
            break;

        case BTN_MUTUAL:
            captureAndShow([&]() {
                for (size_t i = 0; i + 1 < ids.size(); i += 2)
                    m_network.show_mutual(ids[i], ids[i + 1]);
            });
            break;

        case BTN_STATS:
            captureAndShow([&]() {
                m_network.showStats();
            });
            break;

        case BTN_DISTANCES:
            captureAndShow([&]() {
                for (const auto& id : ids)
                    m_network.show_distances(id);
            });
            break;

        case BTN_CIRCLES:
            captureAndShow([&]() {
                m_network.show_circles();
            });
            break;

        case BTN_ADD_CIRCLE:
            captureAndShow([&]() {
                m_network.add_circle(getCircleName(), splitCsv(getCircleMembersRaw()));
            });
            break;

        case BTN_REMOVE_CIRCLE:
            captureAndShow([&]() {
                m_network.remove_circle(getCircleName());
            });
            break;

        case BTN_ADD_USER:
            captureAndShow([&]() {
                for (size_t i = 0; i < ids.size(); ++i) {
                    std::string name = (i < names.size()) ? names[i] : "User " + ids[i];
                    m_network.add_person(ids[i], name);
                }
            });
            refreshUserList();
            break;

        case BTN_REMOVE_USER:
            captureAndShow([&]() {
                for (const auto& id : ids)
                    m_network.remove_person(id);
            });
            refreshUserList();
            break;

        case BTN_RENAME:
            captureAndShow([&]() {
                for (size_t i = 0; i < ids.size() && i < names.size(); ++i)
                    m_network.rename_person(ids[i], names[i]);
            });
            refreshUserList();
            break;

        case BTN_ADD_LINK:
            captureAndShow([&]() {
                for (size_t i = 0; i + 1 < ids.size(); i += 2)
                    m_network.add_link(ids[i], ids[i + 1]);
            });
            break;

        case BTN_REMOVE_LINK:
            captureAndShow([&]() {
                for (size_t i = 0; i + 1 < ids.size(); i += 2)
                    m_network.remove_link(ids[i], ids[i + 1]);
            });
            break;

        case BTN_KEY_USERS:
            captureAndShow([&]() {
                m_network.findKeyUsers();
            });
            break;

        case BTN_COMMUNITIES:
            captureAndShow([&]() {
                m_network.detect_communities();
            });
            break;

        case BTN_SPREADERS: {
            int k = 3;  // default
            try {
                std::string kStr = getCtrlText(m_editNames);
                if (!kStr.empty())
                    k = std::stoi(kStr);
            } catch (...) {
                // ignore and keep default
            }
            captureAndShow([&]() {
                m_network.findBestSpreaders(k);
            });
            break;
        }

        case BTN_REFRESH:
            refreshUserList();
            break;

        case BTN_CLEAR_OUTPUT:
            SetWindowTextW(m_output, L"");
            break;
        }
    }
};

// main
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nShow) {
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&icc);

    SetConsoleOutputCP(CP_UTF8);

    MainWindow window(hInstance);
    if (!window.create()) {
        MessageBoxW(nullptr, L"Failed to create main window.", L"FriendNet", MB_ICONERROR);
        return 1;
    }
    return window.run();
}