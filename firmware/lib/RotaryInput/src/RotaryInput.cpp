#include "RotaryInput.h"

#include <Arduino.h>
#include "hardware/gpio.h"

namespace RotaryInput {

static Config s_cfg{2, 3, 6, false, 100};
static volatile int32_t s_pendingSteps = 0;
static bool s_lastBtn = true;
static bool s_btnEdge = false;
static volatile uint8_t s_lastAB = 0;
static volatile int8_t s_quarterStepAcc = 0;

static constexpr uint8_t kEventQueueSize = 64;
static volatile uint8_t s_eventQueue[kEventQueueSize] = {};
static volatile uint8_t s_eventHead = 0;
static volatile uint8_t s_eventTail = 0;

static constexpr int8_t kQuarterStepsPerDetent = 2;

// Transition delta table for quadrature encoder.
// Index is (prevAB << 2) | currAB, values are quarter-steps.
static const int8_t kTransitionDelta[16] = {
  0, -1, +1,  0,
  +1,  0,  0, -1,
  -1,  0,  0, +1,
  0, +1, -1,  0
};

static inline void enqueueAB(uint8_t ab) {
  const uint8_t nextTail = static_cast<uint8_t>((s_eventTail + 1) % kEventQueueSize);
  if (nextTail == s_eventHead) {
    // Queue full: drop newest event. Existing history is kept intact.
    return;
  }

  s_eventQueue[s_eventTail] = ab;
  s_eventTail = nextTail;
}

static inline bool dequeueAB(uint8_t& outAB) {
  bool hasEvent = false;
  noInterrupts();
  if (s_eventHead != s_eventTail) {
    outAB = s_eventQueue[s_eventHead];
    s_eventHead = static_cast<uint8_t>((s_eventHead + 1) % kEventQueueSize);
    hasEvent = true;
  }
  interrupts();
  return hasEvent;
}

static inline void sampleAndQueueEncoder() {
  const bool a = gpio_get(s_cfg.pinA);
  const bool b = gpio_get(s_cfg.pinB);

  uint8_t ab = 0;
  if (b) ab |= 0x02;
  if (a) ab |= 0x01;
  enqueueAB(ab);
}

static void gpioCallback(uint gpio, uint32_t events) {
  (void)events;
  if (gpio == s_cfg.pinA || gpio == s_cfg.pinB) {
    sampleAndQueueEncoder();
  }
}

void begin(const Config& cfg) {
  s_cfg = cfg;

  gpio_set_irq_enabled(s_cfg.pinA, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
  gpio_set_irq_enabled(s_cfg.pinB, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);

  pinMode(s_cfg.pinA, INPUT_PULLUP);
  pinMode(s_cfg.pinB, INPUT_PULLUP);
  if (s_cfg.pinButton >= 0) {
    pinMode(static_cast<uint8_t>(s_cfg.pinButton), INPUT_PULLUP);
    s_lastBtn = digitalRead(static_cast<uint8_t>(s_cfg.pinButton));
  }

  s_lastAB = 0;
  if (digitalRead(s_cfg.pinB)) s_lastAB |= 0x02;
  if (digitalRead(s_cfg.pinA)) s_lastAB |= 0x01;
  s_quarterStepAcc = 0;
  s_pendingSteps = 0;
  s_eventHead = 0;
  s_eventTail = 0;
  s_btnEdge = false;

  gpio_set_irq_enabled_with_callback(s_cfg.pinA,
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true,
                                     &gpioCallback);
  gpio_set_irq_enabled(s_cfg.pinB,
                       GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                       true);
}

int32_t readDeltaSteps() {
  // Fallback polling path in case this core misses or suppresses GPIO interrupts.
  sampleAndQueueEncoder();

  uint8_t currAB = 0;
  while (dequeueAB(currAB)) {
    const uint8_t idx = static_cast<uint8_t>((s_lastAB << 2) | currAB);
    s_lastAB = currAB;

    const int8_t q = kTransitionDelta[idx];
    if (q == 0) {
      continue;
    }

    s_quarterStepAcc = static_cast<int8_t>(s_quarterStepAcc + q);
    if (s_quarterStepAcc >= kQuarterStepsPerDetent) {
      s_quarterStepAcc = 0;
      s_pendingSteps += s_cfg.invertDirection ? -1 : +1;
    } else if (s_quarterStepAcc <= -kQuarterStepsPerDetent) {
      s_quarterStepAcc = 0;
      s_pendingSteps += s_cfg.invertDirection ? +1 : -1;
    }
  }

  noInterrupts();
  const int32_t delta = s_pendingSteps;
  s_pendingSteps = 0;
  interrupts();
  return delta;
}

bool buttonPressed() {
  if (s_cfg.pinButton < 0) return false;

  const bool now = digitalRead(static_cast<uint8_t>(s_cfg.pinButton));
  if (s_lastBtn && !now) {
    s_btnEdge = true; // falling edge
  }
  s_lastBtn = now;

  const bool ret = s_btnEdge;
  s_btnEdge = false;
  return ret;
}

int32_t stepHz() {
  return s_cfg.stepHz;
}

void setStepHz(int32_t step) {
  if (step < 1) step = 1;
  s_cfg.stepHz = step;
}

} // namespace RotaryInput
