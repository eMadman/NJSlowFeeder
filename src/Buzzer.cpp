#include "Buzzer.h"

Buzzer::Buzzer(int pin, int channel)
    : buzzerPin(pin), buzzerChannel(channel) {}

void Buzzer::setup() {
    gpio_num_t buzzer_gpio = static_cast<gpio_num_t>(buzzerPin);
	if (rtc_gpio_is_valid_gpio(buzzer_gpio)) {
		rtc_gpio_hold_dis(buzzer_gpio);  
	}
    rtc_gpio_hold_dis((gpio_num_t)buzzerPin);  
    ledcAttachPin(buzzerPin, buzzerChannel);
    ledcSetup(buzzerChannel, 2000, 8); // 2kHz default, 8-bit resolution
}

void Buzzer::makeSound(int frequency, int duration) {
    ledcWriteTone(buzzerChannel, frequency); // Play tone
    delay(duration);                         // Wait
    ledcWriteTone(buzzerChannel, 0);         // Stop tone
}