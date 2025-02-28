GPP := g++-11

GPP += -std=c++20
GPP += -mavx -mavx2 -mfma
#GPP += -O3
#GPP += -DNDEBUG

#GPP += -DALLOC_COUNT

NO_FLAGS := 
SDL2   := `sdl2-config --cflags --libs`
OPENGL := -lGL -ldl

ALL_LFLAGS := $(SDL2) $(OPENGL) -lpthread


root       := ../../../..

app   := $(root)/01_basic_fft
build := $(app)/build/ubuntu
src   := $(app)/src

pltfm := $(src)/pltfm/ubuntu

libs := $(root)/libs

exe := basic

program_exe := $(build)/$(exe)


#*** imgui ***

imgui := $(src)/pltfm/imgui_sdl2_ogl3
imgui_options_h := $(imgui)/imgui_options.hpp

imgui_include_h := $(imgui)/imgui_include.hpp
imgui_include_h += $(imgui_options_h)

#**************


#*** libs/util ***

util := $(libs)/util

types_h := $(util)/types.hpp

numeric_h := $(util)/numeric.hpp
numeric_h += $(types_h)

stopwatch_h    := $(util)/stopwatch.hpp

#************


#*** stb_libs ***

stb_libs := $(libs)/stb_libs

qsprintf_h := $(stb_libs)/qsprintf.hpp

stb_libs_c := $(stb_libs)/stb_libs.cpp

#*************


#*** fft ***

fft := $(libs)/fft

fft_h := $(fft)/fft.hpp
fft_h += $(numeric_h)

fft_c := $(fft)/fft.cpp

#**********


#*** mic ***

mic := $(src)/mic

mic_h := $(mic)/mic.hpp
mic_h += $(fft_h)

mic_c := $(mic)/mic.cpp
mic_c += $(stopwatch_h)

#**********


#*** display ***

display := $(src)/display

display_h := $(display)/display.hpp
display_h += $(qsprintf_h)
display_h += $(mic_h)

#***************


#*** main cpp ***

main_c := $(pltfm)/basic_main_ubuntu.cpp
main_o := $(build)/main.o
obj    := $(main_o)

main_dep := $(imgui_include_h)

# main_o.cpp
main_dep += $(pltfm)/main_o.cpp
main_dep += $(display_h)
main_dep += $(stb_libs_c)
main_dep += $(mic_c)
main_dep += $(fft_c)

#****************


#*** imgui cpp ***

imgui_c := $(imgui)/imgui_o.cpp
imgui_o := $(build)/imgui.o
obj     += $(imgui_o)

imgui_dep := $(imgui_options_h)

#****************


#*** app ***


$(main_o): $(main_c) $(main_dep)
	@echo "\n  main"
	$(GPP) -o $@ -c $< $(ALL_LFLAGS)


$(imgui_o): $(imgui_c) $(imgui_dep)
	@echo "\n  imgui"
	$(GPP) -o $@ -c $< $(SDL2) $(OPENGL)

#**************


$(program_exe): $(obj)
	@echo "\n  program_exe"
	$(GPP) -o $@ $+ $(ALL_LFLAGS)


build: $(program_exe)


run: build
	$(program_exe)
	@echo "\n"


clean:
	rm -fv $(build)/*


clean_main:
	rm -fv $(build)/main.o

setup:
	mkdir -p $(build)