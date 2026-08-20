#pragma once

// Common interface for Daisy Seed I/O. SerialHAL implements it over USB
// (dashboard control), HardwareHAL implements it over real GPIO/ADC —
// application code stays identical in both modes.
class DashboardHAL {
public:
    virtual ~DashboardHAL() = default;

    virtual void Init() = 0;
    virtual void Update() = 0;

    // Inputs
    virtual float GetAnalog(const char* ch) const = 0;        // 0.0-1.0
    virtual bool  GetButton(const char* ch) const = 0;        // true while held
    virtual bool  GetToggle(const char* ch) const = 0;        // flips on each press
    virtual int   GetEncoder(const char* ch) const = 0;       // accumulated total
    virtual int   GetEncoderDelta(const char* ch) = 0;        // steps since last Update()

    // Outputs
    virtual void SetLed(const char* ch, bool on) = 0;
    virtual void SetLcd(const char* ch, const char* text) = 0;
    virtual void SendAnalog(const char* ch, float value) = 0; // 0.0-1.0
};
