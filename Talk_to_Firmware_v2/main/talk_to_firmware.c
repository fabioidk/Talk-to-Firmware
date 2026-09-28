#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_partition.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "led_strip.h"
#include "led_strip_rmt.h"
#include "nvs.h"
#include "nvs_flash.h"

#define LED_PIN GPIO_NUM_8
#define KEY_PIN GPIO_NUM_9
#define AUX_PIN GPIO_NUM_10
#define STATE_OFFSET 0xF0000
#define UNIT_MS 120
#define POLL_MS 10
#define STABLE_TICKS 4

static const gpio_num_t pins[] = {
    GPIO_NUM_0, GPIO_NUM_1, GPIO_NUM_3, GPIO_NUM_4,
    GPIO_NUM_5, GPIO_NUM_6, GPIO_NUM_7,
};

volatile uint8_t probe[4];
volatile uint32_t epoch;

static const uint8_t payload_a[] = {
    0x6D, 0x6E, 0x16, 0x11, 0x7A, 0x6D, 0x6A, 0x7A,
    0x1C, 0x6B, 0x08, 0x17, 0x0D, 0x6E, 0x08, 0x69
};

static const uint8_t payload_b[] = {
    0x6E, 0x14, 0x1E, 0x7A, 0x1C, 0x6B, 0x08, 0x17,
    0x0D, 0x6E, 0x08, 0x69, 0x7A, 0x0D, 0x6B, 0x16,
    0x16, 0x7A, 0x6D, 0x6E, 0x16, 0x11, 0x7A, 0x6D,
    0x6A, 0x7A, 0x03, 0x6A, 0x0F
};

static const char *const alphabet[] = {
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....",
    "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.",
    "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-",
    "-.--", "--..", "-----", ".----", "..---", "...--", "....-",
    ".....", "-....", "--...", "---..", "----."
};

static const char symbols[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
static led_strip_handle_t led;

static void banner(void)
{
    fputs(
        "\n\033[1;36m"
        "+--------------------------------------------------+\n"
        "|                  TALK // UNL0CK                  |\n"
        "|            ESP32-C3 FIRMWARE CHALLENGE           |\n"
        "+--------------------------------------------------+\n"
        "|  Listen to the board. Inspect what changes.      |\n"
        "|  Alter what remains.                             |\n"
        "+--------------------------------------------------+\n"
        "\033[0m",
        stdout
    );
    fflush(stdout);
}

static void wait_units(unsigned units)
{
    vTaskDelay(pdMS_TO_TICKS(UNIT_MS * units));
}

static void color(uint8_t r, uint8_t g, uint8_t b)
{
    ESP_ERROR_CHECK(led_strip_set_pixel(led, 0, r, g, b));
    ESP_ERROR_CHECK(led_strip_refresh(led));
}

static void dark(void)
{
    ESP_ERROR_CHECK(led_strip_clear(led));
}

static const char *lookup(char value)
{
    const char *p = strchr(symbols, value);
    return p == NULL ? NULL : alphabet[p - symbols];
}

static void pulse(const char *pattern, uint8_t r, uint8_t g, uint8_t b)
{
    for (size_t i = 0; pattern[i] != '\0'; ++i) {
        putchar(pattern[i]);
        putchar('\a');
        fflush(stdout);
        color(r, g, b);
        wait_units(pattern[i] == '.' ? 1 : 3);
        dark();
        wait_units(1);
    }
}

static void transmit(const uint8_t *data, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        char value = (char)(data[i] ^ 0x5A);
        if (value == ' ') {
            fputs("/ ", stdout);
            fflush(stdout);
            continue;
        }

        const char *pattern = lookup(value);
        if (pattern == NULL) {
            continue;
        }

        pulse(pattern, 0, 20, 0);
        putchar(' ');
        fflush(stdout);
        if (i + 1 < length) {
            wait_units(((char)(data[i + 1] ^ 0x5A) == ' ') ? 6 : 2);
        }
    }
    putchar('\n');
    fflush(stdout);
}

static void reject(void)
{
    pulse("...", 20, 0, 0);
    putchar(' ');
    wait_units(2);
    pulse("---", 20, 0, 0);
    putchar(' ');
    wait_units(2);
    pulse("...", 20, 0, 0);
    putchar('\n');
    fflush(stdout);
    dark();
}

static void setup_led(void)
{
    led_strip_config_t a = {
        .strip_gpio_num = LED_PIN,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags.invert_out = false,
    };
    led_strip_rmt_config_t b = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000,
        .mem_block_symbols = 0,
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&a, &b, &led));
    dark();
}

static void setup_inputs(void)
{
    uint64_t mask = 1ULL << AUX_PIN;
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        mask |= 1ULL << pins[i];
    }

    gpio_config_t a = {
        .pin_bit_mask = mask,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&a));

    gpio_config_t b = {
        .pin_bit_mask = 1ULL << KEY_PIN,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&b));
}

