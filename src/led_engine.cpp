#include "led_engine.h"
#include "pin_config.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include <string.h>

CRGB leds[MAX_OUTPUTS][MAX_LEDS_PER_OUTPUT];
CRGB canvasLeds[MAX_OUTPUTS][MAX_LEDS_PER_OUTPUT];
int claimed_sms[NUM_CHANNELS];

static PIO led_pio = pio0;

static const uint output_pins[NUM_CHANNELS] = { 
    OUTPUT_0_PIN, 
    OUTPUT_1_PIN, 
    OUTPUT_2_PIN, 
    OUTPUT_3_PIN 
}; 

static const uint16_t ws2812_program_instructions[] = {
    0x6221, //  0: out    x, 1            side 0 [2]  
    0x1123, //  1: jmp    !x, 3           side 1 [1]  
    0x1400, //  2: jmp    0               side 1 [4]  
    0xa442  //  3: nop                    side 0 [4]  
};

static const struct pio_program ws2812_program = {
    .instructions = ws2812_program_instructions,
    .length = 4,
    .origin = -1,
};

static inline void local_ws2812_program_init(PIO pio, uint sm, uint offset, uint pin, float freq) {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + 0, offset + 3);
    
    sm_config_set_sideset(&c, 1, false, false); 
    
    sm_config_set_sideset_pins(&c, pin);
    sm_config_set_out_pins(&c, pin, 1);
    sm_config_set_set_pins(&c, pin, 1);
    
    sm_config_set_out_shift(&c, false, true, 24);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);
    
    float div = (float)clock_get_hz(clk_sys) / (freq * 10.0f);
    sm_config_set_clkdiv(&c, div);
    
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

void ledInit() {
    memset(leds, 0, sizeof(leds));
    memset(canvasLeds, 0, sizeof(canvasLeds));

    uint offset = pio_add_program(led_pio, &ws2812_program);

    for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
        // FIXED: Dynamically claims available state machines on boot to prevent conflicts
        claimed_sms[ch] = pio_claim_unused_sm(led_pio, true);
        local_ws2812_program_init(led_pio, claimed_sms[ch], offset, output_pins[ch], 800000.f);
    }
}

void showAll() {
    for (uint16_t led = 0; led < LEDS_PER_CHANNEL; led++) {
        for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++) {
            CRGB pixel = leds[ch][led];
            
            // Map output colors into GRB WS2812 sequence
            uint32_t grb = ((uint32_t)pixel.g << 16) | ((uint32_t)pixel.r << 8) | pixel.b;
            uint32_t data = grb << 8;
            
            // FIXED: Target the dynamically claimed state machine index
            pio_sm_put_blocking(led_pio, claimed_sms[ch], data);
        }
    }
}

void ledCommitCanvas() {
    memcpy(leds, canvasLeds, sizeof(leds));
    showAll();
}

void setPixel(uint8_t output, uint16_t led, CRGB color) {
    if (output >= NUM_CHANNELS || led >= LEDS_PER_CHANNEL) return;
    leds[output][led] = color;
}

void clearAll() {
    memset(leds, 0, sizeof(leds));
    showAll();
}