//
// Live Memory View for vAmigaWeb
//
// Renders a bit-visualization of Amiga memory into a dockable panel on the
// right side of the screen. Every 16-bit word is drawn as 16 pixels (one
// pixel per bit), colored col1 (bit set) or col2 (bit clear). While the
// emulation is running and "live update" is enabled, the view refreshes
// each frame. The shown memory region can be scrolled by dragging or the
// mouse wheel, and the start address can be entered manually.
//
// Ported/adapted from the "coppenheimer" fork by Losso.
//

// internal (crisp) render resolution of the memory canvas
const MEMVIEW_WORDS_PER_ROW = 16;                       // 16 words per row
const MEMVIEW_HPIXELS = MEMVIEW_WORDS_PER_ROW * 16;     // 16px per word -> 256
const MEMVIEW_VPIXELS = 512;                            // rows (1px each)
const MEMVIEW_BYTES_PER_ROW = MEMVIEW_WORDS_PER_ROW * 2;
// number of bytes visible in the detail view (used for the overview highlight)
const MEMVIEW_WINDOW_BYTES = MEMVIEW_VPIXELS * MEMVIEW_BYTES_PER_ROW;

// overview resolution: each memory region gets its own stacked block
const MEMPREVIEW_HPIXELS = 256;      // internal width per block
const MEMPREVIEW_BLOCK_ROWS = 128;   // internal height per block

// base addresses of the amiga ram areas in the cpu address space
// (chip: $000000, zorro-II fast ram: $200000, slow/ranger ram: $C00000)
const MEM_CHIP_BASE = 0x000000;
const MEM_FAST_BASE = 0x200000;
const MEM_SLOW_BASE = 0xC00000;

// dynamic detail-view geometry (defaults to 16 words per row, contiguous).
// a bitplane "guess" click switches these so one canvas row equals one
// bitplane scanline (width = words per line, stride skips the modulo gap).
var memview_words_per_row = MEMVIEW_WORDS_PER_ROW;
var memview_hpixels = MEMVIEW_HPIXELS;
var memview_row_stride = MEMVIEW_BYTES_PER_ROW;   // bytes advanced per displayed row

// amber on dark-brown, matching the original look
var memdump_col1 = 0xffdf942a;
var memdump_col2 = 0xff371d20;

// writer-highlight mode: tint each word by who last wrote it (chip ram only).
// requires the core's write-owner tracking (wasm_set_write_tracking). the tag
// values match Memory::WRITE_OWNER_* (1 = cpu, 2 = blitter).
var memview_show_writers = true;
const MEMVIEW_WRITE_CPU = 1;
const MEMVIEW_WRITE_BLITTER = 2;
// writer mode colors: set bit vs. cleared bit background
var memdump_cpu_col1 = 0xffcccccc; // cpu = light gray
var memdump_cpu_col2 = 0xff1a1a1a; // cpu = very dark gray
var memdump_blt_col1 = 0xff2196f3; // blitter = blue
var memdump_blt_col2 = 0xff0d1f35; // blitter = very dark blue, 10% lighter

// heatmap fade: a fresh write flashes in its author's color (blitter = blue,
// cpu = gray) and then fades back to the default amber palette over this many
// *rendered emulation frames*. tying the fade to frames (instead of wall-clock
// time) means it freezes while the emulation is paused and only advances when
// frames are actually produced (running, single step or slomo).
var MEMVIEW_HEAT_FADE_FRAMES = 250;     // ~5s at 50fps (PAL); user-adjustable
// counter of rendered emulation frames while the panel is open (drives the fade)
var memview_frame_seq = 0;
// per-address decay state: addr -> { v: last seen value, f: last-write frame seq }.
// only holds addresses currently on screen; rebuilt when the window changes
var memview_heat = new Map();
var memview_heat_start = null;
var memview_heat_stride = null;

var live_memory_dump_enabled = false;
var memview_open = false;

var memdump_start = 0;
var memview_buffer = null;
var memview_ctx = null;
var memview_image_data = null;
var memview_initialized = false;

// drag state
var memview_pressed = false;
var memview_drag_start_y = 0;
var memview_drag_start_addr = 0;

var last_memdump_info_start = -1;

// overview state
var mempreview_pressed = false;
var mempreview_counter = 0;
var memview_drag_region = null;

// memory regions shown in the overview (built from the core's ram config)
var memview_regions = [];
var memview_regions_signature = "";

// bitplane area list state (auto-refreshed while the panel is open)
var memview_bpl_hover = false;       // paused while the mouse is over the list
var memview_bpl_counter = 0;         // frame throttle counter
var memview_bpl_last_raw = null;     // last rendered payload (skip if unchanged)
var memview_bpl_autoselect = true;   // follow mode: keep detail view locked to bpl1
var memview_bpl_sel_sig = null;      // signature of the currently followed bpl1
var memview_bpl_recent = [];         // recent bpl1 signatures (page-flip detection)
const MEMVIEW_BPL_THROTTLE = 15;     // update every n live frames
const MEMVIEW_BPL_RECENT_MAX = 6;    // how many recent signatures to remember
const MEMVIEW_BPL_MIN_HEIGHT = 3;    // drop guessed areas shorter than this (fragments)

// width reserved by the docked panel (used by scaleVMCanvas in vAmiga_canvas.js)
function memview_reserved_width() {
    if (!memview_open) return 0;
    let panel = document.getElementById("memview_panel");
    return panel ? panel.offsetWidth : 0;
}

