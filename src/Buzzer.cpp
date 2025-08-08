#include "Buzzer.h"
#include "driver/ledc.h"

Buzzer::Buzzer(int pin, int channel)
    : buzzerPin(pin), buzzerChannel(channel) {}

void Buzzer::setup() {
    rtc_gpio_hold_dis((gpio_num_t)buzzerPin);  
    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW);
    
    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 2000, // 2kHz default
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
    
    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .gpio_num = buzzerPin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = (ledc_channel_t)buzzerChannel,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ledc_channel);
}

void Buzzer::makeSound(int frequency, int duration) {
    // Update timer frequency
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = frequency,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);
    
    // Set duty to 50% (127 for 8-bit resolution)
    ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)buzzerChannel, 127);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)buzzerChannel);
    
    delay(duration);
    
    // Stop tone
    ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)buzzerChannel, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)buzzerChannel);
}