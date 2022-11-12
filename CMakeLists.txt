cmake_minimum_required(VERSION 3.16)

project(vAmiga C CXX)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

add_executable(vAmiga mainsdl.cpp)


target_compile_options(vAmiga PRIVATE -s USE_SDL=2)
target_compile_options(vAmiga PRIVATE -Wall -Werror)
target_compile_options(vAmiga PRIVATE -Wno-unused-parameter)
target_compile_options(vAmiga PRIVATE -O3)
#target_compile_options(vAmiga PRIVATE -O3 -mnontrapping-fptoint -fwasm-exceptions)
#target_compile_options(vAmiga PRIVATE -flto)
target_compile_options(vAmiga PRIVATE -Wfatal-errors -fexceptions)
#target_compile_options(vAmiga PRIVATE -g) 


add_subdirectory("Emulator" "vAmigaCore")
target_link_libraries(vAmiga vAmigaCore)

#emscripten linker settings
set(LF_MEM   "-s INITIAL_MEMORY=320MB -s TOTAL_STACK=32MB -s ALLOW_MEMORY_GROWTH=1")
set(LF_OPTI  "-O3 -s NO_DISABLE_EXCEPTION_CATCHING")
#set(LF_OPTI  "-O3 -mnontrapping-fptoint -fwasm-exceptions")
set(LF_SHELL "--shell-file ../shell.html")
set(LF_SDL2  "-s USE_SDL=2 -s USE_WEBGL2=1 -s MAX_WEBGL_VERSION=2")
set(LF_EXPORTS "-s EXPORTED_RUNTIME_METHODS=['cwrap','ccall'] -s EXPORTED_FUNCTIONS=\"['_free','_main', '_wasm_toggleFullscreen', '_wasm_loadFile', '_wasm_key', '_wasm_joystick', '_wasm_reset', '_wasm_halt', '_wasm_run', '_wasm_take_user_snapshot', '_wasm_create_renderer', '_wasm_set_warp', '_wasm_pull_user_snapshot_file','_wasm_delete_user_snapshot', '_wasm_sprite_info', '_wasm_cut_layers', '_wasm_rom_info', '_wasm_get_cpu_cycles', '_wasm_set_color_palette', '_wasm_schedule_key', '_wasm_peek', '_wasm_poke', '_wasm_export_disk', '_wasm_has_disk','_wasm_configure', '_wasm_write_string_to_ser', '_wasm_print_error', '_wasm_power_on', '_wasm_get_sound_buffer_address', '_wasm_copy_into_sound_buffer', '_wasm_set_sample_rate', '_wasm_mouse', '_wasm_mouse_button', '_wasm_set_display', '_wasm_auto_type','_wasm_set_target_fps', '_wasm_get_renderer','_wasm_get_render_width','_wasm_get_render_height', '_wasm_get_config_item', '_wasm_get_core_version', '_wasm_eject_disk', '_wasm_draw_one_frame', '_wasm_execute']\"")
set(LF_OTHER  "-s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=1  -s LLD_REPORT_UNDEFINED -s ASSERTIONS=0 -s GL_ASSERTIONS=0")
#-s ALLOW_MEMORY_GROWTH=1 -g -s BINARYEN_EXTRA_PASSES=--one-caller-inline-max-function-size=19306 

set_target_properties(vAmiga PROPERTIES LINK_FLAGS "${LF_SHELL} ${LF_OPTI} ${LF_MEM} ${LF_SDL2} ${LF_EXPORTS} ${LF_OTHER}" )
set(CMAKE_EXECUTABLE_SUFFIX ".html")


#run local webserver
add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E copy vAmiga.html ../index.html
    COMMAND ${CMAKE_COMMAND} -E copy vAmiga.js ..
    COMMAND ${CMAKE_COMMAND} -E copy vAmiga.wasm ..
    COMMAND ${CMAKE_COMMAND} -E chdir .. python3 -m http.server 8080
	DEPENDS vAmiga
)


