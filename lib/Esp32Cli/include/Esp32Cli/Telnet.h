#pragma once

#if ESP32_CLI_ENABLE_TELNET

#include <cstdint>

enum class Telnet : uint8_t {
    /**
     * End of file.
     */
    EOF_ = 0xec,

    /**
     * End of subnegotiation parameters.
     */
    SE = 240,

    /**
     * No operation
     */
    NOP = 241,

    /**
     * Data mark. Indicates the position of
       * a Synch event within the data stream. This
       * should always be accompanied by a TCP
       * urgent notification.
     */
    DM = 242,

    /**
     * Break. Indicates that the "break"
       * or "attention" key was hit.
     */
    BRK = 243,

    /**
     * Suspend, interrupt or abort the process
       * to which the NVT is connected.
     */
    IP = 244,

    /**
     * Abort output. Allows the current process
       * to run to completion but do not send
       * its output to the user.
     */
    AO = 245,

    /**
     * Are you there? Send back to the NVT some
       * visible evidence that the AYT was received.
     */
    AYT = 246,

    /**
     * Erase character. The receiver should delete
       * the last preceding undeleted
       * character from the data stream.
     */
    EC = 247,

    /**
     * Erase line. Delete characters from the data
       * stream back to but not including the previous CRLF.
     */
    EL = 248,

    /**
     * Go ahead. Used, under certain circumstances,

       * to tell the other end that it can transmit.
     */
    GA = 249,

    /**
     * Subnegotiation of the indicated option follows.
     */
    SB = 250,

    /**
     * Indicates the desire to begin
       * performing, or confirmation that you are
       * now performing, the indicated option.
     */
    WILL = 251,

    /**
     * Indicates the refusal to perform, or
       * continue performing, the indicated option.
     */
    WONT = 252,

    /**
     * Indicates the request that the other
       * party perform, or confirmation that you are
       * expecting the other party to
       * perform, the indicated option.
     */
    DO = 253,

    /**
     * Indicates the demand that the other
       * party stop performing, or confirmation that you
       * are no longer expecting the other party to
       * perform, the indicated option.
     */
    DONT = 254,

    /**
     * Interpret as command
     */
    IAC = 255,
};

bool operator==(uint8_t v, Telnet c);

enum class TelnetOption : uint8_t {
    Echo = 1,
    SuppressGoAhead = 3,
    Status = 5,
    TimingMark = 6,
    TerminalType = 24,
    WindowSize = 31,
    TerminalSpeed = 32,
    RemoteFlowControl = 33,
    Linemode = 34,
    EnvironmentVariables = 35,
};

void writeTelnetCommand(Telnet, class Stream&);

void writeTelnetOption(TelnetOption, class Stream&);

void writeTelnetResponse(Telnet, TelnetOption, class Stream&);

const char* telnetCommandToString(Telnet);

const char* telnetCommandToString(uint8_t c);

const char* telnetOptionToString(TelnetOption);

const char* telnetOptionToString(uint8_t c);

#endif
