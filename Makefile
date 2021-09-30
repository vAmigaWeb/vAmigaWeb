SUBDIRS = Emulator

CXX        = emcc 
CC         = $(CXX)
CCFLAGS    = -std=c++17 -O2 -Wall -Wfatal-errors -fexceptions
CPPFLAGS   = -I $(CURDIR) $(addprefix -I, $(shell find $(CURDIR) -type d))

WASM_EXPORTS= -s EXPORTED_RUNTIME_METHODS=['cwrap'] -s EXPORTED_FUNCTIONS="['_main', '_wasm_toggleFullscreen', '_wasm_loadFile', '_wasm_key', '_wasm_joystick', '_wasm_reset', '_wasm_halt', '_wasm_run', '_wasm_take_user_snapshot', '_wasm_create_renderer', '_wasm_set_warp', '_wasm_pull_user_snapshot_file','_wasm_set_borderless', '_wasm_press_play', '_wasm_sprite_info', '_wasm_set_sid_model', '_wasm_cut_layers', '_wasm_rom_info', '_wasm_set_2nd_sid', '_wasm_set_sid_engine', '_wasm_get_cpu_cycles', '_wasm_set_color_palette', '_wasm_schedule_key', '_wasm_peek', '_wasm_poke', '_wasm_export_disk', '_wasm_configure', '_wasm_write_string_to_ser', '_wasm_print_error']"
LFLAGS= -s USE_SDL=2 $(WASM_EXPORTS) -s NO_DISABLE_EXCEPTION_CATCHING -s BINARYEN_EXTRA_PASSES=--one-caller-inline-max-function-size=18500 -s LLD_REPORT_UNDEFINED
#-s BINARYEN_EXTRA_PASSES=--one-caller-inline-max-function-size=19306
#-s ASSERTIONS=1

PUBLISH_FOLDER=../vAmigaWeb.github.io

export CC CCFLAGS CPPFLAGS

SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)

.PHONY: all prebuild subdirs clean bin

all: prebuild $(OBJ) subdirs
	@echo > /dev/null
	
prebuild:
	@echo "Entering ${CURDIR}"
		
subdirs:
	@for dir in $(SUBDIRS); do \
		echo "Entering ${CURDIR}/$$dir"; \
		$(MAKE) -C $$dir; \
	done

clean:
	@echo "Cleaning up $(CURDIR)"
	@rm -f index.html
	@rm -f vAmiga.js
	@rm -f vAmiga.wasm
	@rm -f vAmiga.data
	@rm -f *.o
	@for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

bin:
	@echo "Linking"
	$(CC) $(LFLAGS) -o vAmiga.html --shell-file shell.html  -s INITIAL_MEMORY=128MB -s ALLOW_MEMORY_GROWTH=1 *.o Emulator/*.o Emulator/*/*.o Emulator/*/*/*.o
	#--preload-file roms
	#-s TOTAL_STACK=512MB
	mv vAmiga.html index.html


publish:
	rm -rf $(PUBLISH_FOLDER)/roms
	rm -rf $(PUBLISH_FOLDER)/css
	rm -rf $(PUBLISH_FOLDER)/js
	rm -rf $(PUBLISH_FOLDER)/img
	rm -f  $(PUBLISH_FOLDER)/vC64.*
	rm -f  $(PUBLISH_FOLDER)/*.js
	rm -f  $(PUBLISH_FOLDER)/*.json
	rm -f  $(PUBLISH_FOLDER)/index.html
	rm -f  $(PUBLISH_FOLDER)/run.html
	cp vC64.* $(PUBLISH_FOLDER)
	cp -r js $(PUBLISH_FOLDER)
	cp -r css $(PUBLISH_FOLDER)
	cp -r img $(PUBLISH_FOLDER)
	cp -r roms $(PUBLISH_FOLDER)
	cp index.html $(PUBLISH_FOLDER)
	cp run.html $(PUBLISH_FOLDER)
	cp sw.js $(PUBLISH_FOLDER)
	cp manifest.json $(PUBLISH_FOLDER)


%.o: %.cpp $(DEPS)
	@echo "Compiling $<"
	@$(CC) $(CCFLAGS) $(CPPFLAGS) -c -o $@ $<
