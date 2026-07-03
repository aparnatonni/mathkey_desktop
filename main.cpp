#include <cstdio>
#include <windows.h>
#include <string>
#include <map>
#include <shellapi.h>

// Tray icon ID and custom message
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_TOGGLE 1002

// Global variables
std::string buffer = "";
bool mathModeEnabled = true;
NOTIFYICONDATA nid = {0};
HWND hwnd;

// MathKey symbol mappings
std::map<std::string, std::string> mathSymbols = {
    // Greek letters
    {"alpha", "α"}, {"beta", "β"}, {"gamma", "γ"},
    {"delta", "δ"}, {"epsilon", "ε"}, {"theta", "θ"},
    {"lambda", "λ"}, {"mu", "μ"}, {"pi", "π"},
    {"sigma", "σ"}, {"phi", "φ"}, {"omega", "ω"},

    // Operations
    {"plus", "+"}, {"minus", "-"}, {"times", "×"},
    {"equal", "="}, {"leq", "≤"}, {"geq", "≥"},
    {"neq", "≠"}, {"approx", "≈"}, {"infinity", "∞"},
    {"pm", "±"},

    // Powers (standalone)
    {"square", "²"}, {"squared", "²"},
    {"cube", "³"}, {"cubed", "³"},
    {"inverse", "⁻¹"},

    // Roots
    {"sqrt", "√"}, {"cbrt", "∛"},

    // Calculus
    {"integral", "∫"}, {"partial", "∂"},
    {"nabla", "∇"}, {"sum", "∑"},

    // Sets
    {"union", "∪"}, {"intersect", "∩"},
    {"subset", "⊂"}, {"in", "∈"},
    {"forall", "∀"}, {"exists", "∃"},

    // Arrows
    {"to", "→"}, {"implies", "⇒"}, {"iff", "⇔"},
};

// Nested script map — superscript and subscript
std::map<std::string, std::map<char, std::string>> scriptMap = {
    {"superscript", {
        {'0',"⁰"},{'1',"¹"},{'2',"²"},{'3',"³"},{'4',"⁴"},
        {'5',"⁵"},{'6',"⁶"},{'7',"⁷"},{'8',"⁸"},{'9',"⁹"},
        {'n',"ⁿ"},{'x',"ˣ"},{'a',"ᵃ"},{'b',"ᵇ"},{'c',"ᶜ"},
        {'d',"ᵈ"},{'e',"ᵉ"},{'i',"ⁱ"},{'j',"ʲ"},{'k',"ᵏ"},
        {'m',"ᵐ"},{'o',"ᵒ"},{'p',"ᵖ"},{'r',"ʳ"},{'s',"ˢ"},
        {'t',"ᵗ"},{'u',"ᵘ"},{'v',"ᵛ"},{'w',"ʷ"},{'y',"ʸ"},
        {'z',"ᶻ"},{'-',"⁻"},{'+',"⁺"}
    }},
    {"subscript", {
        {'0',"₀"},{'1',"₁"},{'2',"₂"},{'3',"₃"},{'4',"₄"},
        {'5',"₅"},{'6',"₆"},{'7',"₇"},{'8',"₈"},{'9',"₉"},
        {'a',"ₐ"},{'e',"ₑ"},{'o',"ₒ"},{'x',"ₓ"},{'n',"ₙ"},
        {'i',"ᵢ"},{'j',"ⱼ"},{'k',"ₖ"},{'m',"ₘ"},{'p',"ₚ"},
        {'r',"ᵣ"},{'s',"ₛ"},{'t',"ₜ"},{'u',"ᵤ"},{'v',"ᵥ"}
    }}
};

// Convert string using script map
std::string convertScript(const std::string& scriptType, const std::string& chars) {
    std::string result = "";
    for (char c : chars) {
        auto it = scriptMap[scriptType].find(c);
        if (it != scriptMap[scriptType].end()) {
            result += it->second;
        } else {
            result += c;
        }
    }
    return result;
}

// Delete n characters before cursor
void deleteChars(int n) {
    for (int i = 0; i < n; i++) {
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_BACK;
        SendInput(1, &input, sizeof(INPUT));
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }
}

// Type a unicode string
void typeUnicode(const std::string& text) {
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
    std::wstring wtext(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, &wtext[0], wlen);

    for (wchar_t wc : wtext) {
        if (wc == 0) continue;
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = 0;
        input.ki.wScan = wc;
        input.ki.dwFlags = KEYEVENTF_UNICODE;
        SendInput(1, &input, sizeof(INPUT));
        input.ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }
}

