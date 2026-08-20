#pragma once
#include "DashboardHAL.h"
#include "daisy_seed.h"
#include <cstring>
#include <cstdio>

// GPIO/ADC implementation. Register channels before calling Init():
//   hal.RegisterAnalog("pot1", seed.GetPin(21));
//   hal.RegisterButton("btn1", seed.GetPin(28));
class HardwareHAL : public DashboardHAL {
public:
    static constexpr int MAX_ANALOG  = 12;
    static constexpr int MAX_DIGITAL = 16;
    static constexpr int MAX_LED     = 8;
    static constexpr int CH_LEN      = 24;

    explicit HardwareHAL(daisy::DaisySeed& hw) : hw_(hw) {}

    void RegisterAnalog(const char* ch, daisy::Pin pin) {
        if (analog_count_ >= MAX_ANALOG) return;
        auto& a = analogs_[analog_count_++];
        strncpy(a.name, ch, CH_LEN - 1);
        a.pin = pin;
    }

    void RegisterButton(const char* ch, daisy::Pin pin, bool activeLow = true) {
        RegisterDigital(ch, pin, activeLow, false);
    }

    void RegisterToggle(const char* ch, daisy::Pin pin, bool activeLow = true) {
        RegisterDigital(ch, pin, activeLow, true);
    }

    void RegisterLed(const char* ch, daisy::Pin pin) {
        if (led_count_ >= MAX_LED) return;
        auto& l = leds_[led_count_++];
        strncpy(l.name, ch, CH_LEN - 1);
        l.pin = pin;
    }

    void Init() override {
        hw_.Init();

        if (analog_count_ > 0) {
            daisy::AdcChannelConfig cfg[MAX_ANALOG];
            for (int i = 0; i < analog_count_; i++)
                cfg[i].InitSingle(analogs_[i].pin);
            adc_.Init(cfg, analog_count_);
            adc_.Start();
        }

        for (int i = 0; i < digital_count_; i++) {
            auto& d = digitals_[i];
            daisy::GPIO::Pull pull = d.activeLow
                ? daisy::GPIO::Pull::PULLUP
                : daisy::GPIO::Pull::PULLDOWN;
            d.gpio.Init(d.pin, daisy::GPIO::Mode::INPUT, pull);
        }

        for (int i = 0; i < led_count_; i++)
            leds_[i].gpio.Init(leds_[i].pin, daisy::GPIO::Mode::OUTPUT);
    }

    void Update() override {
        for (int i = 0; i < digital_count_; i++) {
            auto& d = digitals_[i];
            bool raw     = d.gpio.Read();
            bool pressed = d.activeLow ? !raw : raw;

            if (d.isToggle) {
                if (pressed && !d.prev) d.state = !d.state;
            } else {
                d.state = pressed;
            }
            d.prev = pressed;
        }
    }

    float GetAnalog(const char* ch) const override {
        for (int i = 0; i < analog_count_; i++)
            if (strcmp(analogs_[i].name, ch) == 0)
                return adc_.GetFloat(i);
        return 0.f;
    }

    bool GetButton(const char* ch) const override {
        const Digital* d = FindDigital(ch);
        return d && !d->isToggle && d->state;
    }

    bool GetToggle(const char* ch) const override {
        const Digital* d = FindDigital(ch);
        return d && d->isToggle && d->state;
    }

    // Not implemented — use libDaisy's Encoder class directly for hardware mode.
    int GetEncoder(const char* ch) const override { (void)ch; return 0; }
    int GetEncoderDelta(const char* ch) override  { (void)ch; return 0; }

    void SetLed(const char* ch, bool on) override {
        for (int i = 0; i < led_count_; i++)
            if (strcmp(leds_[i].name, ch) == 0) { leds_[i].gpio.Write(on); return; }
    }

    // No display driver wired up here — implement with your own.
    void SetLcd(const char* ch, const char* text) override { (void)ch; (void)text; }

    void SendAnalog(const char* ch, float value) override {
        int v = (int)(value * 127.f);
        if (v < 0) v = 0;
        if (v > 127) v = 127;
        hw_.PrintLine("%s:%d", ch, v);
    }

private:
    struct Analog {
        char       name[CH_LEN];
        daisy::Pin pin;
    };

    struct Digital {
        char        name[CH_LEN];
        daisy::GPIO gpio;
        daisy::Pin  pin;
        bool        activeLow;
        bool        isToggle;
        bool        state;
        bool        prev;
    };

    struct Led {
        char        name[CH_LEN];
        daisy::GPIO gpio;
        daisy::Pin  pin;
    };

    daisy::DaisySeed& hw_;
    daisy::AdcHandle  adc_;

    Analog  analogs_[MAX_ANALOG]   = {};
    Digital digitals_[MAX_DIGITAL] = {};
    Led     leds_[MAX_LED]         = {};

    int analog_count_  = 0;
    int digital_count_ = 0;
    int led_count_     = 0;

    void RegisterDigital(const char* ch, daisy::Pin pin, bool activeLow, bool isToggle) {
        if (digital_count_ >= MAX_DIGITAL) return;
        auto& d = digitals_[digital_count_++];
        strncpy(d.name, ch, CH_LEN - 1);
        d.pin       = pin;
        d.activeLow = activeLow;
        d.isToggle  = isToggle;
        d.state     = false;
        d.prev      = false;
    }

    const Digital* FindDigital(const char* ch) const {
        for (int i = 0; i < digital_count_; i++)
            if (strcmp(digitals_[i].name, ch) == 0) return &digitals_[i];
        return nullptr;
    }
};
