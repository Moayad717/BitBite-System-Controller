#include "SerialOTAReceiver.h"
#include <Update.h>

// ============================================================================
// CONSTRUCTOR
// ============================================================================

SerialOTAReceiver::SerialOTAReceiver()
    : receiving_(false), updateBegun_(false),
      totalSize_(0), expectedCRC_(0), expectedSeq_(0), lineIdx_(0) {}

// ============================================================================
// START OTA — called from onCommand() in main.cpp
// ============================================================================

void SerialOTAReceiver::startOTA(size_t totalSize, uint32_t expectedCRC) {
    Serial.printf("[OTA] Starting receive: %u bytes, CRC=0x%08X\n", totalSize, expectedCRC);

    totalSize_   = totalSize;
    expectedCRC_ = expectedCRC;
    expectedSeq_ = 0;
    lineIdx_     = 0;
    updateBegun_ = false;

    if (!Update.begin(totalSize_)) {
        Serial.printf("[OTA] Update.begin() failed — not enough OTA partition space\n");
        Serial2.println("OTA_ERROR:no_space");
        return;
    }

    updateBegun_ = true;
    receiving_   = true;

    // Flush any leftover bytes from normal protocol traffic
    while (Serial2.available()) Serial2.read();

    Serial2.println("OTA_READY");
    Serial.println("[OTA] Sent OTA_READY, waiting for chunks...");
}

// ============================================================================
// TICK — called from main loop while isReceiving()
// ============================================================================

void SerialOTAReceiver::tick() {
    while (Serial2.available()) {
        char c = (char)Serial2.read();

        if (c == '\n') {
            // Strip trailing \r
            if (lineIdx_ > 0 && lineBuf_[lineIdx_ - 1] == '\r') lineIdx_--;
            lineBuf_[lineIdx_] = '\0';

            if (lineIdx_ > 0) {
                processLine();
            }

            lineIdx_ = 0;
            return;  // One line per tick — keeps main loop responsive
        }

        if (c != '\r' && lineIdx_ < LINE_BUF_SIZE - 1) {
            lineBuf_[lineIdx_++] = c;
        } else if (lineIdx_ >= LINE_BUF_SIZE - 1) {
            // Line buffer overflow — drain and abort
            Serial.println("[OTA] Line overflow — aborting");
            abort("line_overflow");
            return;
        }
    }
}

// ============================================================================
// LINE DISPATCH
// ============================================================================

void SerialOTAReceiver::processLine() {
    if (strncmp(lineBuf_, "OTA_CHUNK:", 10) == 0) {
        handleChunk(lineBuf_);
    } else if (strcmp(lineBuf_, "OTA_END") == 0) {
        handleEnd();
    } else {
        Serial.printf("[OTA] Unexpected line during receive: '%s'\n", lineBuf_);
    }
}

// ============================================================================
// CHUNK HANDLER
// ============================================================================

void SerialOTAReceiver::handleChunk(const char* line) {
    // Format: OTA_CHUNK:<seq>:<len>:<hexdata>
    const char* p = line + 10;  // Skip "OTA_CHUNK:"

    // Parse seq
    char* end;
    int seq = (int)strtol(p, &end, 10);
    if (*end != ':') { abort("bad_seq"); return; }
    p = end + 1;

    // Parse len
    size_t len = (size_t)strtol(p, &end, 10);
    if (*end != ':') { abort("bad_len"); return; }
    p = end + 1;

    // Validate sequence number
    if (seq != expectedSeq_) {
        Serial.printf("[OTA] Seq mismatch: expected %d, got %d\n", expectedSeq_, seq);
        char nack[24];
        snprintf(nack, sizeof(nack), "OTA_NACK:%d", seq);
        Serial2.println(nack);
        return;
    }

    // Decode hex data
    size_t hexLen = strlen(p);
    if (hexLen != len * 2) {
        Serial.printf("[OTA] Hex length mismatch: expected %u, got %u\n", len * 2, hexLen);
        char nack[24];
        snprintf(nack, sizeof(nack), "OTA_NACK:%d", seq);
        Serial2.println(nack);
        return;
    }

    // Stack-allocate decode buffer (max chunk = 256 bytes)
    uint8_t buf[256];
    if (len > sizeof(buf)) { abort("chunk_too_large"); return; }
    size_t decoded = hexToBytes(p, hexLen, buf);

    // Write to OTA partition
    size_t written = Update.write(buf, decoded);
    if (written != decoded) {
        Serial.printf("[OTA] Write failed: wrote %u / %u bytes\n", written, decoded);
        abort("write_fail");
        return;
    }

    // Send ACK
    char ack[24];
    snprintf(ack, sizeof(ack), "OTA_ACK:%d", seq);
    Serial2.println(ack);
    expectedSeq_++;

    // Progress every 50 chunks
    if (expectedSeq_ % 50 == 0) {
        Serial.printf("[OTA] Progress: chunk %d (%u bytes)\n", expectedSeq_, expectedSeq_ * 256);
    }
}

// ============================================================================
// END HANDLER
// ============================================================================

void SerialOTAReceiver::handleEnd() {
    Serial.println("[OTA] OTA_END received — finalizing...");

    if (!Update.end()) {
        Serial.printf("[OTA] Update.end() failed: %s\n", Update.errorString());
        Serial2.println("OTA_ERROR:end_fail");
        receiving_ = false;
        updateBegun_ = false;
        return;
    }

    if (!Update.isFinished()) {
        Serial.println("[OTA] Update not finished (incomplete write?)");
        Serial2.println("OTA_ERROR:not_finished");
        receiving_ = false;
        updateBegun_ = false;
        return;
    }

    Serial.println("[OTA] Firmware verified — rebooting!");
    Serial2.println("OTA_OK");
    Serial2.flush();
    delay(500);
    ESP.restart();
}

// ============================================================================
// ABORT
// ============================================================================

void SerialOTAReceiver::abort(const char* reason) {
    Serial.printf("[OTA] Aborted: %s\n", reason);
    char msg[48];
    snprintf(msg, sizeof(msg), "OTA_ERROR:%s", reason);
    Serial2.println(msg);

    if (updateBegun_) {
        Update.abort();
        updateBegun_ = false;
    }
    receiving_ = false;
    lineIdx_ = 0;
}

// ============================================================================
// HEX DECODE
// ============================================================================

size_t SerialOTAReceiver::hexToBytes(const char* hex, size_t hexLen, uint8_t* buf) {
    auto nibble = [](char c) -> uint8_t {
        if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
        if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10);
        if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10);
        return 0;
    };

    size_t out = 0;
    for (size_t i = 0; i + 1 < hexLen; i += 2) {
        buf[out++] = (nibble(hex[i]) << 4) | nibble(hex[i + 1]);
    }
    return out;
}
