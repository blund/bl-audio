
-- Call cadence_setup
local ctx = cadence_setup(44100)
local s   = new_sine(ctx)

set_sine_freq(s, 440)

for i = 1, 10 do
   local sample = gen_sine(ctx, s)
   print("sample:", sample)
end


