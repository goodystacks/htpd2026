// thomsonBurstModeExtern.ino — Arduino Giga R1 WiFi (STM32H747)
//
// Generates a gated 100 Hz signal with 10 µs pulses on pin 7,
// enabled/disabled by a gate input on pin 22.
//
// LAS_TRIG_IN (pin 23, rising edge) immediately drives a 1 ms HIGH
// pulse on LAS_TRIG_OUT (pin 6) and blanks the 100 Hz output for
// 20 ms after the trigger edge.
//
// Timer allocation:
//   TIM3 — 100 Hz gated pulse (update + OC ch1)
//   TIM4 — 1 Hz LED heartbeat (update only)
//   TIM2 — laser-trigger one-shot: 1 ms pulse + 20 ms blanking
//          (one-pulse mode, OC ch1 @ 1 ms, period @ 20 ms)
//
// NOTE: All ISRs use direct register access instead of HAL callbacks
// to avoid conflicts with Mbed OS, which owns HAL_TIM_PeriodElapsed-
// Callback and uses TIM5 for its us_ticker.

#include <Arduino.h>
#include "stm32h7xx_hal.h"

// ── Pin definitions ─────────────────────────────────────────
#define OUTPUT_PIN    7    // 100 Hz pulsed output
#define GATE_PIN      22   // drive HIGH to enable 100 Hz output
#define LAS_TRIG_IN   23   // laser trigger input  (rising edge)
#define LAS_TRIG_OUT  2    // laser trigger output  (1 ms pulse)

// ── Timer handles (used only for HAL init, not for callbacks) ───
static TIM_HandleTypeDef htim2;
static TIM_HandleTypeDef htim3;
static TIM_HandleTypeDef htim4;

// ── Shared state ────────────────────────────────────────────
static volatile bool blanking = false;   // true while 100 Hz is suppressed

// ── Direct-register ISRs ────────────────────────────────────
// These do NOT call HAL_TIM_IRQHandler() so they cannot collide
// with Mbed OS's own HAL callback chain.

extern "C" void TIM3_IRQHandler() {
  uint32_t sr = TIM3->SR;

  // Update (period rollover) — start of each 10 ms period
  if (sr & TIM_SR_UIF) {
    TIM3->SR = ~TIM_SR_UIF;             // clear flag
    if (!blanking && digitalRead(GATE_PIN)) {
      digitalWrite(OUTPUT_PIN, HIGH);
    } else {
      digitalWrite(OUTPUT_PIN, LOW);
    }
  }

  // CC1 match — 10 µs after period start → end pulse
  if (sr & TIM_SR_CC1IF) {
    TIM3->SR = ~TIM_SR_CC1IF;
    digitalWrite(OUTPUT_PIN, LOW);
  }
}