function memview_init() {
    if (memview_initialized) return;
    let canvas = document.getElementById("memview_canvas");
    if (!canvas) return;
    memview_ctx = canvas.getContext("2d");
    memview_apply_geometry();

    // scroll by wheel
    canvas.addEventListener("wheel", function(e) {
        e.preventDefault();
        let rows = Math.sign(e.deltaY) * 8;
        memview_set_start(memdump_start + rows * memview_row_stride);
    }, { passive: false });

    // drag to scroll (pointer events cover mouse, touch and pen -> works on iPad)
    canvas.addEventListener("pointerdown", function(e) {
        e.preventDefault();
        memview_begin_drag();
        memview_pressed = true;
        memview_drag_start_y = e.clientY;
        memview_drag_start_addr = memdump_start;
        if (canvas.setPointerCapture) {
            try { canvas.setPointerCapture(e.pointerId); } catch (err) {}
        }
    });
    window.addEventListener("pointermove", function(e) {
        if (!memview_pressed) return;
        e.preventDefault();
        // scale pointer pixels to internal rows
        let canvasRect = canvas.getBoundingClientRect();
        let rowsPerPixel = MEMVIEW_VPIXELS / canvasRect.height;
        let dyRows = Math.round((e.clientY - memview_drag_start_y) * rowsPerPixel);
        memview_set_start(memview_drag_start_addr - dyRows * memview_row_stride, true);
        if (!(live_memory_dump_enabled && is_running_safe())) memdump();
    }, { passive: false });
    let end_detail_drag = function() {
        if (memview_pressed) {
            memview_pressed = false;
            if (!(live_memory_dump_enabled && is_running_safe())) memdump();
        }
        memview_end_drag();
    };
    window.addEventListener("pointerup", end_detail_drag);
    window.addEventListener("pointercancel", end_detail_drag);

    // manual start address input
    let startInput = document.getElementById("memview_start");
    if (startInput) {
        startInput.addEventListener("change", function() {
            let v = parseInt(this.value.replace(/[^0-9a-fA-F]/g, ""), 16);
            if (!isNaN(v)) {
                memview_set_start(v);
                if (!(live_memory_dump_enabled && is_running_safe())) memdump();
            }
        });
    }

    // overview drag handling (blocks are created per region in build_overview_dom)
    window.addEventListener("pointermove", function(e) {
        if (mempreview_pressed && memview_drag_region) {
            e.preventDefault();
            memview_overview_jump(memview_drag_region, e);
        }
    }, { passive: false });
    let end_overview_drag = function() {
        mempreview_pressed = false;
        memview_drag_region = null;
        memview_end_drag();
    };
    window.addEventListener("pointerup", end_overview_drag);
    window.addEventListener("pointercancel", end_overview_drag);

    // live update checkbox
    let liveCb = document.getElementById("memview_live");
    if (liveCb) {
        live_memory_dump_enabled = liveCb.checked;
        liveCb.addEventListener("change", function() {
            live_memory_dump_enabled = this.checked;
        });
    }

    // "step" button: pause emulation and advance exactly one frame per click.
    // if slomo is running, this cancels it and stays in manual
    // single-step mode (pressing slomo again re-enters slow-mo)
    let stepBtn = document.getElementById("memview_step");
    if (stepBtn) {
        // bind via pointerup (not click) so it fires reliably for touch and
        // apple pencil on ios, where the synthetic click can get swallowed
        stepBtn.addEventListener("pointerup", function() { memview_step_button(); });
    }

    // "slomo" button: toggle slow-motion single stepping (one frame every 500ms);
    // press again to resume normal running speed
    let slomoBtn = document.getElementById("memview_slomo");
    if (slomoBtn) {
        slomoBtn.addEventListener("pointerup", function() { memview_slomo_toggle(); });
    }

    // hop on press, just like the navbar icons: add the "pop" class on pointerup
    // and drop it when the popBounce animation finishes (restart via reflow so
    // rapid presses re-trigger)
    let addPop = function(btn) {
        if (!btn) return;
        btn.addEventListener("animationend", function(e) {
            if (e.animationName === "memview_pop") btn.classList.remove("pop");
        });
        btn.addEventListener("pointerup", function() {
            btn.classList.remove("pop");
            void btn.offsetWidth;   // force reflow so the animation restarts
            btn.classList.add("pop");
        });
    };
    addPop(stepBtn);
    addPop(slomoBtn);

    // info ("i") button: toggle the "what am I looking at?" explanation overlay
    let infoBtn = document.getElementById("memview_info");
    let infoPop = document.getElementById("memview_info_pop");
    let infoClose = document.getElementById("memview_info_close");
    addPop(infoBtn);   // hop on press, same as the slomo/step buttons
    if (infoBtn && infoPop) {
        infoBtn.addEventListener("pointerup", function(e) {
            e.stopPropagation();
            let showing = (infoPop.style.display === "none");
            if (showing) {
                // if there is enough room to the left of the docked panel, float
                // the overlay there (over the amiga canvas, via position:fixed so
                // it escapes the panel's overflow:hidden) so the detail view stays
                // visible while dragging the sliders; otherwise cover the detail
                // canvas as before. 320px overlay + 6px gap = ~330px.
                let panel = document.getElementById("memview_panel");
                let rect = panel ? panel.getBoundingClientRect() : null;
                if (rect && rect.left >= 330) {
                    infoPop.classList.add("to_left");
                    infoPop.style.right = (window.innerWidth - rect.left + 6) + "px";
                    infoPop.style.top = (rect.top + 6) + "px";
                    infoPop.style.maxHeight = (rect.height - 12) + "px";
                } else {
                    infoPop.classList.remove("to_left");
                    infoPop.style.right = "";
                    infoPop.style.top = "";
                    infoPop.style.maxHeight = "";
                }
            }
            infoPop.style.display = showing ? "flex" : "none";
        });
    }
    if (infoClose && infoPop) {
        infoClose.addEventListener("pointerup", function() { infoPop.style.display = "none"; });
    }
    let infoCloseBottom = document.getElementById("memview_info_close_bottom");
    if (infoCloseBottom && infoPop) {
        infoCloseBottom.addEventListener("pointerup", function() { infoPop.style.display = "none"; });
    }
    // panel close ("x") button in the memory header: bind via pointerup too
    // (the inline onclick was removed from the html for the same ios reason)
    let panelClose = document.getElementById("memview_close");
    if (panelClose) {
        panelClose.addEventListener("pointerup", function() { memview_close_panel(); });
    }
    // start interacting with the memory canvas -> get the overlay out of the way
    if (infoPop) {
        canvas.addEventListener("pointerdown", function() { infoPop.style.display = "none"; });
    }

    // heatmap fade-length slider (in rendered frames); persisted across sessions
    let fadeInput = document.getElementById("memview_fade_frames");
    let fadeVal = document.getElementById("memview_fade_frames_val");
    if (fadeInput) {
        let saved = (typeof load_setting === "function")
            ? parseInt(load_setting("memview_fade_frames", MEMVIEW_HEAT_FADE_FRAMES), 10)
            : MEMVIEW_HEAT_FADE_FRAMES;
        if (!isNaN(saved) && saved >= 10) MEMVIEW_HEAT_FADE_FRAMES = saved;
        fadeInput.value = MEMVIEW_HEAT_FADE_FRAMES;
        if (fadeVal) fadeVal.textContent = MEMVIEW_HEAT_FADE_FRAMES;
        fadeInput.addEventListener("input", function() {
            let v = parseInt(this.value, 10);
            if (isNaN(v) || v < 10) v = 10;
            MEMVIEW_HEAT_FADE_FRAMES = v;
            if (fadeVal) fadeVal.textContent = v;
            if (typeof save_setting === "function") save_setting("memview_fade_frames", v);
        });
    }

    // slomo speed slider (step interval in ms); applies live and is persisted
    let slomoInput = document.getElementById("memview_slomo_interval");
    let slomoVal = document.getElementById("memview_slomo_interval_val");
    if (slomoInput) {
        let saved = (typeof load_setting === "function")
            ? parseInt(load_setting("memview_slomo_interval", MEMVIEW_SLOMO_INTERVAL_MS), 10)
            : MEMVIEW_SLOMO_INTERVAL_MS;
        if (!isNaN(saved) && saved >= 50) MEMVIEW_SLOMO_INTERVAL_MS = saved;
        slomoInput.value = MEMVIEW_SLOMO_INTERVAL_MS;
        if (slomoVal) slomoVal.textContent = MEMVIEW_SLOMO_INTERVAL_MS;
        slomoInput.addEventListener("input", function() {
            let v = parseInt(this.value, 10);
            if (isNaN(v) || v < 50) v = 50;
            MEMVIEW_SLOMO_INTERVAL_MS = v;
            if (slomoVal) slomoVal.textContent = v;
            memview_slomo_restart_timer();   // apply immediately if slomo is running
            if (typeof save_setting === "function") save_setting("memview_slomo_interval", v);
        });
    }

    // auto-select (follow mode): keep the detail view locked to the top-of-list
    // bitplane while enabled; toggling it on re-locks onto the current bpl1
    let bplAutoCb = document.getElementById("memview_bpl_autoselect");
    if (bplAutoCb) {
        memview_bpl_autoselect = bplAutoCb.checked;
        bplAutoCb.addEventListener("change", function() {
            memview_bpl_autoselect = this.checked;
            if (memview_bpl_autoselect) {
                memview_bpl_sel_sig = null;      // force a re-select on next refresh
                memview_bpl_recent.length = 0;   // forget the page-flip history
                memview_refresh_bitplanes(true);
            }
        });
    }
    let bplList = document.getElementById("memview_bpl_list");
    if (bplList) {
        bplList.addEventListener("mouseenter", function() { memview_bpl_hover = true; });
        bplList.addEventListener("mouseleave", function() { memview_bpl_hover = false; });
    }

    // keep the panel below the navbar while it is visible, full height otherwise
    if (typeof $ !== "undefined") {
        $("#navbar").on("shown.bs.collapse", memview_update_top);
        $("#navbar").on("hide.bs.collapse", function() {
            // navbar is collapsing -> reclaim the full height immediately
            let panel = document.getElementById("memview_panel");
            if (panel) panel.style.top = "0px";
        });
    }
    window.addEventListener("resize", memview_update_top);

    memview_initialized = true;
}

