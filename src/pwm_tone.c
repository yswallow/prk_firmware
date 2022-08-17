#include <mrubyc.h>

#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define PWM_TONE_BASE_HZ (1000000UL)
#define PWM_TONE_SERIES_MAX 127

static uint16_t tone_series[PWM_TONE_SERIES_MAX];
static uint8_t tone_index;
static uint8_t tone_index_max;
static uint tone_slice_num;
static uint32_t tone_count;
static uint16_t tone_cycle_ms;
static uint8_t tone_pin;


static inline void
reset_to_tone(uint16_t freq)
{
  pwm_set_wrap(tone_pin, PWM_TONE_BASE_HZ/freq);
  pwm_set_gpio_level(tone_pin, (PWM_TONE_BASE_HZ/freq)>>1);
  tone_count = 1;
}

static void
on_pwm_tone_wrap(void)
{
  pwm_clear_irq(tone_slice_num);
  if (++tone_count>((uint32_t)tone_cycle_ms*tone_series[tone_index]/1000))
  {
    if(++tone_index>tone_index_max)
    {
      pwm_set_enabled(tone_slice_num, false);
      return;
    }

    reset_to_tone(tone_series[tone_index]);
  }
}

void
c_pwm_tone_init(mrb_vm *vm, mrb_value *v, int argc)
{
  tone_pin = GET_INT_ARG(1);

  gpio_set_function(tone_pin, GPIO_FUNC_PWM);
  tone_slice_num = pwm_gpio_to_slice_num(tone_pin);
  
  pwm_clear_irq(tone_slice_num);
  pwm_set_irq_enabled(tone_slice_num, true);
  //irq_add_shared_handler(PWM_IRQ_WRAP, on_pwm_tone_wrap, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_exclusive_handler(PWM_IRQ_WRAP, on_pwm_tone_wrap);
  irq_set_enabled(PWM_IRQ_WRAP, true);
  
  pwm_config config = pwm_get_default_config();
  pwm_config_set_clkdiv(&config, (float)clock_get_hz(clk_sys)/PWM_TONE_BASE_HZ);
  
  pwm_init(tone_slice_num, &config, false);
}

void
c_pwm_tone_set_tones(mrb_vm *vm, mrb_value *v, int argc)
{
  mrbc_array *rb_ary = GET_ARY_ARG(1).array;
  tone_index_max = ( rb_ary->n_stored < PWM_TONE_SERIES_MAX ) ? (rb_ary->n_stored - 1) : (PWM_TONE_SERIES_MAX - 1);
  tone_cycle_ms = GET_INT_ARG(2);

  if( tone_cycle_ms==0 )
  {
    irq_set_enabled(PWM_IRQ_WRAP, false);
  }

  for(uint8_t i=0; i<=tone_index_max; i++)
  {
    tone_series[i] = mrbc_integer(rb_ary->data[i]);
    if(! tone_series[i]) {
      tone_series[i] = 1;
    }
  }
}

void
c_pwm_tone_start(mrb_vm *vm, mrb_value *v, int argc)
{
  tone_index = 0;
  reset_to_tone(tone_series[tone_index]);
  pwm_set_enabled(tone_slice_num, true);
}
