
/* --------------------- BEGIN SLEEP RELATED FUNCTIONS --------------------- */

void hibernate_mode() {
    // Tell Arduino we're turning off
    digitalWrite(ARDUINO_SIGNAL_READY_PIN, LOW);

    // Disable so even deeper sleep
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_SLOW_MEM, ESP_PD_OPTION_OFF);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_FAST_MEM, ESP_PD_OPTION_OFF);

    // Need to keep this on so that we can wake up using a pin interrupt
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    // Configure GPIO4 = RTC_GPIO_10 as ext0 wake up source for HIGH logic level
    // *IMPORTANT*: These are referred to by their actual GPIO pins, not the RTC_GPIO pin number
    rtc_gpio_pulldown_en(RTC_GPIO_WAKEUP_PIN);
    rtc_gpio_pullup_dis(RTC_GPIO_WAKEUP_PIN);
    esp_sleep_enable_ext0_wakeup(RTC_GPIO_WAKEUP_PIN,1);

    //Go to sleep now
    esp_deep_sleep_start();
}

//Function that prints the reason by which ESP32 has been awaken from sleep
void print_wakeup_reason(){
    esp_sleep_wakeup_cause_t wakeup_reason;
    wakeup_reason = esp_sleep_get_wakeup_cause();
    switch(wakeup_reason)
    {
        case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("[WAKE] Wakeup caused by external signal using RTC_IO"); break;
        case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("[WAKE] Wakeup caused by external signal using RTC_CNTL"); break;
        case ESP_SLEEP_WAKEUP_TIMER : Serial.println("[WAKE] Wakeup caused by timer"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("[WAKE] Wakeup caused by touchpad"); break;
        case ESP_SLEEP_WAKEUP_ULP : Serial.println("[WAKE] Wakeup caused by ULP program"); break;
        default : Serial.printf("[WAKE] Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
    }
}

static intr_handle_t s_timer_handle;
static void initialize_sleep_timer(unsigned int timer_interval_sec){
    timer_idx_t timer = TIMER_0;
    timer_group_t group = TIMER_GROUP_0;
    bool auto_reload = true;
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .alarm_en = TIMER_ALARM_EN,
        .counter_en = TIMER_PAUSE,
        .intr_type = TIMER_INTR_LEVEL,
        .counter_dir = TIMER_COUNT_UP,
        .auto_reload = auto_reload,
        .divider = TIMER_DIVIDER,
    }; // default clock source is APB
    timer_init(group, timer, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(group, timer, 0);

    /* Configure the alarm value and the interrupt on alarm. */
    timer_set_alarm_value(group, timer, timer_interval_sec * TIMER_SCALE);
    timer_enable_intr(group, timer);

    timer_isr_register(group, timer, &sleep_timer_isr_callback, NULL, 0, &s_timer_handle);

    timer_start(group, timer);
}


static void sleep_timer_isr_callback(void* args) {
    Serial.println("[INTERRUPT] A sleep timer interrupt has occurred.");
    
    // Reset the timers
    TIMERG0.int_clr_timers.t0 = 1;
    TIMERG0.hw_timer[0].config.alarm_en = 1;

    if(!digitalRead(RTC_GPIO_WAKEUP_PIN)) {
        // Go to sleep
        Serial.println("[INTERRUPT] Going to sleep.");
        hibernate_mode();
    }
}

/* --------------------- END SLEEP RELATED FUNCTIONS --------------------- */