function is_running_safe() {
    return typeof running !== "undefined" && running;
}

// suppress text/canvas selection while a drag is in progress (some browsers
// invert the canvas colors when it becomes part of a selection)
function memview_begin_drag() {
    document.body.classList.add("memview-dragging");
    let sel = window.getSelection && window.getSelection();
    if (sel && sel.removeAllRanges) { try { sel.removeAllRanges(); } catch (e) {} }
}
function memview_end_drag() {
    document.body.classList.remove("memview-dragging");
}

// positions the panel directly below the navbar while it is visible,
// otherwise lets it use the full viewport height
function memview_update_top() {
    let panel = document.getElementById("memview_panel");
    if (!panel) return;
    let nav = document.getElementById("navbar");
    // note: offsetParent is always null for position:fixed elements, so we
    // detect visibility via offsetHeight (0 when the collapse is hidden)
    let visible = nav && nav.offsetHeight > 0;
    panel.style.top = visible ? nav.getBoundingClientRect().bottom + "px" : "0px";
    memview_update_bottom();
}

// when the activity monitor grid is visible, stop the panel right above it so
// its vertical end lines up exactly with the top of the monitor grid
function memview_update_bottom() {
    let panel = document.getElementById("memview_panel");
    if (!panel) return;
    let activity = document.getElementById("activity");
    let h = (activity && activity.offsetHeight > 0) ? activity.offsetHeight : 0;
    panel.style.bottom = h + "px";
}