static void shuffle(void)
{
    uint8_t old[4];
    bool repeat = epoch != 0;
    memcpy(old, (const void *)probe, sizeof(old));

    do {
        uint8_t pool[sizeof(pins) / sizeof(pins[0])];
        for (size_t i = 0; i < sizeof(pool); ++i) {
            pool[i] = (uint8_t)pins[i];
        }
        for (size_t i = 0; i < 4; ++i) {
            size_t pick = i + esp_random() % (sizeof(pool) - i);
            uint8_t tmp = pool[i];
            pool[i] = pool[pick];
            pool[pick] = tmp;
            probe[i] = pool[i];
        }
    } while (repeat && memcmp(old, (const void *)probe, sizeof(old)) == 0);

    ++epoch;
}

static int contact(void)
{
    int found = -1;
    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); ++i) {
        if (gpio_get_level(pins[i]) == 0) {
            if (found >= 0) {
                return -2;
            }
            found = pins[i];
        }
    }
    return found;
}

static void released(void)
{
    while (contact() != -1 || gpio_get_level(KEY_PIN) == 0) {
        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
    }
    vTaskDelay(pdMS_TO_TICKS(50));
}

static bool phase(void)
{
    nvs_handle_t h;
    uint8_t value = 0;
    if (nvs_open("a", NVS_READONLY, &h) != ESP_OK) {
        return false;
    }
    esp_err_t result = nvs_get_u8(h, "b", &value);
    nvs_close(h);
    return result == ESP_OK && value == 1;
}

static void store(void)
{
    nvs_handle_t h;
    ESP_ERROR_CHECK(nvs_open("a", NVS_READWRITE, &h));
    ESP_ERROR_CHECK(nvs_set_u8(h, "b", 1));
    ESP_ERROR_CHECK(nvs_commit(h));
    nvs_close(h);
}

static bool state(void)
{
    uint8_t value[6];
    const esp_partition_t *app = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
    if (app == NULL || esp_partition_read(app, STATE_OFFSET, value, sizeof(value)) != ESP_OK) {
        return false;
    }
    return value[0] == 0x55 && value[1] == 0x4E && value[2] == 0x4C &&
           value[3] == 0x4F && value[4] == 0x43 && value[5] == 0x4B;
}

static bool match(const uint8_t answer[4], size_t count, bool invalid)
{
    return !invalid && count == 4 &&
           memcmp(answer, (const void *)probe, sizeof(probe)) == 0;
}

static void mode_a(void)
{
    uint8_t answer[4] = {0};
    size_t count = 0;
    int active = -1;
    bool invalid = false;
    int candidate = -1;
    int stable = -1;
    unsigned ticks = 0;
    int key_candidate = 1;
    int key_stable = 1;
    unsigned key_ticks = 0;

    shuffle();
    released();

    while (true) {
        int raw = contact();
        if (raw == candidate) {
            if (ticks < STABLE_TICKS) {
                ++ticks;
            }
        } else {
            candidate = raw;
            ticks = 1;
        }

        if (ticks == STABLE_TICKS && stable != candidate) {
            stable = candidate;
            if (stable == -2) {
                invalid = true;
                active = -1;
            } else if (stable >= 0) {
                if (active == -1) {
                    active = stable;
                } else if (active != stable) {
                    invalid = true;
                }
            } else if (active >= 0) {
                if (count < 4) {
                    answer[count++] = (uint8_t)active;
                } else {
                    invalid = true;
                }
                active = -1;
            }
        }

        int raw_key = gpio_get_level(KEY_PIN);
        if (raw_key == key_candidate) {
            if (key_ticks < STABLE_TICKS) {
                ++key_ticks;
            }
        } else {
            key_candidate = raw_key;
            key_ticks = 1;
        }

        if (key_ticks == STABLE_TICKS && key_stable != key_candidate) {
            key_stable = key_candidate;
            if (key_stable == 0) {
                if (match(answer, count, invalid) && active == -1 && raw == -1 && stable == -1) {
                    store();
                    transmit(payload_a, sizeof(payload_a));
                    dark();
                    while (true) {
                        vTaskDelay(pdMS_TO_TICKS(1000));
                    }
                }

                reject();
                shuffle();
                count = 0;
                active = -1;
                invalid = false;
                released();
                candidate = stable = -1;
                ticks = 0;
                key_candidate = key_stable = 1;
                key_ticks = 0;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(POLL_MS));
    }
}

static void mode_b(void)
{
    if (gpio_get_level(AUX_PIN) != 0 || !state()) {
        reject();
        while (true) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    color(0, 0, 20);
    vTaskDelay(pdMS_TO_TICKS(500));
    dark();
    vTaskDelay(pdMS_TO_TICKS(500));
    transmit(payload_b, sizeof(payload_b));
    color(0, 20, 0);

    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    setup_led();
    setup_inputs();
    banner();

    if (phase()) {
        mode_b();
    } else {
        mode_a();
    }
}