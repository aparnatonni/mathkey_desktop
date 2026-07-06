#include <windows.h>
#include <string>
#include <map>
#include <unordered_map>
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
    // Greek lowercase
    {"alpha", "α"}, {"beta", "β"}, {"gamma", "γ"},
    {"delta", "δ"}, {"epsilon", "ε"}, {"theta", "θ"},
    {"lambda", "λ"}, {"mu", "μ"}, {"pi", "π"},
    {"sigma", "σ"}, {"phi", "φ"}, {"omega", "ω"},
    {"eta", "η"}, {"zeta", "ζ"}, {"kappa", "κ"},
    {"nu", "ν"}, {"xi", "ξ"}, {"rho", "ρ"},
    {"tau", "τ"}, {"upsilon", "υ"}, {"chi", "χ"},
    {"psi", "ψ"},

    // Greek uppercase (cap prefix)
    {"capsigma", "Σ"}, {"capdelta", "Δ"}, {"capomega", "Ω"}, {"cappi", "Π"},
    {"capgamma", "Γ"}, {"caplambda", "Λ"}, {"captheta", "Θ"}, {"capphi", "Φ"},
    {"capxi", "Ξ"}, {"cappsi", "Ψ"}, {"capupsilon", "Υ"},

    // Operations
    {"plus", "+"}, {"minus", "-"}, {"times", "×"},
    {"equal", "="}, {"leq", "≤"}, {"geq", "≥"},
    {"neq", "≠"}, {"approx", "≈"}, {"infinity", "∞"},
    {"pm", "±"}, {"div", "÷"}, {"cdot", "·"},
    {"equiv", "≡"}, {"propto", "∝"}, {"sim", "∼"},

    // Powers (standalone)
    {"square", "²"}, {"squared", "²"},
    {"cube", "³"}, {"cubed", "³"},
    {"inverse", "⁻¹"},

    // Roots
    {"sqrt", "√"}, {"cbrt", "∛"},

    // Degree and angle
    {"deg", "°"}, {"angle", "∠"}, {"perp", "⊥"},
    {"parallel", "∥"},

    // Calculus
    {"integral", "∫"}, {"iint", "∬"}, {"iiint", "∭"},
    {"oint", "∮"}, {"partial", "∂"},
    {"nabla", "∇"}, {"sum", "∑"}, {"prod", "∏"},
    {"lim", "lim"},

    // Sets
    {"union", "∪"}, {"intersect", "∩"},
    {"subset", "⊂"}, {"supset", "⊃"},
    {"subseteq", "⊆"}, {"supseteq", "⊇"},
    {"in", "∈"}, {"notin", "∉"},
    {"forall", "∀"}, {"exists", "∃"},
    {"nexists", "∄"}, {"empty", "∅"},

    // Number sets
    {"real", "ℝ"}, {"integer", "ℤ"}, {"natural", "ℕ"},
    {"rational", "ℚ"}, {"complex", "ℂ"},

    // Logic
    {"and", "∧"}, {"or", "∨"}, {"not", "¬"},
    {"xor", "⊕"}, {"therefore", "∴"}, {"because", "∵"},

    // Arrows
    {"to", "→"}, {"from", "←"}, {"implies", "⇒"},
    {"iff", "⇔"}, {"uparrow", "↑"}, {"downarrow", "↓"},
    {"mapsto", "↦"}, {"leftrightarrow", "↔"},

    // Trig inverse
    {"arcsin", "sin⁻¹"}, {"arccos", "cos⁻¹"}, {"arctan", "tan⁻¹"},

    // Floor and ceiling
    {"floor", "⌊⌋"}, {"ceil", "⌈⌉"},

    // Misc
    {"inf", "∞"}, {"dots", "…"}, {"cdots", "⋯"},
    {"vdots", "⋮"}, {"ddots", "⋱"},
    {"aleph", "ℵ"}, {"hbar", "ℏ"},
    {"dagger", "†"}, {"star", "★"},
    {"checkmark", "✓"}, {"cross", "✗"},
};

// Prefix to script type mapping
std::unordered_map<std::string, std::string> prefixToScript = {
    {"power", "superscript"},
    {"sub",   "subscript"},
    {"root",  "root"},
    {"log",   "log"},
};

// Nested script map
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

// Split buffer into text prefix and number/letter suffix
std::pair<std::string, std::string> splitBuffer(const std::string& buf) {
    int i = buf.length() - 1;
    while (i >= 0 && isdigit(buf[i])) {
        i--;
    }
    std::string prefix = buf.substr(0, i + 1);
    std::string suffix = buf.substr(i + 1);
    return {prefix, suffix};
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

// Check buffer and convert
void checkBuffer() {
    if (buffer.empty() || !mathModeEnabled) return;

    std::string result = "";

    // Split buffer into prefix and suffix
    auto [p, q] = splitBuffer(buffer);

    // Check prefix map
    auto pit = prefixToScript.find(p);
    if (pit != prefixToScript.end() && !q.empty()) {
        std::string scriptType = pit->second;

        if (scriptType == "root") {
            result = convertScript("superscript", q) + "√";
        }
        else if (scriptType == "log") {
            result = "log" + convertScript("subscript", q);
        }
        else {
            result = convertScript(scriptType, q);
        }
    }
    else {
        // Check single word map
        auto it = mathSymbols.find(buffer);
        if (it != mathSymbols.end()) {
            result = it->second;
        }
    }

    if (!result.empty()) {
        deleteChars(buffer.length() + 1);
        Sleep(50);
        typeUnicode(result);
    } else {
        deleteChars(buffer.length() + 1);
        Sleep(50);
        typeUnicode(buffer);
    }
}

// Update tray icon tooltip
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

        // Ignore injected keystrokes
        if (kbStruct->flags & LLKHF_INJECTED) {
            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }

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
            bool shiftPressed = GetAsyncKeyState(VK_SHIFT) & 0x8000;
            if (!shiftPressed) {
                char c = (char)(vkCode);
                buffer += c;
            } else {
                if (buffer.empty()) {
                    Sleep(30);
                    deleteChars(1);
                }
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