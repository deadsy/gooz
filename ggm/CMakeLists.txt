cmake_minimum_required(VERSION 3.13.1)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(NONE)

target_include_directories(app
	PRIVATE
		src/inc
		src/module
)

target_sources(app
	PRIVATE
		src/core/block.c
		src/core/event.c
		src/core/lut.c
		src/core/math.c
		src/core/midi.c
		src/core/module.c
		src/core/port.c
		src/core/synth.c
		src/core/util.c
		src/module/template.c
		src/module/delay/delay.c
		src/module/env/adsr.c
		src/module/filter/biquad.c
		src/module/filter/svf.c
		src/module/midi/mono.c
		src/module/midi/poly.c
		src/module/mix/pan.c
		src/module/osc/goom.c
		src/module/osc/ks.c
		src/module/osc/lfo.c
		src/module/osc/noise.c
		src/module/osc/sine.c
		src/module/pm/breath.c
		src/module/root/metro.c
		src/module/root/poly.c
		src/module/seq/seq.c
		src/module/seq/smf.c
		src/module/voice/goom.c
		src/module/voice/osc.c
		src/os/zephyr/zephyr.c
		src/os/zephyr/main.c
)
