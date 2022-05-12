while !$mutex
  relinquish
end


puts "Starting rgb task ..."

initial = true
while true
  unless $rgb
    sleep 1
  else
    if initial
      $rgb.turn_off
      2.times do
        $rgb.show()
      end
      $rgb.reset_pixel
      initial = false
    end

    $rgb.show
  end
end
