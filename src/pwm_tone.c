#include <mrubyc.h>

#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define PWM_TONE_BASE_HZ (1000000UL)
#define PWM_TONE_SERIES_MAX 64

static uint16_t tone_series[PWM_TONE_SERIES_MAX];
static uint32_t current_tone_count;
static uint32_t tone_counts[PWM_TONE_SERIES_MAX];
static uint8_t tone_index;
static uint8_t tone_index_max;
static uint tone_slice_num;
static uint32_t tone_count;
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
  if (++tone_count>(tone_counts[tone_index]))
  {
    if(++tone_index>tone_index_max)
    {
      pwm_set_enabled(tone_slice_num, false);
      return;
    }

    if (tone_counts[tone_index]==0)
    {
      pwm_set_irq_enabled(tone_slice_num, false);
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
  //irq_set_priority(PWM_IRQ_WRAP, PICO_DEFAULT_IRQ_PRIORITY + 2);
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

  for (uint8_t i=0; i<=tone_index_max; i++)
  {
    mrbc_array *item = rb_ary->data[i].array;
    tone_series[i] = mrbc_integer(item->data[0]);
    if (! tone_series[i])
    {
      tone_series[i] = 1;
    }
    tone_counts[i] = ((uint32_t)mrbc_integer(item->data[1]) * tone_series[i]) >> 10;
    if(tone_counts[i]<10)
    {
      tone_counts[i] = 10;
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