function memview_set_start(addr, keepPressed) {
    if (addr < 0) addr = 0;
    memdump_start = addr & 0xfffffe;   // word aligned
    if (!keepPressed) memview_pressed = false;
}

// (re)allocates the detail canvas/backbuffer for the current words-per-row
function memview_apply_geometry() {
    let canvas = document.getElementById("memview_canvas");
    if (!canvas || !memview_ctx) return;
    memview_hpixels = memview_words_per_row * 16;
    canvas.width = memview_hpixels;
    canvas.height = MEMVIEW_VPIXELS;
    memview_image_data = memview_ctx.createImageData(memview_hpixels, MEMVIEW_VPIXELS);
    memview_buffer = new Uint8Array(memview_hpixels * MEMVIEW_VPIXELS * 4);
}

// switches the detail view to a specific bitplane geometry:
//   words = visible words per scanline, strideBytes = memory advance per row
//   (words*2 + modulo, so interleaved planes and modulo gaps are skipped)
function memview_set_geometry(words, strideBytes) {
    words = Math.max(1, Math.min(words | 0, 512));
    memview_words_per_row = words;
    memview_row_stride = Math.max(2, strideBytes | 0) & 0xfffffe;
    memview_apply_geometry();
}

// restores the default contiguous 16-words-per-row layout
function memview_reset_geometry() {
    memview_words_per_row = MEMVIEW_WORDS_PER_ROW;
    memview_row_stride = MEMVIEW_BYTES_PER_ROW;
    memview_apply_geometry();
}

function memview_toggle() {
    // dismiss the button's bootstrap tooltip so it doesn't linger and cover the
    // panel/canvas after the click. on touch, tapping the button focuses it, so
    // bootstrap (trigger "hover focus") re-shows the tooltip on focus right
    // after our hide - with a mouse there is no focus-on-tap so it stays hidden.
    // blur the button (removes the focus that triggers the re-show) and hide
    // again on the next tick via the shared helper to defeat that re-show.
    let el = document.getElementById("button_memview");
    if (el && el.blur) el.blur();
    if (typeof hide_all_tooltips === "function") {
        hide_all_tooltips();
        setTimeout(hide_all_tooltips, 0);
    }
    if (memview_open) memview_close_panel();
    else memview_open_panel();
}

function memview_open_panel() {
    let panel = document.getElementById("memview_panel");
    if (!panel) return;
    panel.style.display = "flex";
    memview_open = true;
    memview_init();
    memview_update_regions();
    memview_update_top();
    // start recording bitplane DMA accesses so the guesser has fresh data
    if (typeof wasm_set_bitplane_guess === "function") wasm_set_bitplane_guess(1);
    // start write-owner tracking (blitter vs cpu on chip ram) only while the
    // panel is open; the heatmap fade decides the coloring from here on
    if (typeof wasm_set_write_tracking === "function") wasm_set_write_tracking(1);
    memview_heat.clear();
    memview_heat_start = null;
    memview_heat_stride = null;
    memview_bpl_last_raw = null;
    memview_refresh_bitplanes(true);
    if (typeof scaleVMCanvas === "function") scaleVMCanvas();
    memdump();
    if (typeof save_setting === "function") save_setting("memview_open", true);
}

function memview_close_panel() {
    let panel = document.getElementById("memview_panel");
    if (panel) panel.style.display = "none";
    memview_open = false;
    // cancel a running slomo and return to normal speed
    memview_slomo_stop(true);
    // stop recording to avoid the small per-write/per-fetch overhead when the
    // panel is closed
    if (typeof wasm_set_bitplane_guess === "function") wasm_set_bitplane_guess(0);
    if (typeof wasm_set_write_tracking === "function") wasm_set_write_tracking(0);
    if (typeof scaleVMCanvas === "function") scaleVMCanvas();
    if (typeof save_setting === "function") save_setting("memview_open", false);
}

// backward-compatible alias for the former "guess" button (pre-rebuild html)
function memview_guess_bitplanes() { memview_refresh_bitplanes(true); }

// single-step button handler: if slomo is active, cancel it and
// stay paused in manual single-step mode; then advance exactly one frame.
function memview_step_button() {
    if (memview_slomo_timer !== null) memview_slomo_stop(false); // stop slow-mo, no resume
    memview_step_frame();
}