extern "C" void TIM4_IRQHandler() {
  if (TIM4->SR & TIM_SR_UIF) {
    TIM4->SR = ~TIM_SR_UIF;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

extern "C" void TIM2_IRQHandler() {
  uint32_t sr = TIM2->SR;

  // CC1 match at 1 ms — end laser trigger pulse
  if (sr & TIM_SR_CC1IF) {
    TIM2->SR = ~TIM_SR_CC1IF;
    digitalWrite(LAS_TRIG_OUT, LOW);
  }

  // Update at 20 ms — blanking period over (timer auto-stopped via OPM)
  if (sr & TIM_SR_UIF) {
    TIM2->SR = ~TIM_SR_UIF;
    blanking = false;
  }
}

// ── External interrupt — LAS_TRIG_IN rising edge ────────────
void lasTrigISR() {
  // Start 1 ms laser pulse
  digitalWrite(LAS_TRIG_OUT, HIGH);

  // Suppress 100 Hz output and force it LOW right now
  blanking = true;
  digitalWrite(OUTPUT_PIN, LOW);

  // (Re)start TIM2 one-shot: reset counter, clear flags, enable
  TIM2->CR1 &= ~TIM_CR1_CEN;   // stop (in case already running)
  TIM2->CNT  = 0;
  TIM2->SR   = 0;               // clear all pending interrupt flags
  TIM2->CR1 |= TIM_CR1_CEN;    // start — auto-stops after 20 ms (OPM)
}

// ── Setup ───────────────────────────────────────────────────
void setup() {
  // Pin setup
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);
  pinMode(GATE_PIN, INPUT);
  pinMode(LAS_TRIG_OUT, OUTPUT);
  digitalWrite(LAS_TRIG_OUT, LOW);
  pinMode(LAS_TRIG_IN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // ── Common clock info ──────────────────────────────────────
  // On STM32H747 the APB1 timer clock is 2× PCLK1 when the APB1
  // prescaler is > 1 (the default Arduino/Mbed clock config).
  uint32_t timerClk  = 2UL * HAL_RCC_GetPCLK1Freq();
  uint32_t prescaler = (timerClk / 1000000UL) - 1;  // → 1 MHz (1 µs/tick)

  // ── TIM3 — 100 Hz gated pulse ─────────────────────────────
  __HAL_RCC_TIM3_CLK_ENABLE();
  htim3.Instance               = TIM3;
  htim3.Init.Prescaler         = prescaler;
  htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim3.Init.Period            = 10000 - 1;            // 100 Hz
  htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim3);

  TIM_OC_InitTypeDef ocInit3 = {};
  ocInit3.OCMode     = TIM_OCMODE_TIMING;
  ocInit3.Pulse      = 10;                             // 10 µs
  ocInit3.OCPolarity = TIM_OCPOLARITY_HIGH;
  HAL_TIM_OC_ConfigChannel(&htim3, &ocInit3, TIM_CHANNEL_1);

  // Enable update + CC1 interrupts via registers
  TIM3->DIER |= TIM_DIER_UIE | TIM_DIER_CC1IE;

  HAL_NVIC_SetPriority(TIM3_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(TIM3_IRQn);

  // Clear any pending flags then start
  TIM3->SR = 0;
  TIM3->CR1 |= TIM_CR1_CEN;

  // ── TIM4 — 1 Hz LED blink ─────────────────────────────────
  __HAL_RCC_TIM4_CLK_ENABLE();
  htim4.Instance               = TIM4;
  htim4.Init.Prescaler         = (timerClk / 10000UL) - 1;  // → 10 kHz
  htim4.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim4.Init.Period            = 5000 - 1;                   // 2 Hz toggle
  htim4.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_TIM_Base_Init(&htim4);

  TIM4->DIER |= TIM_DIER_UIE;

  HAL_NVIC_SetPriority(TIM4_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(TIM4_IRQn);

  TIM4->SR = 0;
  TIM4->CR1 |= TIM_CR1_CEN;

  // ── TIM2 — laser trigger one-shot (1 ms pulse + 20 ms blank) ──
  // TIM2 is a 32-bit APB1 timer.  TIM5 is reserved by Mbed (us_ticker).
  __HAL_RCC_TIM2_CLK_ENABLE();
  htim2.Instance               = TIM2;
  htim2.Init.Prescaler         = prescaler;            // 1 MHz (1 µs/tick)
  htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim2.Init.Period            = 20000 - 1;            // 20 ms total
  htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  HAL_TIM_Base_Init(&htim2);

  // Enable one-pulse mode: timer auto-stops after reaching period
  TIM2->CR1 |= TIM_CR1_OPM;

  // OC channel 1 at 1 ms — ends the laser trigger pulse
  TIM_OC_InitTypeDef ocInit2 = {};
  ocInit2.OCMode     = TIM_OCMODE_TIMING;
  ocInit2.Pulse      = 1000;                           // 1 ms
  ocInit2.OCPolarity = TIM_OCPOLARITY_HIGH;
  HAL_TIM_OC_ConfigChannel(&htim2, &ocInit2, TIM_CHANNEL_1);

  // Enable update + CC1 interrupts but do NOT start the timer —
  // it will be started on-demand by the external interrupt ISR.
  TIM2->DIER |= TIM_DIER_UIE | TIM_DIER_CC1IE;
  TIM2->SR    = 0;

  HAL_NVIC_SetPriority(TIM2_IRQn, 1, 0);   // highest priority
  HAL_NVIC_EnableIRQ(TIM2_IRQn);

  // ── External interrupt — laser trigger input ───────────────
  attachInterrupt(digitalPinToInterrupt(LAS_TRIG_IN), lasTrigISR, RISING);
}

// ── Loop ────────────────────────────────────────────────────
void loop() {
  // Nothing here — ISRs handle all timing.
}
