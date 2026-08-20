#include "daisy_seed.h"

// make TEST=1  -> serial mode (control from the dashboard)
// make         -> hardware mode (real pots/buttons)
#ifdef DASHBOARD_TEST
  #include "../hal/SerialHAL.h"
  using HAL = SerialHAL;
#else
  #include "../hal/HardwareHAL.h"
  using HAL = HardwareHAL;
#endif

using namespace daisy;

DaisySeed hw;
HAL       hal(hw);

int main() {
#ifdef DASHBOARD_TEST
    hal.RegisterEncoder("enc1");
#else
    // Pin numbers: https://electro-smith.github.io/libDaisy/index.html
    hal.RegisterAnalog("pot1",   hw.GetPin(21));
    hal.RegisterAnalog("pot2",   hw.GetPin(15));
    hal.RegisterButton("btn1",   hw.GetPin(28));
    hal.RegisterToggle("sw1",    hw.GetPin(27));
    hal.RegisterLed   ("led1",   hw.GetPin(22));
    hal.RegisterLed   ("led2",   hw.GetPin(23));
#endif

    hal.Init();

    while (true) {
        hal.Update();

        float pot   = hal.GetAnalog("pot1");
        bool  btn   = hal.GetButton("btn1");
        bool  sw    = hal.GetToggle("sw1");
        int   enc   = hal.GetEncoder("enc1");
        int   delta = hal.GetEncoderDelta("enc1");

        hal.SetLed("led1", btn);
        hal.SetLed("led2", sw);

        char buf[16];
        snprintf(buf, sizeof(buf), "%.2f", pot);
        hal.SetLcd("lcd1", buf);

        hal.SendAnalog("pot1_out", pot);
        if (delta != 0)
            hal.SendAnalog("enc1_out", (float)(enc % 128) / 127.f);

        System::Delay(10);
    }
}