// Call Python parser
std::string callPythonParser(const std::string& text) {
    std::string command = "python C:\\Users\\Administrator\\Downloads\\Mathkey\\convert.py " + text;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hReadPipe, hWritePipe;
    CreatePipe(&hReadPipe, &hWritePipe, &sa, 0);

    STARTUPINFOA si = {0};
    si.cb = sizeof(STARTUPINFOA);
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    PROCESS_INFORMATION pi = {0};
    CreateProcessA(NULL, (LPSTR)command.c_str(), NULL, NULL,
        TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);

    CloseHandle(hWritePipe);

    char buf[256];
    std::string result = "";
    DWORD bytesRead;
    while (ReadFile(hReadPipe, buf, sizeof(buf) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buf[bytesRead] = '\0';
        result += buf;
    }

    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }

    return result;
}

// Check buffer and convert
void checkBuffer() {
    if (buffer.empty() || !mathModeEnabled) return;

    std::string result = "";

    // Pattern 1: "power" + chars → superscript
    if (buffer.length() > 5 && buffer.substr(0, 5) == "power") {
        std::string exp = buffer.substr(5);
        result = convertScript("superscript", exp);
    }

    // Pattern 2: "sub" + chars → subscript
    else if (buffer.length() > 3 && buffer.substr(0, 3) == "sub") {
        std::string sub = buffer.substr(3);
        result = convertScript("subscript", sub);
    }

    // Pattern 3: check single word map
    else {
        auto it = mathSymbols.find(buffer);
        if (it != mathSymbols.end()) {
            result = it->second;
        }
    }

    // Pattern 4: fallback to Python parser
    if (result.empty()) {
        result = callPythonParser(buffer);
        if (result.empty() || result == buffer) {
            result = "";
        }
    }

    if (!result.empty()) {
        deleteChars(buffer.length() + 1);
        Sleep(50);
        typeUnicode(result);
        typeUnicode(" ");
    }
}

// Update tray icon tooltip based on mode
void updateTrayIcon() {
    if (mathModeEnabled) {
        lstrcpy(nid.szTip, "MathKey - ON (Ctrl+Alt+M to toggle)");
    } else {
        lstrcpy(nid.szTip, "MathKey - OFF (Ctrl+Alt+M to toggle)");
    }
    Shell_NotifyIcon(NIM_MODIFY, &nid);
}

// Window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TRAYICON) {
        if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);

            HMENU menu = CreatePopupMenu();
            if (mathModeEnabled) {
                AppendMenu(menu, MF_STRING, ID_TRAY_TOGGLE, "Disable MathKey");
            } else {
                AppendMenu(menu, MF_STRING, ID_TRAY_TOGGLE, "Enable MathKey");
            }
            AppendMenu(menu, MF_SEPARATOR, 0, NULL);
            AppendMenu(menu, MF_STRING, ID_TRAY_EXIT, "Exit");

            SetForegroundWindow(hwnd);
            TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(menu);
        }
    }

    if (msg == WM_COMMAND) {
        if (LOWORD(wParam) == ID_TRAY_EXIT) {
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
        }
        if (LOWORD(wParam) == ID_TRAY_TOGGLE) {
            mathModeEnabled = !mathModeEnabled;
            updateTrayIcon();
        }
    }

    if (msg == WM_DESTROY) {
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostQuitMessage(0);
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

// Keyboard hook callback
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vkCode = kbStruct->vkCode;

        // Toggle shortcut: Ctrl+Alt+M
        bool ctrlPressed = GetAsyncKeyState(VK_CONTROL) & 0x8000;
        bool altPressed = GetAsyncKeyState(VK_MENU) & 0x8000;
        if (ctrlPressed && altPressed && vkCode == 'M') {
            mathModeEnabled = !mathModeEnabled;
            updateTrayIcon();
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }

        if (vkCode == VK_SPACE) {
            checkBuffer();
            buffer = "";
        }
        else if (vkCode == VK_BACK) {
            if (!buffer.empty()) buffer.pop_back();
        }
        else if (vkCode >= 'A' && vkCode <= 'Z') {
            char c = (char)(vkCode + 32);
            buffer += c;
        }
        else if (vkCode >= 0x30 && vkCode <= 0x39) {
    // Only buffer digit if Shift is NOT held
    bool shiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;
    if (!shiftPressed) {
        char c = (char)(vkCode);
        buffer += c;
    } else {
        buffer = "";
    }
}
else if (vkCode >= VK_NUMPAD0 && vkCode <= VK_NUMPAD9) {
    char c = '0' + (vkCode - VK_NUMPAD0);
    buffer += c;
}
        else {
            buffer = "";
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() {
    SetConsoleOutputCP(CP_UTF8);

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "MathKeyTray";
    RegisterClassEx(&wc);

    hwnd = CreateWindowEx(0, "MathKeyTray", "MathKey",
        0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lstrcpy(nid.szTip, "MathKey - ON (Ctrl+Alt+M to toggle)");
    Shell_NotifyIcon(NIM_ADD, &nid);

    HHOOK hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (hook == NULL) return 1;

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hook);
    return 0;
}