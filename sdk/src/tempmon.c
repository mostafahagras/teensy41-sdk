#include <stddef.h>

#include <teensy/imxrt.h>
#include <teensy/tempmon.h>

/*
 * TEMPMON measure timing.  The sensor updates TEMPMON_TEMPSENSE0 every
 * measure period; 0x03 gives the fastest automatic refresh, the same value
 * Teensyduino uses.  Conversion latency is bounded by a wait limit so
 * tempmon_get_temp_c() cannot hang if monitoring was stopped.
 */
#define TEMPMON_MEASURE_FREQUENCY 0x03u
#define TEMPMON_WAIT_LIMIT 100000000u

/* TEMPMON_TEMPSENSE0 bits (RT1060 has no alarm-enable bit; alarm interrupts
 * fire whenever the measured count crosses the programmed alarm values). */
#define TEMPMON_POWER_DOWN (1u << 0)
#define TEMPMON_MEASURE (1u << 1)
#define TEMPMON_FINISHED (1u << 2)
#define TEMPMON_TEMP_CNT_MASK 0x000FFF00u
#define TEMPMON_HIGH_ALARM_SHIFT 20
#define TEMPMON_HIGH_ALARM_MASK 0xFFF00000u

/* TEMPMON_TEMPSENSE2 bits */
#define TEMPMON_PANIC_ALARM_SHIFT 16
#define TEMPMON_PANIC_ALARM_MASK 0x0FFF0000u
#define TEMPMON_LOW_ALARM_MASK 0x0FFFu

/* OCOTP ANA1 fuse layout: [7:0] hot temperature (deg C),
 * [19:8] sensor count at the hot temperature, [31:20] sensor count at 25 C. */
#define OCOTP_ANA1_HOT_TEMP_SHIFT 0
#define OCOTP_ANA1_HOT_COUNT_SHIFT 8
#define OCOTP_ANA1_ROOM_COUNT_SHIFT 20

typedef struct {
  float hot_temp;     /* fused calibration temperature */
  float hot_count;    /* sensor count at hot_temp */
  float room_to_hot;  /* room count minus hot count */
} tempmon_calibration_t;

static tempmon_calibration_t tempmon_calibration;
static void (*tempmon_high_alarm_callback)(void);
static void (*tempmon_low_alarm_callback)(void);
static bool tempmon_initialized;

static float tempmon_count_to_c(uint32_t count) {
  return tempmon_calibration.hot_temp -
         ((float)count - tempmon_calibration.hot_count) *
             (tempmon_calibration.hot_temp - 25.0f) /
             tempmon_calibration.room_to_hot;
}

static uint32_t tempmon_count_from_c(float degrees_c) {
  return (uint32_t)(tempmon_calibration.hot_count +
                    (tempmon_calibration.hot_temp - degrees_c) *
                        tempmon_calibration.room_to_hot /
                        (tempmon_calibration.hot_temp - 25.0f));
}

static bool tempmon_read_fuses(void) {
  uint32_t calibration;
  uint32_t wait = TEMPMON_WAIT_LIMIT;

  /* Enable the OCOTP clock and wait for the fuse bus to be idle. */
  CCM_CCGR2 |= CCM_CCGR2_OCOTP_CTRL(CCM_CCGR_ON);
  while ((HW_OCOTP_CTRL & HW_OCOTP_CTRL_BUSY) != 0u && wait-- != 0u)
    __asm volatile("nop");
  if ((HW_OCOTP_CTRL & HW_OCOTP_CTRL_BUSY) != 0u ||
      (HW_OCOTP_CTRL & HW_OCOTP_CTRL_ERROR) != 0u)
    return false;

  calibration = HW_OCOTP_ANA1;
  tempmon_calibration.hot_temp =
      (float)((calibration >> OCOTP_ANA1_HOT_TEMP_SHIFT) & 0xFFu);
  tempmon_calibration.hot_count =
      (float)((calibration >> OCOTP_ANA1_HOT_COUNT_SHIFT) & 0xFFFu);
  tempmon_calibration.room_to_hot =
      (float)((calibration >> OCOTP_ANA1_ROOM_COUNT_SHIFT) & 0xFFFu) -
      tempmon_calibration.hot_count;

  /* Degenerate fuse data (room count <= hot count) would divide by zero. */
  return tempmon_calibration.hot_temp > 25.0f &&
         tempmon_calibration.room_to_hot > 0.0f;
}

static void tempmon_set_high_alarm_raw(uint32_t count) {
  uint32_t sense0 = TEMPMON_TEMPSENSE0;
  sense0 &= ~TEMPMON_HIGH_ALARM_MASK;
  sense0 |= (count << TEMPMON_HIGH_ALARM_SHIFT) & TEMPMON_HIGH_ALARM_MASK;
  TEMPMON_TEMPSENSE0 = sense0;
}

static void tempmon_set_panic_alarm_raw(uint32_t count) {
  uint32_t sense2 = TEMPMON_TEMPSENSE2;
  sense2 &= ~TEMPMON_PANIC_ALARM_MASK;
  sense2 |= (count << TEMPMON_PANIC_ALARM_SHIFT) & TEMPMON_PANIC_ALARM_MASK;
  TEMPMON_TEMPSENSE2 = sense2;
}

static void tempmon_set_low_alarm_raw(uint32_t count) {
  uint32_t sense2 = TEMPMON_TEMPSENSE2;
  sense2 &= ~TEMPMON_LOW_ALARM_MASK;
  sense2 |= count & TEMPMON_LOW_ALARM_MASK;
  TEMPMON_TEMPSENSE2 = sense2;
}

