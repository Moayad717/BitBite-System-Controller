#pragma once

#include <Arduino.h>

// ============================================================================
// SERIAL OTA RECEIVER
// ============================================================================
// Receives firmware over Serial2 from the WiFi ESP and applies it via Update.h.
//
// Integration:
//   1. SerialProtocol dispatches "OTA_START:<size>:<crc>" to commandCallback_
//   2. onCommand() in main.cpp calls serialOTAReceiver.startOTA(size, crc)
//   3. main loop replaces serialProtocol.processIncoming() with
//      serialOTAReceiver.tick() while isReceiving() is true
//   4. Receiver applies firmware chunk by chunk, then reboots on success
//
// Protocol received from WiFi ESP:
//   OTA_START:<total_bytes>:<crc32>    → handled by SerialProtocol/onCommand
//   OTA_CHUNK:<seq>:<len>:<hexdata>    → handled by tick()
//   OTA_END                            → handled by tick()
//
// Protocol sent back to WiFi ESP:
//   OTA_READY                          → sent by startOTA()
//   OTA_ACK:<seq>                      → sent after each good chunk
//   OTA_NACK:<seq>                     → sent if chunk is bad (triggers retry)
//   OTA_OK                             → sent before reboot on success
//   OTA_ERROR:<reason>                 → sent on failure

class SerialOTAReceiver {
public:
    SerialOTAReceiver();

    // Called from onCommand() when OTA_START:<size>:<crc32> arrives.
    // Sends OTA_READY back and activates receiving mode.
    void startOTA(size_t totalSize, uint32_t expectedCRC);

    // Non-blocking — call from the main loop instead of serialProtocol.processIncoming()
    // while isReceiving() returns true.
    void tick();

    bool isReceiving() const { return receiving_; }

private:
    bool receiving_;
    bool updateBegun_;
    size_t totalSize_;
    uint32_t expectedCRC_;
    int expectedSeq_;

    // Line receive buffer
    // Max line: "OTA_CHUNK:" (10) + seq (4) + ":" + len (3) + ":" + 512 hex = ~531
    static const size_t LINE_BUF_SIZE = 700;
    char lineBuf_[LINE_BUF_SIZE];
    size_t lineIdx_;

    void processLine();
    void handleChunk(const char* line);
    void handleEnd();
    void abort(const char* reason);

    // Decodes uppercase hex string into buf, returns number of bytes written.
    size_t hexToBytes(const char* hex, size_t hexLen, uint8_t* buf);
};