// advances the emulation by exactly one frame. the emulator is paused first
// (so it stays frozen between clicks) and the freshly computed frame is
// rendered to the amiga canvas, memory view and bitplane list.
function memview_step_frame() {
    // pause the run loop; route through button_run_click so the toolbar's
    // run/pause icon stays in sync with the actual state
    if (is_running_safe() && typeof app !== "undefined" &&
        typeof app.button_run_click === "function") {
        app.button_run_click();
    }
    // compute exactly one frame synchronously
    if (typeof Module !== "undefined" && typeof Module._wasm_execute === "function") {
        Module._wasm_execute();
    }
    // draw the new frame to the amiga canvas
    let now = (typeof performance !== "undefined") ? performance.now() : 0;
    if (typeof render_frame === "function") {
        render_frame(now);
    } else if (typeof current_renderer !== "undefined" && current_renderer === "gpu shader" &&
               typeof render_canvas_gl === "function") {
        render_canvas_gl(now);
    } else if (typeof render_canvas === "function") {
        render_canvas(now);
    }
    // refresh the memory view and detected bitplane areas for this frame
    memdump();
    memview_refresh_bitplanes(true);
    // the activity monitor interval skips paused frames, so update it here too
    if (typeof update_activity_monitors === "function") update_activity_monitors();
}

// --- slomo: slow-motion single stepping -----------------------------------
// executes one frame every MEMVIEW_SLOMO_INTERVAL_MS and keeps going until the
// button is clicked again, which resumes normal running speed.
var MEMVIEW_SLOMO_INTERVAL_MS = 500;     // one single-step every 500ms; user-adjustable
var memview_slomo_timer = null;
// whether the emulator was running when slomo started. if it was already
// paused, stopping slomo must leave it paused (don't force a resume)
var memview_slomo_was_running = false;

function memview_slomo_step() {
    memview_step_frame();   // pauses the run loop on the first call, then steps
}

function memview_slomo_toggle() {
    // second press while active: stop and resume normal speed
    if (memview_slomo_timer !== null) { memview_slomo_stop(true); return; }

    let slomoBtn = document.getElementById("memview_slomo");
    if (slomoBtn) slomoBtn.classList.add("slomo_active");

    // remember the pre-slomo run state so we can restore it on stop
    memview_slomo_was_running = (typeof is_running_safe === "function") ? is_running_safe() : false;

    memview_slomo_step();   // immediate first step for responsiveness
    memview_slomo_timer = setInterval(memview_slomo_step, MEMVIEW_SLOMO_INTERVAL_MS);
}

// apply a changed interval right away if slomo is currently running
function memview_slomo_restart_timer() {
    if (memview_slomo_timer === null) return;
    clearInterval(memview_slomo_timer);
    memview_slomo_timer = setInterval(memview_slomo_step, MEMVIEW_SLOMO_INTERVAL_MS);
}

function memview_slomo_stop(resume) {
    if (memview_slomo_timer !== null) {
        clearInterval(memview_slomo_timer);
        memview_slomo_timer = null;
    }
    let slomoBtn = document.getElementById("memview_slomo");
    if (slomoBtn) slomoBtn.classList.remove("slomo_active");
    // return to normal running speed only if the emulator was running before
    // slomo started (memview_step_frame paused the loop). if it was already
    // paused, stay paused.
    if (resume && memview_slomo_was_running && !is_running_safe() &&
        typeof app !== "undefined" && typeof app.button_run_click === "function") {
        app.button_run_click();
    }
}

// throttled per-frame driver: refreshes the bitplane list while the panel is open
function memview_bpl_tick() {
    if (!memview_open) return;
    // one rendered emulation frame -> advance the heatmap fade clock. this is
    // the single per-frame hook (called from render_frame for live, single step
    // and slomo), so the fade only progresses when frames are actually produced.
    memview_frame_seq++;
    if ((++memview_bpl_counter % MEMVIEW_BPL_THROTTLE) !== 0) return;
    memview_refresh_bitplanes(false);
}