__attribute__((noreturn, used)) static void
tempmon_panic_isr(void) {
  /* The panic alarm means the die is past its shutdown temperature; there is
   * nothing safe to do from software, so stop. */
  for (;;) {
    __asm volatile("wfi");
  }
}

void tempmon_temperature_isr(void) {
  uint32_t count = (TEMPMON_TEMPSENSE0 & TEMPMON_TEMP_CNT_MASK) >> 8;

  /* High alarm: measured count fell to the high alarm count. */
  if (tempmon_high_alarm_callback != NULL &&
      count <= ((TEMPMON_TEMPSENSE0 & TEMPMON_HIGH_ALARM_MASK) >>
                TEMPMON_HIGH_ALARM_SHIFT)) {
    tempmon_high_alarm_callback();
  }
  /* Low alarm: measured count rose to the low alarm count. */
  else if (tempmon_low_alarm_callback != NULL &&
           count >= (TEMPMON_TEMPSENSE2 & TEMPMON_LOW_ALARM_MASK)) {
    tempmon_low_alarm_callback();
  }
}

int tempmon_init(void) {
  uint32_t wait = TEMPMON_WAIT_LIMIT;

  if (!tempmon_read_fuses())
    return TEMPMON_ERROR_CALIBRATION;

  /* The bandgap reference feeds the sensor; startup.c already sets
   * PMU_MISC0_REFTOP_SELFBIASOFF, so only wait for the bandgap to be up. */
  while ((PMU_MISC0 & PMU_MISC0_REFTOP_VBGUP) == 0u && wait-- != 0u)
    __asm volatile("nop");

  /* Power up and stop conversions while the alarm fields are written. */
  TEMPMON_TEMPSENSE0_CLR = TEMPMON_POWER_DOWN | TEMPMON_MEASURE;
  TEMPMON_TEMPSENSE1 = TEMPMON_CTRL1_MEASURE_FREQ(TEMPMON_MEASURE_FREQUENCY);

  tempmon_set_high_alarm_raw(tempmon_count_from_c(TEMPMON_DEFAULT_HIGH_ALARM_C));
  tempmon_set_panic_alarm_raw(
      tempmon_count_from_c(TEMPMON_DEFAULT_PANIC_ALARM_C));
  tempmon_set_low_alarm_raw(tempmon_count_from_c(TEMPMON_DEFAULT_LOW_ALARM_C));

  _VectorsRam[IRQ_TEMPERATURE_PANIC + 16] = tempmon_panic_isr;
  NVIC_SET_PRIORITY(IRQ_TEMPERATURE_PANIC, 0);
  NVIC_ENABLE_IRQ(IRQ_TEMPERATURE_PANIC);

  /* Start periodic measurements. */
  TEMPMON_TEMPSENSE0_SET = TEMPMON_MEASURE;

  tempmon_initialized = true;
  return TEMPMON_OK;
}

float tempmon_get_temp_c(void) {
  uint32_t wait = TEMPMON_WAIT_LIMIT;
  uint32_t count;

  if (!tempmon_initialized)
    return -273.15f;

  while ((TEMPMON_TEMPSENSE0 & TEMPMON_FINISHED) == 0u && wait-- != 0u)
    __asm volatile("nop");
  if ((TEMPMON_TEMPSENSE0 & TEMPMON_FINISHED) == 0u)
    return -273.15f;

  count = (TEMPMON_TEMPSENSE0 & TEMPMON_TEMP_CNT_MASK) >> 8;
  return tempmon_count_to_c(count);
}

float tempmon_get_temp_f(void) {
  return tempmon_get_temp_c() * 9.0f / 5.0f + 32.0f;
}

int tempmon_set_panic_alarm_c(float degrees_c) {
  if (!tempmon_initialized)
    return TEMPMON_ERROR_INVALID;
  /* Guard against a threshold colder than the fused reference temperature,
   * which would flip the count calculation upside down. */
  if (degrees_c >= tempmon_calibration.hot_temp || degrees_c < 0.0f)
    return TEMPMON_ERROR_INVALID;
  tempmon_set_panic_alarm_raw(tempmon_count_from_c(degrees_c));
  return TEMPMON_OK;
}

int tempmon_attach_high_alarm(float degrees_c, void (*callback)(void)) {
  if (!tempmon_initialized)
    return TEMPMON_ERROR_INVALID;

  tempmon_high_alarm_callback = callback;
  tempmon_set_high_alarm_raw(tempmon_count_from_c(degrees_c));

  NVIC_CLEAR_PENDING(IRQ_TEMPERATURE);
  _VectorsRam[IRQ_TEMPERATURE + 16] = tempmon_temperature_isr;
  NVIC_ENABLE_IRQ(IRQ_TEMPERATURE);
  return TEMPMON_OK;
}

int tempmon_attach_low_alarm(float degrees_c, void (*callback)(void)) {
  if (!tempmon_initialized)
    return TEMPMON_ERROR_INVALID;

  tempmon_low_alarm_callback = callback;
  tempmon_set_low_alarm_raw(tempmon_count_from_c(degrees_c));

  NVIC_CLEAR_PENDING(IRQ_TEMPERATURE);
  _VectorsRam[IRQ_TEMPERATURE + 16] = tempmon_temperature_isr;
  NVIC_ENABLE_IRQ(IRQ_TEMPERATURE);
  return TEMPMON_OK;
}

void tempmon_stop(void) { TEMPMON_TEMPSENSE0_CLR = TEMPMON_MEASURE; }

void tempmon_start(void) { TEMPMON_TEMPSENSE0_SET = TEMPMON_MEASURE; }

void tempmon_power_down(void) {
  TEMPMON_TEMPSENSE0_CLR = TEMPMON_MEASURE;
  TEMPMON_TEMPSENSE0_SET = TEMPMON_POWER_DOWN;
}