#publish complete build to a gh-pages folder
set(PUBLISH_FOLDER "../../vAmigaWeb.github.io")
add_custom_target(publish
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${PUBLISH_FOLDER}/roms
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${PUBLISH_FOLDER}/sounds
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${PUBLISH_FOLDER}/css
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${PUBLISH_FOLDER}/js
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${PUBLISH_FOLDER}/img
	COMMAND ${CMAKE_COMMAND} -E rm -f  ${PUBLISH_FOLDER}/vC64.*
	COMMAND ${CMAKE_COMMAND} -E rm -f  ${PUBLISH_FOLDER}/*.js
	COMMAND ${CMAKE_COMMAND} -E rm -f  ${PUBLISH_FOLDER}/*.json
	COMMAND ${CMAKE_COMMAND} -E rm -f  ${PUBLISH_FOLDER}/index.html
	COMMAND ${CMAKE_COMMAND} -E rm -f  ${PUBLISH_FOLDER}/run.html
	COMMAND ${CMAKE_COMMAND} -E copy vAmiga.html ${PUBLISH_FOLDER}/index.html
    COMMAND ${CMAKE_COMMAND} -E copy vAmiga.js ${PUBLISH_FOLDER}
	COMMAND ${CMAKE_COMMAND} -E copy vAmiga.wasm ${PUBLISH_FOLDER}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ../js ${PUBLISH_FOLDER}/js
	COMMAND ${CMAKE_COMMAND} -E copy_directory ../css ${PUBLISH_FOLDER}/css
	COMMAND ${CMAKE_COMMAND} -E copy_directory ../img ${PUBLISH_FOLDER}/img
	COMMAND ${CMAKE_COMMAND} -E copy_directory ../roms ${PUBLISH_FOLDER}/roms
	COMMAND ${CMAKE_COMMAND} -E copy_directory ../sounds ${PUBLISH_FOLDER}/sounds
	COMMAND ${CMAKE_COMMAND} -E copy ../run.html ${PUBLISH_FOLDER}
	COMMAND ${CMAKE_COMMAND} -E copy ../sw.js ${PUBLISH_FOLDER}
	COMMAND ${CMAKE_COMMAND} -E copy ../manifest.json ${PUBLISH_FOLDER}
	DEPENDS vAmiga
)

set(UAT_FOLDER "../../uat")
add_custom_target(uat
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${UAT_FOLDER}/sounds
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${UAT_FOLDER}/roms
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${UAT_FOLDER}/css
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${UAT_FOLDER}/js
	COMMAND ${CMAKE_COMMAND} -E rm -rf ${UAT_FOLDER}/img
	COMMAND ${CMAKE_COMMAND} -E rm -f  ${UAT_FOLDER}/vC64.*
	COMMAND ${CMAKE_COMMAND} -E rm -f  ${UAT_FOLDER}/*.js
#	COMMAND ${CMAKE_COMMAND} -E rm -f  ${UAT_FOLDER}/*.json
	COMMAND ${CMAKE_COMMAND} -E rm -f  ${UAT_FOLDER}/index.html
	COMMAND ${CMAKE_COMMAND} -E rm -f  ${UAT_FOLDER}/run.html
	COMMAND ${CMAKE_COMMAND} -E copy vAmiga.html ${UAT_FOLDER}/index.html
    COMMAND ${CMAKE_COMMAND} -E copy vAmiga.js ${UAT_FOLDER}
	COMMAND ${CMAKE_COMMAND} -E copy vAmiga.wasm ${UAT_FOLDER}
	COMMAND ${CMAKE_COMMAND} -E copy_directory ../js ${UAT_FOLDER}/js
	COMMAND ${CMAKE_COMMAND} -E copy_directory ../css ${UAT_FOLDER}/css
	COMMAND ${CMAKE_COMMAND} -E copy_directory ../img ${UAT_FOLDER}/img
	COMMAND ${CMAKE_COMMAND} -E copy_directory ../roms ${UAT_FOLDER}/roms
	COMMAND ${CMAKE_COMMAND} -E copy_directory ../sounds ${UAT_FOLDER}/sounds
	COMMAND ${CMAKE_COMMAND} -E copy ../run.html ${UAT_FOLDER}
	COMMAND ${CMAKE_COMMAND} -E copy ../sw.js ${UAT_FOLDER}
#	COMMAND ${CMAKE_COMMAND} -E copy ../manifest.json ${UAT_FOLDER}
	DEPENDS vAmiga
)