// reads the recorded bitplane DMA ranges of the last frame from the core and
// lists them as clickable "possible bitplane areas" (jump into the detail view).
// pass force=true to rebuild regardless of the hover/unchanged guards.
function memview_refresh_bitplanes(force) {
    let list = document.getElementById("memview_bpl_list");
    if (!list) return;
    if (typeof wasm_get_bitplane_areas !== "function") return;
    // don't rebuild under the cursor (would make entries jump away on click)
    if (!force && memview_bpl_hover) return;

    let raw = wasm_get_bitplane_areas() || "";
    // skip the DOM work when nothing changed since the last render
    if (!force && raw === memview_bpl_last_raw) return;
    memview_bpl_last_raw = raw;

    list.innerHTML = "";
    let entries = raw.split(";").filter(function(s) { return s.length > 0; });
    if (entries.length === 0) {
        let empty = document.createElement("div");
        empty.className = "memview_bpl_empty";
        empty.textContent = "no bitplane dma detected \u2013 run a graphical program";
        list.appendChild(empty);
        memview_bpl_sel_sig = null;
        memview_bpl_recent.length = 0;
        return;
    }

    let firstSel = null;   // dominant listed entry -> tracked by follow mode

    // parse + validate, then drop fragments too short to be a real image
    let parsed = [];
    for (let i = 0; i < entries.length; i++) {
        let parts = entries[i].split(",");
        let plane = parseInt(parts[0], 10);
        let start = parseInt(parts[1], 10);
        let end = parseInt(parts[2], 10);
        let mod = parseInt(parts[3], 10);
        let words = parseInt(parts[4], 10);
        let lines = parseInt(parts[5], 10);
        if (isNaN(start) || isNaN(end)) continue;
        if (isNaN(words) || words < 1) words = MEMVIEW_WORDS_PER_ROW;
        if (isNaN(mod)) mod = 0;
        let widthPx = words * 16;                 // one bit per pixel
        let stride = words * 2 + mod;             // memory bytes per scanline
        // height = number of scanlines the core actually did bitplane dma on.
        // this is layout-independent and works even when the copper reloads
        // bplpt every line. fall back to the address-range estimate if the
        // core does not report a line count (older build).
        let heightPx;
        if (!isNaN(lines) && lines > 0) {
            heightPx = lines;
        } else {
            heightPx = stride > 0
                ? Math.max(1, Math.round((end - start - words * 2) / stride) + 1)
                : 1;
        }
        // drop transitional / fragment detections: only keep areas tall enough
        // to be a real, viewable bitplane image
        if (heightPx < MEMVIEW_BPL_MIN_HEIGHT) continue;
        parsed.push({ plane: plane, start: start, mod: mod, words: words,
                      widthPx: widthPx, heightPx: heightPx });
    }

    // keep the bpl1..bpl6 grouping, but put the tallest (dominant) area of each
    // plane on top of its group -> easy to find, and follow mode locks onto the
    // main image instead of a small segment
    parsed.sort(function(a, b) {
        if (a.plane !== b.plane) return a.plane - b.plane;
        return b.heightPx - a.heightPx;
    });

    if (parsed.length === 0) {
        let empty = document.createElement("div");
        empty.className = "memview_bpl_empty";
        empty.textContent = "no bitplane dma detected \u2013 run a graphical program";
        list.appendChild(empty);
        memview_bpl_sel_sig = null;
        memview_bpl_recent.length = 0;
        return;
    }

    // per-plane segment counts (to label "k/n" when a plane has several areas)
    let planeCount = {};
    for (let e of parsed) planeCount[e.plane] = (planeCount[e.plane] || 0) + 1;
    let planeSeen = {};

    for (let e of parsed) {
        let plane = e.plane, start = e.start, mod = e.mod, words = e.words;
        let widthPx = e.widthPx, heightPx = e.heightPx;
        let n = planeCount[plane];
        let k = (planeSeen[plane] = (planeSeen[plane] || 0) + 1);
        let segLabel = n > 1 ? " (" + k + "/" + n + ")" : "";

        let item = document.createElement("div");
        item.className = "memview_bpl_item";
        item.title = "jump to bitplane " + plane + segLabel + " \u00b7 " +
            widthPx + "x" + heightPx + " px \u00b7 modulo " + mod;
        item.innerHTML =
            "<span class='memview_bpl_pl'>bpl" + (plane + 1) + "</span>" +
            "<span class='memview_bpl_addr'>$" + ("000000" + start.toString(16)).slice(-6) + "</span>" +
            "<span class='memview_bpl_meta'>" + widthPx + "\u00d7" + heightPx + segLabel + "</span>";
        let addrEl = item.querySelector(".memview_bpl_addr");
        (function(addr, w, m, ael) {
            item.addEventListener("click", function() {
                memview_select_bpl(addr, w, m, ael);
            });
        })(start, words, mod, addrEl);
        list.appendChild(item);

        if (!firstSel) firstSel = { start: start, words: words, mod: mod, addrEl: addrEl };
    }

    // auto-select (follow mode): keep the detail view locked to the top-of-list
    // bitplane and re-jump whenever its address/geometry changes. suppressed
    // while the user is dragging (detail-view or overview scroll) so an active
    // manual inspection is never yanked away.
    if (memview_bpl_autoselect && firstSel && !memview_pressed && !mempreview_pressed) {
        let sig = firstSel.start + "," + firstSel.words + "," + firstSel.mod;
        if (sig !== memview_bpl_sel_sig) {
            // double-buffer guard: if bpl1's base address keeps returning to a
            // value we saw a few frames ago, the program is page-flipping between
            // a small set of buffers (A,B,A,B…). stay locked on the current view
            // instead of jumping every frame. a genuinely new address (e.g. a
            // smooth scroll or a real screen change) is never in the history, so
            // it still follows normally.
            if (memview_bpl_recent.indexOf(sig) === -1) {
                memview_bpl_sel_sig = sig;
                memview_select_bpl(firstSel.start, firstSel.words, firstSel.mod, firstSel.addrEl);
            }
        }
        // record the observed signature (ring buffer) so page-flips age out
        memview_bpl_recent.push(sig);
        if (memview_bpl_recent.length > MEMVIEW_BPL_RECENT_MAX) memview_bpl_recent.shift();
    }
}

// jumps the detail view to a detected bitplane and plays the "pop" highlight on
// both the clicked list address and the detail start-address input, so it is
// visible what just got selected (used by manual clicks and auto-select).
// width = words per line, stride = line bytes + modulo (skips the modulo gap /
// interleaved planes) -> clean bitplane image
function memview_select_bpl(addr, words, mod, addrEl) {
    memview_set_geometry(words, words * 2 + mod);
    memview_set_start(addr);
    memdump();
    mempreview();
    if (addrEl) memview_flash(addrEl);
    memview_flash(document.getElementById("memview_start"));
}

// retriggerable pop+highlight animation (see .memview_flash in vAmiga.css)
function memview_flash(el) {
    if (!el) return;
    el.classList.remove("memview_flash");
    void el.offsetWidth;               // force reflow so the animation restarts
    el.classList.add("memview_flash");
    el.addEventListener("animationend", function handler() {
        el.classList.remove("memview_flash");
        el.removeEventListener("animationend", handler);
    });
}

// called from the emulator frame loop and on manual updates
function memdump() {
    if (!memview_open || !memview_initialized) return;
    if (typeof wasm_peek16 !== "function") return;
    memdump_do(memdump_start, memdump_col1, memdump_col2);
}

