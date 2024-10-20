#pragma once

#include <Print.h>
#include <cstdio>

#define ANSI_PREFIX "\033["

#define ANSI_FG_DEFAULT "39"
#define ANSI_FG_BLACK "30"
#define ANSI_FG_RED "31"
#define ANSI_FG_GREEN "32"
#define ANSI_FG_YELLOW "33"
#define ANSI_FG_BLUE "34"
#define ANSI_FG_MAGENTA "35"
#define ANSI_FG_CYAN "36"
#define ANSI_FG_WHITE "37"
#define ANSI_FG_GREY "90"
#define ANSI_FG_BRIGHT_RED "91"
#define ANSI_FG_BRIGHT_GREEN "92"
#define ANSI_FG_BRIGHT_YELLOW "93"
#define ANSI_FG_BRIGHT_BLUE "94"
#define ANSI_FG_BRIGHT_MAGENTA "95"
#define ANSI_FG_BRIGHT_CYAN "96"
#define ANSI_FG_BRIGHT_WHITE "97"

#define ANSI_BG_DEFAULT "49"
#define ANSI_BG_BLACK "40"
#define ANSI_BG_RED "41"
#define ANSI_BG_GREEN "42"
#define ANSI_BG_YELLOW "43"
#define ANSI_BG_BLUE "44"
#define ANSI_BG_MAGENTA "45"
#define ANSI_BG_CYAN "46"
#define ANSI_BG_WHITE "47"
#define ANSI_BG_GREY "100"
#define ANSI_BG_BRIGHT_RED "101"
#define ANSI_BG_BRIGHT_GREEN "102"
#define ANSI_BG_BRIGHT_YELLOW "103"
#define ANSI_BG_BRIGHT_BLUE "104"
#define ANSI_BG_BRIGHT_MAGENTA "105"
#define ANSI_BG_BRIGHT_CYAN "106"
#define ANSI_BG_BRIGHT_WHITE "107"

#define ANSI_BOLD "1"
#define ANSI_DIM 2
#define ANSI_UNDERLINE 4
#define ANSI_BLINK 5
#define ANSI_INVERTED 7
#define ANSI_HIDDEN 8

#define ANSI_RESET_BOLD 21
#define ANSI_RESET_DIM 22
#define ANSI_RESET_UNDERLINE 24
#define ANSI_RESET_BLINK 25
#define ANSI_RESET_INVERTED 27
#define ANSI_RESET_HIDDEN 28

#define ANSI_CLS ANSI_PREFIX"1J"
#define ANSI_CLEAR_LINE ANSI_PREFIX"2K"
#define ANSI_CLEAR_EO_LINE ANSI_PREFIX"K"
#define ANSI_HOME ANSI_PREFIX"H"
#define ANSI_CURSOR_UP(x) ANSI_PREFIX #x "A"
#define ANSI_CURSOR_DOWN(x) ANSI_PREFIX #x "B"
#define ANSI_CURSOR_LEFT(y) ANSI_PREFIX #y "D"
#define ANSI_CURSOR_RIGHT(y) ANSI_PREFIX #y "C"
#define ANSI_CURSOR_SAVE ANSI_PREFIX"s"
#define ANSI_CURSOR_RESTORE ANSI_PREFIX"u"
#define ANSI_TEXT_STYLE(style) ANSI_PREFIX style "m"
#define ANSI_TEXT_STYLE2(style1, style2) ANSI_PREFIX style1 ";" style2 "m"
#define ANSI_TEXT_STYLE3(style1, style2, style3) ANSI_PREFIX #style1 ";" #style2 ";" #style3 "m"

class Ansi {
public:
    enum class Color {
        Black = 0,
        Red = 1,
        Green = 2,
        Yellow = 3,
        Blue = 4,
        Magenta = 5,
        Cyan = 6,
        White = 7,
        BrightBlack = 60,
        BrightRed = 61,
        BrightGreen = 62,
        BrightYellow = 63,
        BrightBlue = 64,
        BrightMagenta = 65,
        BrightCyan = 66,
        BrightWhite = 67,
    };

    static const char* cursorXY(int x, int y) {
        snprintf(ansiBuffer, sizeof(ansiBuffer), ANSI_PREFIX"%i;%iH", x, y);
        return ansiBuffer;
    }

    static const char* cursorUp(int x) {
        prefixAndNumberAndValue(x, 'A');
        return ansiBuffer;
    }

    static const char* cursorDown(int x) {
        prefixAndNumberAndValue(x, 'B');
        return ansiBuffer;
    }

    static const char* cursorRight(int x) {
        prefixAndNumberAndValue(x, 'C');
        return ansiBuffer;
    }

    static const char* cursorLeft(int x) {
        prefixAndNumberAndValue(x, 'D');
        return ansiBuffer;
    }

    static const char* setBG(Color color) {
        setAttribute((int)color + 40);
        return ansiBuffer;
    }

    static const char* setFG(Color color) {
        setAttribute((int)color + 30);
        return ansiBuffer;
    }

    static const char* bold(String str) {
        setAttribute(1);
        return ansiBuffer;
    }

    static const char* blink(String str) {
        setAttribute(5);
        return ansiBuffer;
    }

    static const char* italic(String str) {
        setAttribute(3);
        return ansiBuffer;
    }

    static const char* underline(String str) {
        setAttribute(4);
        return ansiBuffer;
    }

    static const char* inverse(String str) {
        setAttribute(7);
        return ansiBuffer;
    }

    static const char* resetBold(String str) {
        setAttribute(1);
        return ansiBuffer;
    }

    static const char* resetBlink(String str) {
        setAttribute(5) + str + setAttribute(25);
        return ansiBuffer;
    }

    static const char* resetItalic(String str) {
        setAttribute(3) + str + setAttribute(23);
        return ansiBuffer;
    }

    static const char* resetUnderline(String str) {
        setAttribute(4) + str + setAttribute(24);
        return ansiBuffer;
    }

    static const char* resetInverse(String str) {
        setAttribute(7) + str + setAttribute(27);
        return ansiBuffer;
    }

    static const char* showCursor(bool blink) {
        snprintf(ansiBuffer, sizeof(ansiBuffer), ANSI_PREFIX"?25%c", blink ? 'h' : 'l');
        return ansiBuffer;
    }

    static const char* reset() {
        strncat(ansiBuffer, ANSI_PREFIX"m", sizeof(ansiBuffer) - 1);
        return ansiBuffer;
    }

    static void prefixAndNumberAndValue(int x, char v) {
        snprintf(ansiBuffer, sizeof(ansiBuffer), ANSI_PREFIX"%i%c", x, v);
    }

    static const char* setAttribute(int a) {
        prefixAndNumberAndValue(a, 'm');
        return ansiBuffer;
    }

private:
    static char ansiBuffer[32];
};
