class Tone
  def initialize(pin)
    puts "Init Tone"
    pwm_tone_init pin
  end

  def set_tones(tones)
    pwm_tone_set_tones tones
  end
  def start
    pwm_tone_start
  end
end