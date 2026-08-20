#pragma once
#include "DashboardHAL.h"
#include "daisy_seed.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>

// USB-CDC serial implementation. Protocol: "channel:value\n" in both directions.
// Channels registered via RegisterEncoder() accumulate incoming values as deltas
// instead of treating them as absolute positions.
class SerialHAL : public DashboardHAL {
public:
    static constexpr int MAX_ENCODER_CHANNELS = 8;
    static constexpr int RX_BUF  = 512;
    static constexpr int MAX_KV  = 32;
    static constexpr int KEY_LEN = 24;
    static constexpr int VAL_LEN = 24;

    explicit SerialHAL(daisy::DaisySeed& hw) : hw_(hw) {}

    void RegisterEncoder(const char* ch) {
        if (enc_count_ < MAX_ENCODER_CHANNELS)
            strncpy(enc_names_[enc_count_++], ch, KEY_LEN - 1);
    }

    // Use when hw.Init() was already called elsewhere (e.g. DaisyPod::Init()).
    void Attach() {
        instance_ = this;
        hw_.StartLog(false);
        hw_.usb_handle.SetReceiveCallback(
            SerialHAL::UsbReceive,
            daisy::UsbHandle::FS_INTERNAL
        );
    }

    void Init() override {
        hw_.Init();
        Attach();
    }

    void Update() override {
        while (rx_tail_ != rx_head_) {
            char c = rx_buf_[rx_tail_];
            rx_tail_ = (rx_tail_ + 1) % RX_BUF;
            if (c == '\n' || c == '\r') {
                if (line_pos_ > 0) {
                    line_buf_[line_pos_] = '\0';
                    ParseLine(line_buf_);
                    line_pos_ = 0;
                }
            } else if (line_pos_ < (int)sizeof(line_buf_) - 1) {
                line_buf_[line_pos_++] = c;
            }
        }
        for (int i = 0; i < kv_count_; i++) kv_[i].delta = 0;
    }

    float GetAnalog(const char* ch) const override {
        const KV* e = Find(ch);
        if (!e) return 0.f;
        float v = (float)atof(e->val) / 127.f;
        if (v < 0.f) v = 0.f;
        if (v > 1.f) v = 1.f;
        return v;
    }

    bool GetButton(const char* ch) const override {
        const KV* e = Find(ch);
        return e && e->val[0] != '0' && e->val[0] != '\0';
    }

    bool GetToggle(const char* ch) const override { return GetButton(ch); }

    int GetEncoder(const char* ch) const override {
        const KV* e = Find(ch);
        return e ? e->accum : 0;
    }

    int GetEncoderDelta(const char* ch) override {
        const KV* e = Find(ch);
        return e ? e->delta : 0;
    }

    void SetLed(const char* ch, bool on) override {
        hw_.PrintLine("%s:%d", ch, on ? 1 : 0);
    }

    void SetLcd(const char* ch, const char* text) override {
        hw_.PrintLine("%s:%s", ch, text);
    }

    void SendAnalog(const char* ch, float value) override {
        int v = (int)(value * 127.f);
        if (v < 0) v = 0;
        if (v > 127) v = 127;
        hw_.PrintLine("%s:%d", ch, v);
    }

private:
    struct KV {
        char key[KEY_LEN];
        char val[VAL_LEN];
        int  accum;
        int  delta;
    };

    daisy::DaisySeed& hw_;

    volatile int rx_head_ = 0;
    int          rx_tail_ = 0;
    char         rx_buf_[RX_BUF] = {};

    char line_buf_[64] = {};
    int  line_pos_     = 0;

    KV  kv_[MAX_KV]  = {};
    int kv_count_    = 0;

    char enc_names_[MAX_ENCODER_CHANNELS][KEY_LEN] = {};
    int  enc_count_ = 0;

    static SerialHAL* instance_;

    // Runs in USB interrupt context — keep it short.
    static void UsbReceive(uint8_t* buf, uint32_t* len) {
        if (!instance_) return;
        for (uint32_t i = 0; i < *len; i++) {
            int next = (instance_->rx_head_ + 1) % RX_BUF;
            if (next != instance_->rx_tail_) {
                instance_->rx_buf_[instance_->rx_head_] = (char)buf[i];
                instance_->rx_head_ = next;
            }
        }
    }

    bool IsEncoder(const char* key) const {
        for (int i = 0; i < enc_count_; i++)
            if (strcmp(enc_names_[i], key) == 0) return true;
        return false;
    }

    KV* Find(const char* key) {
        for (int i = 0; i < kv_count_; i++)
            if (strcmp(kv_[i].key, key) == 0) return &kv_[i];
        return nullptr;
    }

    const KV* Find(const char* key) const {
        for (int i = 0; i < kv_count_; i++)
            if (strcmp(kv_[i].key, key) == 0) return &kv_[i];
        return nullptr;
    }

    void ParseLine(const char* line) {
        const char* colon = strchr(line, ':');
        if (!colon || colon == line) return;

        char key[KEY_LEN] = {};
        char val[VAL_LEN] = {};
        int klen = (int)(colon - line);
        if (klen >= KEY_LEN) klen = KEY_LEN - 1;
        strncpy(key, line, klen);
        strncpy(val, colon + 1, VAL_LEN - 1);

        KV* e = Find(key);
        if (!e) {
            if (kv_count_ >= MAX_KV) return;
            e = &kv_[kv_count_++];
            strncpy(e->key, key, KEY_LEN - 1);
            e->accum = 0;
            e->delta = 0;
        }
        strncpy(e->val, val, VAL_LEN - 1);

        if (IsEncoder(key)) {
            int d = atoi(val);
            e->accum += d;
            e->delta += d;
            // Keep the accumulated total in val so GetAnalog/GetButton read it too.
            snprintf(e->val, VAL_LEN, "%d", e->accum);
        }
    }
};

SerialHAL* SerialHAL::instance_ = nullptr;
