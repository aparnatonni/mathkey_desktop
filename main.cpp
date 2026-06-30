#include <iostream>
#include <windows.h>
#include <string>
#include <map>

// Buffer to store what the user is typing
std::string buffer = "";

// MathKey symbol mappings
std::map<std::string, std::string> mathSymbols = {
    // Greek letters
    {"alpha", "α"},
    {"beta", "β"},
    {"gamma", "γ"},
    {"delta", "δ"},
    {"epsilon", "ε"},
    {"theta", "θ"},
    {"lambda", "λ"},
    {"mu", "μ"},
    {"pi", "π"},
    {"sigma", "σ"},
    {"phi", "φ"},
    {"omega", "ω"},

    // Operations
    {"plus", "+"},
    {"minus", "-"},
    {"times", "×"},
    {"equal", "="},
    {"leq", "≤"},
    {"geq", "≥"},
    {"neq", "≠"},
    {"approx", "≈"},
    {"infinity", "∞"},
    {"pm", "±"},

    // Calculus
    {"integral", "∫"},
    {"partial", "∂"},
    {"nabla", "∇"},
    {"sum", "∑"},

    // Sets
    {"union", "∪"},
    {"intersect", "∩"},
    {"subset", "⊂"},
    {"in", "∈"},
    {"forall", "∀"},
    {"exists", "∃"},

    // Arrows
    {"to", "→"},
    {"implies", "⇒"},
    {"iff", "⇔"},
};

// Delete n characters before cursor
void deleteChars(int n) {
    for (int i = 0; i < n; i++) {
        // Simulate pressing Backspace
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = VK_BACK;
        SendInput(1, &input, sizeof(INPUT));

        // Release Backspace
        input.ki.dwFlags = KEYEVENTF_KEYUP;
        SendInput(1, &input, sizeof(INPUT));
    }
}

// Type a unicode string into the active window
void typeUnicode(const std::string& text) {
    // Convert UTF-8 string to wide string
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

// Check if buffer matches a MathKey command
void checkBuffer() {
    auto it = mathSymbols.find(buffer);
    if (it != mathSymbols.end()) {
        std::string symbol = it->second;
        std::cout << "Match found: " << buffer << " → " << symbol << std::endl;

        // Delete the typed word + space
        deleteChars(buffer.length() + 1);

        // Small delay to let deletions process
        Sleep(50);

        // Type the symbol
        typeUnicode(symbol);
    }
}

// Keyboard hook callback
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vkCode = kbStruct->vkCode;

        if (vkCode == VK_SPACE) {
            // Space pressed — check buffer
            std::cout << "Buffer: " << buffer << std::endl;
            checkBuffer();
            buffer = "";
        }
        else if (vkCode == VK_BACK) {
            // Backspace — remove last char from buffer
            if (!buffer.empty()) {
                buffer.pop_back();
            }
        }
        else if (vkCode >= 'A' && vkCode <= 'Z') {
            // Letter key — add lowercase to buffer
            char c = (char)(vkCode + 32);
            buffer += c;
        }
        else {
            // Any other key — reset buffer
            buffer = "";
        }
    }

    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

int main() {
    std::cout << "MathKey Desktop - Active!" << std::endl;
    std::cout << "Type a math word followed by Space to convert." << std::endl;
    std::cout << "Examples: alpha, beta, pi, integral, union" << std::endl;
    std::cout << "Press Ctrl+C to exit." << std::endl;
    std::cout << "----------------------------------------" << std::endl;

    // Set console to UTF-8
    SetConsoleOutputCP(CP_UTF8);

    HHOOK hook = SetWindowsHookEx(
        WH_KEYBOARD_LL,
        KeyboardProc,
        NULL,
        0
    );

    if (hook == NULL) {
        std::cout << "Failed to install hook!" << std::endl;
        return 1;
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    UnhookWindowsHookEx(hook);
    return 0;
}