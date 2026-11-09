zephyr_include_directories(include)

target_sources_ifdef(CONFIG_ZMK_BUZZER app PRIVATE src/buzzer.c)
target_sources_ifdef(CONFIG_ZMK_BEHAVIOR_BUZZER app PRIVATE src/behaviors/behavior_buzzer.c)
target_sources_ifdef(CONFIG_ZMK_BUZZER_TRIGGERS app PRIVATE src/buzzer_triggers.c)
target_sources_ifdef(CONFIG_ZMK_BUZZER_TEST_PWM app PRIVATE src/test/test_pwm.c)