// per-channel linear interpolation between two 0xAARRGGBB colors (t in [0,1])
function memview_lerp_color(c0, c1, t) {
    let a0 = (c0 >>> 24) & 255, r0 = (c0 >>> 16) & 255, g0 = (c0 >>> 8) & 255, b0 = c0 & 255;
    let a1 = (c1 >>> 24) & 255, r1 = (c1 >>> 16) & 255, g1 = (c1 >>> 8) & 255, b1 = c1 & 255;
    let a = (a0 + (a1 - a0) * t + 0.5) | 0;
    let r = (r0 + (r1 - r0) * t + 0.5) | 0;
    let g = (g0 + (g1 - g0) * t + 0.5) | 0;
    let b = (b0 + (b1 - b0) * t + 0.5) | 0;
    return ((a << 24) | (r << 16) | (g << 8) | b) >>> 0;
}

function memdump_do(start0, col1, col2) {
    let start = start0 < 0 ? 0 : start0;
    let writers = memview_show_writers && typeof wasm_get_write_owner === "function";
    // fast/slow ram can only be written by the cpu (the blitter/chipset cannot
    // reach it), so any such address is cpu; collect their ranges once.
    let cpuRanges = [];
    if (writers) {
        for (let i = 0; i < memview_regions.length; i++) {
            let r = memview_regions[i];
            if (r.name === "fast" || r.name === "slow") {
                cpuRanges.push([r.base, r.base + r.size]);
            }
        }
    }
    let isCpuOnly = function(a) {
        for (let i = 0; i < cpuRanges.length; i++) {
            if (a >= cpuRanges[i][0] && a < cpuRanges[i][1]) return true;
        }
        return false;
    };
    // heatmap decay: the fade is driven by the rendered-frame counter, so it
    // freezes while the emulation is paused. the state is keyed by absolute
    // address, so it is kept across scrolling/dragging - that way hot writes
    // stay visible when you drag the (paused) view instead of vanishing the
    // moment the window moves. off-screen entries are simply not drawn; bound
    // the map's growth by dropping fully faded (cold) entries once it gets big.
    let seq = memview_frame_seq;
    if (writers && memview_heat.size > 200000) {
        for (let [k, rec] of memview_heat) {
            if (seq - rec.f >= MEMVIEW_HEAT_FADE_FRAMES) memview_heat.delete(k);
        }
    }
    for (let y = 0; y < MEMVIEW_VPIXELS; y++) {
        let addr = start + y * memview_row_stride;
        for (let w = 0; w < memview_words_per_row; w++) {
            let a = addr + w * 2;
            let value = wasm_peek16(a);
            let c1 = col1, c2 = col2;
            if (writers) {
                // who last wrote this cell (only the blitter/chip can be blue;
                // fast/slow and everything else is attributed to the cpu)
                let ownerTag;
                if (isCpuOnly(a)) ownerTag = MEMVIEW_WRITE_CPU;
                else ownerTag = (wasm_get_write_owner(a) === MEMVIEW_WRITE_BLITTER)
                    ? MEMVIEW_WRITE_BLITTER : MEMVIEW_WRITE_CPU;

                // detect a fresh write by watching the value change. the first
                // time we see an address we record it silently (no flash on
                // open/scroll); a later change starts the fade at full heat.
                let rec = memview_heat.get(a);
                if (rec === undefined) {
                    rec = { v: value, f: -Infinity };
                    memview_heat.set(a, rec);
                } else if (value !== rec.v) {
                    rec.v = value;
                    rec.f = seq;
                }
                let heat = 1 - (seq - rec.f) / MEMVIEW_HEAT_FADE_FRAMES;
                if (heat > 0) {
                    if (heat > 1) heat = 1;
                    let base1 = (ownerTag === MEMVIEW_WRITE_BLITTER) ? memdump_blt_col1 : memdump_cpu_col1;
                    let base2 = (ownerTag === MEMVIEW_WRITE_BLITTER) ? memdump_blt_col2 : memdump_cpu_col2;
                    c1 = memview_lerp_color(col1, base1, heat);
                    c2 = memview_lerp_color(col2, base2, heat);
                }
                // heat <= 0 -> stays at the default amber palette (col1/col2)
            }
            memdump_plotword(w * 16, y, value, c1, c2);
        }
    }
    memview_image_data.data.set(memview_buffer);
    memview_ctx.putImageData(memview_image_data, 0, 0, 0, 0, memview_hpixels, MEMVIEW_VPIXELS);
    update_memdump_info(start);

    // refresh the overview: every frame while paused/manual, throttled while live
    if (!(live_memory_dump_enabled && is_running_safe())) {
        mempreview();
    } else if ((++mempreview_counter % 8) === 0) {
        mempreview();
    }
}

// reads the current ram configuration from the core and (re)builds the list of
// memory regions the overview visualizes (chip + optional fast + slow ram)
function memview_update_regions() {
    let kb = function(item) {
        if (typeof wasm_get_config_item !== "function") return 0;
        let v = parseInt(wasm_get_config_item(item));
        return isNaN(v) ? 0 : v;
    };
    let chip = kb("CHIP_RAM");
    let fast = kb("FAST_RAM");
    let slow = kb("SLOW_RAM");
    let defs = [];
    if (chip > 0) defs.push({ name: "chip", base: MEM_CHIP_BASE, size: chip * 1024 });
    if (slow > 0) defs.push({ name: "slow", base: MEM_SLOW_BASE, size: slow * 1024 });
    if (fast > 0) defs.push({ name: "fast", base: MEM_FAST_BASE, size: fast * 1024 });
    if (defs.length === 0) defs.push({ name: "chip", base: MEM_CHIP_BASE, size: 512 * 1024 });

    let signature = defs.map(function(d) { return d.name + d.size; }).join(",");
    if (signature !== memview_regions_signature) {
        memview_regions_signature = signature;
        memview_build_overview_dom(defs);
    } else {
        for (let i = 0; i < defs.length; i++) {
            memview_regions[i].base = defs[i].base;
            memview_regions[i].size = defs[i].size;
        }
    }
}

function memview_format_size(bytes) {
    let kb = bytes / 1024;
    return kb >= 1024 ? (kb / 1024) + "M" : kb + "K";
}

// (re)creates one labeled heatmap canvas per memory region, stacked vertically
function memview_build_overview_dom(defs) {
    let host = document.getElementById("memview_overview_blocks");
    if (!host) return;
    host.innerHTML = "";
    memview_regions = [];
    for (let i = 0; i < defs.length; i++) {
        let d = defs[i];
        let block = document.createElement("div");
        block.className = "memview_ov_block";
        block.style.flexGrow = String(d.size);

        let label = document.createElement("div");
        label.className = "memview_ov_label";
        label.textContent = d.name + " \u00b7 " + memview_format_size(d.size);

        let canvas = document.createElement("canvas");
        canvas.width = MEMPREVIEW_HPIXELS;
        canvas.height = MEMPREVIEW_BLOCK_ROWS;
        canvas.title = "click to jump into " + d.name + " ram";

        block.appendChild(label);
        block.appendChild(canvas);
        host.appendChild(block);

        let ctx = canvas.getContext("2d");
        let r = {
            name: d.name, base: d.base, size: d.size,
            canvas: canvas,
            ctx: ctx,
            image_data: ctx.createImageData(MEMPREVIEW_HPIXELS, MEMPREVIEW_BLOCK_ROWS),
            buffer: new Uint8Array(MEMPREVIEW_HPIXELS * MEMPREVIEW_BLOCK_ROWS * 4)
        };
        memview_regions.push(r);

        (function(region) {
            canvas.addEventListener("pointerdown", function(e) {
                e.preventDefault();
                memview_begin_drag();
                mempreview_pressed = true;
                memview_drag_region = region;
                memview_overview_jump(region, e);
            });
        })(r);
    }
}

// maps a click/drag position on a region block to a cpu address and jumps there
function memview_overview_jump(r, e) {
    let rect = r.canvas.getBoundingClientRect();
    let W = MEMPREVIEW_HPIXELS, H = MEMPREVIEW_BLOCK_ROWS;
    let px = Math.floor((e.clientX - rect.left) / rect.width * W);
    let py = Math.floor((e.clientY - rect.top) / rect.height * H);
    if (px < 0) px = 0; if (px >= W) px = W - 1;
    if (py < 0) py = 0; if (py >= H) py = H - 1;
    let idx = py * W + px;
    let addr = r.base + Math.floor(idx * (r.size / (W * H)));
    memview_reset_geometry();
    memview_set_start(addr);
    memdump();
    mempreview();
}

// draws each memory region as a heatmap; the visible detail window is tinted red
function mempreview() {
    if (typeof wasm_peek16 !== "function") return;
    memview_update_regions();
    for (let i = 0; i < memview_regions.length; i++) {
        memview_render_region(memview_regions[i]);
    }
}

function memview_render_region(r) {
    if (!r.ctx) return;
    let W = MEMPREVIEW_HPIXELS, H = MEMPREVIEW_BLOCK_ROWS;
    let bytes_per_px = r.size / (W * H);
    let winStart = memdump_start;
    let winEnd = memdump_start + MEMVIEW_VPIXELS * memview_row_stride;
    let buf = r.buffer;
    for (let y = 0; y < H; y++) {
        for (let x = 0; x < W; x++) {
            let idx = y * W + x;
            let addr = r.base + Math.floor(idx * bytes_per_px);
            let base_col = (addr >= winStart && addr < winEnd) ? 0xffff0000 : 0xff000000;
            let argb = base_col | wasm_peek16(addr);
            let o = idx * 4;
            buf[o + 0] = 0xff & (argb >> 16); // R
            buf[o + 1] = 0xff & (argb >> 8);  // G
            buf[o + 2] = 0xff & (argb);       // B
            buf[o + 3] = 0xff & (argb >> 24); // A
        }
    }
    r.image_data.data.set(buf);
    r.ctx.putImageData(r.image_data, 0, 0, 0, 0, W, H);
}

function memdump_plotword(x, y, word, col1, col2) {
    for (let b = 0; b < 16; b++) {
        memdumpset(x + b, y, (word & (0x8000 >> b)) ? col1 : col2);
    }
}

function memdumpset(x, y, argb) {
    let o = memview_hpixels * 4 * y + x * 4;
    memview_buffer[o + 0] = 0xff & (argb >> 16); // R
    memview_buffer[o + 1] = 0xff & (argb >> 8);  // G
    memview_buffer[o + 2] = 0xff & (argb);       // B
    memview_buffer[o + 3] = 0xff & (argb >> 24); // A
}

function update_memdump_info(start) {
    if (start === last_memdump_info_start) return;
    last_memdump_info_start = start;
    let el = document.getElementById("memview_start");
    if (el && document.activeElement !== el) {
        el.value = ("000000" + start.toString(16)).slice(-6);
    }
}
