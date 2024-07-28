const default_app_title="vAmiga - start screen";
let global_apptitle=default_app_title;
let call_param_openROMS=false;
let call_param_gpu=null;
let call_param_navbar=null;
let call_param_wide=null;
let call_param_border=null;
let call_param_touch=null;
let call_param_dark=null;
let call_param_buttons=[];
let call_param_dialog_on_missing_roms=null;
let call_param_dialog_on_disk=null;
let call_param_mouse=null;
let call_param_warpto=null;
let call_param_url=null;
let call_param_display=null;
let call_param_wait_for_kickstart_injection=null;
let call_param_kickstart_rom_url=null;
let call_param_kickstart_ext_url=null;

let startup_script_executed=false;
let on_ready_to_run=()=>{};
let on_hdr_step=(drive_number, cylinder)=>{};
let on_power_led_dim=()=>{};
let df_mount_list=[];//to auto mount disks from zip e.g. ["Batman_Rises_disk1.adf","Batman_Rises_disk2.adf"];
let hd_mount_list=[];

let virtual_keyboard_clipping = true; //keyboard scrolls when it clips
let use_wide_screen=false;
let use_ntsc_pixel=false;
let joystick_button_count=1;

let v_joystick=null;
let v_fire=null;
let fixed_touch_joystick_base=false;
let stationaryBase = false;

const AudioContext = window.AudioContext || window.webkitAudioContext;
let audioContext = new AudioContext();
let audio_connected=false;

let load_sound = async function(url){
    let response = await fetch(url);
    let buffer = await response.arrayBuffer();
    let audio_buffer= await audioContext.decodeAudioData(buffer);
    return audio_buffer;
} 
let parallel_playing=0;
let keyboard_sound_volumne=0.04;
let play_sound = function(audio_buffer, sound_volumne=0.1){
        if(audio_buffer== null)
        {                 
            load_all_sounds();
            return;
        }
        if(parallel_playing>2 && audio_buffer==audio_df_step)
        {//not more than 3 stepper sounds at the same time
            return;
        }
        const source = audioContext.createBufferSource();
        source.buffer = audio_buffer;

        let gain_node = audioContext.createGain();
        gain_node.gain.value = sound_volumne; 
        gain_node.connect(audioContext.destination);

        source.addEventListener('ended', () => {
            parallel_playing--;
        });
        source.connect(gain_node);
        parallel_playing++;
        source.start();
}   

let audio_df_insert=null;
let audio_df_eject=null;
let audio_df_step=null;
let audio_hd_step=null;
let audio_key_standard=null;
let audio_key_backspace=null;
let audio_key_space=null;

async function load_all_sounds()
{
    if(audio_df_insert==null)
        audio_df_insert=await load_sound('sounds/insert.mp3');
    if(audio_df_eject==null)
        audio_df_eject=await load_sound('sounds/eject.mp3');
    if(audio_df_step == null)
        audio_df_step=await load_sound('sounds/step.mp3');
    if(audio_hd_step == null)   
        audio_hd_step=await load_sound('sounds/stephd.mp3');
    if(audio_key_standard == null)   
        audio_key_standard=await load_sound('sounds/key_standard.mp3');
    if(audio_key_backspace == null)   
        audio_key_backspace=await load_sound('sounds/key_backspace.mp3');
    if(audio_key_space == null)   
        audio_key_space=await load_sound('sounds/key_space.mp3');
}
load_all_sounds();


const load_script= (url) => {
    return new Promise(resolve =>
    {
        let script = document.createElement("script")
        script.type = "text/javascript";
        script.onload = resolve;
        script.src = url;
        document.getElementsByTagName("head")[0].appendChild(script);
    });
}

function ToBase64_small(u8) 
{
    return btoa(String.fromCharCode.apply(null, u8));
}

function ToBase64(u8)
{
    return btoa(new Uint8Array(u8)
      .reduce((data, byte) => data + String.fromCharCode(byte), ''));
}


function FromBase64(str) {
    return atob(str).split('').map(function (c) { return c.charCodeAt(0); });
}

function html_encode(s) {
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/'/g, '&#39;').replace(/"/g, '&#34;');
}

function get_parameter_link()
{
    let parameter_link=null;
    let param_pos = window.location.href.indexOf('#');
    let param_part = decodeURIComponent(window.location.href.substring(param_pos));
    if( param_pos >= 0 && param_part.startsWith("#{") &&
    param_part.endsWith("}"))
    {//json notation 
        /*
        #{"url":"http://csdb.dk/getinternalfile.php/205771/CopperBooze.prg","buttons": [{"title":"hello","key":"h","script": "alert('d2hpbGUobm90X3N0b3BwZWQodGhpc19pZCkpIHthd2FpdCBhY3Rpb24oIkE9Pjk5OW1zIik7fQ');"}] }
        #{"buttons":[{"position":"top:10vh;left:90vw","title":"hello","key":"h","run":true,"script":"d2hpbGUobm90X3N0b3BwZWQodGhpc19pZCkpIHthd2FpdCBhY3Rpb24oIkE9Pjk5OW1zIik7fQ=="}]}
        */
        call_obj = JSON.parse(param_part.substring(1), (key, value) => {            
            console.log(key); 
            if(key=='script_base64')
            {//base64decode
                return atob(value);
            }
            return value;
        });
        parameter_link = call_obj.url;
        call_param_openROMS=call_obj.AROS === undefined ? null : call_obj.AROS;
        call_param_dialog_on_missing_roms = call_obj.dialog_on_missing_roms === undefined ? null : call_obj.dialog_on_missing_roms;
        call_param_dialog_on_disk = call_obj.dialog_on_disk === undefined ? null : call_obj.dialog_on_disk;
        call_param_navbar = call_obj.navbar === undefined ? null : call_obj.navbar==false ? "hidden": null;
        call_param_wide=call_obj.wide === undefined ? null : call_obj.wide;
        call_param_border=call_obj.border === undefined ? null : call_obj.border;
        call_param_touch=call_obj.touch === undefined ? null : call_obj.touch;
        call_param_dark=call_obj.dark === undefined ? null : call_obj.dark;
        call_param_warpto=call_obj.warpto === undefined ? null : call_obj.warpto;
        call_param_mouse=call_obj.mouse === undefined ? null : call_obj.mouse;
        call_param_display=call_obj.display === undefined ? null : call_obj.display;
        call_param_wait_for_kickstart_injection=call_obj.wait_for_kickstart_injection === undefined ? null : call_obj.wait_for_kickstart_injection;
        call_param_kickstart_rom_url=call_obj.kickstart_rom_url === undefined ? null : call_obj.kickstart_rom_url;
        call_param_kickstart_ext_url=call_obj.kickstart_ext_url === undefined ? null : call_obj.kickstart_ext_url;
        call_param_gpu = call_obj.gpu === undefined ? null : call_obj.gpu;

        if(call_obj.touch)
        {
            call_param_touch=true; 
        }
        if(call_obj.port1)
        {
            port1=call_param_touch == true ?"touch":"keys";          
            port2="none";
            $('#port1').val(port1);
            $('#port2').val(port2);
        }
        if(call_obj.port2)
        {
            port1="none";
            port2=call_param_touch == true ? "touch":"keys";
            $('#port1').val(port1);       
            $('#port2').val(port2);
        }
        
        if(call_obj.buttons !== undefined && call_param_buttons.length==0)
        {
            for(let b of call_obj.buttons)
            {
                if(b.position === undefined)
                {
                    b.position = "top:50vh;left:50vw";
                }
                if(b.script_base64 !== undefined){
                    b.script=b.script_base64;
                }
                if(b.lang === undefined)
                {
                    b.lang = "javascript";
                }
                b.transient = true;
                b.app_scope = true;
                b.id = 1000+call_param_buttons.length;
                call_param_buttons.push( b );
            }
        }
        if(call_obj.startup_script !== undefined && startup_script_executed==false)
        {
            try {
                new Function(`${call_obj.startup_script}`)();
                startup_script_executed=true;
            } catch (error) {
                console.error(`error in startup_script: ${call_obj.startup_script}`,e)
            }
        }
    }
    else
    {
        //hash style notation
        var call_url = window.location.href.split('#');
        if(call_url.length>1)
        {//there are # inside the URL
            //process settings 
            for(var i=1; i<call_url.length;i++)
            {//in case there was a # inside the parameter link ... rebuild that
                var token = call_url[i]; 
                
                if(parameter_link != null)
                {
                    parameter_link+="#"+token;
                }
                else if(token.startsWith("http"))
                {
                    parameter_link=token;
                }
                else
                { // it must be a setting
                    if(token.match(/AROS=true/i))
                    {
                        call_param_openROMS=true;
                    }
                    else if(token.match(/touch=true/i))
                    {
                        call_param_touch=true;
//                        register_v_joystick();
                    }
                    else if(token.match(/port1=true/i))
                    {
                        port1=call_param_touch != true ? "keys":"touch";          
                        port2="none";     
                        $('#port1').val(port1);
                        $('#port2').val(port2);
                    }
                    else if(token.match(/port2=true/i))
                    {
                        port1="none";
                        port2=call_param_touch != true ? "keys":"touch";
                        $('#port1').val(port1);       
                        $('#port2').val(port2);
                    }
                    else if(token.match(/navbar=hidden/i))
                    {
                        call_param_navbar='hidden';
                    }
                    else if(token.match(/wide=(true|false)/i))
                    {
                        call_param_wide=token.match(/.*(true|false)/i)[1].toLowerCase() == 'true';
                    }
                    else if(token.match(/border=(true|false)/i))
                    {
                        call_param_border=token.match(/.*(true|false)/i)[1].toLowerCase() == 'true';
                    }
                    else if(token.match(/border=([01]([.][0-9]*)?)/i))
                    {//border=0.3
                        call_param_border=token.match(/border=([01]([.][0-9]*)?)/i)[1];
                    }
                    else if(token.match(/dark=(true|false)/i))
                    {
                        call_param_dark=token.match(/.*(true|false)/i)[1].toLowerCase() == 'true';
                    }
                    else if(token.match(/dialog_on_missing_roms=(true|false)/i))
                    {
                        call_param_dialog_on_missing_roms=token.match(/.*(true|false)/i)[1].toLowerCase() == 'true';
                    }
                    else if(token.match(/dialog_on_disk=(true|false)/i))
                    {
                        call_param_dialog_on_disk=token.match(/.*(true|false)/i)[1].toLowerCase() == 'true';
                    }
                    else if(token.match(/warpto=([0-9]*)/i))
                    {
                        call_param_warpto=token.match(/warpto=([0-9]*)/i)[1];
                    }
                    else if(token.match(/mouse=(true|false)/i))
                    {
                        call_param_mouse=token.match(/.*(true|false)/i)[1].toLowerCase() == 'true';
                    }            
                    else if(token.match(/display=.*/i))
                    {
                        call_param_display=token.replace(/display=/i,""); 
                    }
                }
            }
        }
    }
    if(port1 == "touch" || port2 == "touch")
    {
        register_v_joystick();
    }

    call_param_url=parameter_link === undefined ? null : parameter_link;
    return parameter_link;
}


var parameter_link__already_checked=false;
var parameter_link_mount_in_df0=false;
async function load_parameter_link()
{
    if($('#modal_roms').is(":visible"))
    {
        return null;
    }

    if(parameter_link__already_checked)
      return null;
    
    parameter_link__already_checked=true;
    var parameter_link = get_parameter_link();
    if(parameter_link != null)
    {
        parameter_link_mount_in_df0=parameter_link.match(/[.](adf|hdf|dms|exe)$/i);
        //get_data_collector("csdb").run_link("call_parameter", 0,parameter_link);            
        let response = await fetch(parameter_link);
        file_slot_file_name = decodeURIComponent(response.url.match(".*/(.*)$")[1]);
        file_slot_file = new Uint8Array( await response.arrayBuffer());

        configure_file_dialog(reset=true);
    }
    return parameter_link;
}


var required_roms_loaded =false;

var last_drive_event=0;
var msg_callback_stack = []
function fire_on_message( msg, callback_fn)
{
    var handler = new Object();
    handler.message = msg;
    handler.callback_fn = callback_fn;
    msg_callback_stack.push(handler); 
}

function wait_on_message(msg)
{
    return new Promise(resolve => fire_on_message(msg, resolve));
}

window.serial_port_out_buffer="";
function set_serial_port_out_handler(new_handler){
	window.serial_port_out_handler = new_handler;  
}
set_serial_port_out_handler((data) => {
    //clear buffer when nobody consumes after 4kb of incoming data
    if(window.serial_port_out_buffer.length>4096)
    {
        window.serial_port_out_buffer="";
    }
    window.serial_port_out_buffer += String.fromCharCode( data & 0xff );
    //if we are embedded in a player send serial data to the host page
    window.parent.postMessage({ msg: 'serial_port_out', value: data},"*");
});
function message_handler(msg, data, data2)
{   
    queueMicrotask(()=>{
        message_handler_queue_worker( msg, data, data2 )
    });
}
function message_handler_queue_worker(msg, data, data2)
{
    //console.log(`js receives msg:${msg} data:${data}`);
    //UTF8ToString(cores_msg);
    if(msg == "MSG_READY_TO_RUN")
    {
        try{on_ready_to_run()} catch(error){console.error(error)}
        if(call_param_warpto !=null && call_param_url==null){
            wasm_configure("warp_to_frame", `${call_param_warpto}`);
        }
        //start it async
        //setTimeout(()=>{try{wasm_run();}catch(e){}},100);
        setTimeout(async function() { 
            try{
                if(call_param_navbar=='hidden')
                {
                    setTimeout(function(){
                        $("#button_show_menu").click();
                    },500);
                }
                let url = await load_parameter_link();

                if(url == null && hd_mount_list.length==0 && df_mount_list.length==0)
                { //when there is no media url to load, power on the Amiga directly here
                  //otherwise it will be started after media is inserted 
                  setTimeout(function(){
                    try{wasm_run();running=true;}catch(e){}
                  },100);
                } 
                
                if(call_param_wide != null)
                {
                    use_wide_screen = call_param_wide;
                    scaleVMCanvas();
                    wide_screen_switch.prop('checked', use_wide_screen);
                }
                if(call_param_display != null)
                {
                    set_display_choice(call_param_display);
                }
            }catch(e){}},
        0);
    }
    else if(msg == "MSG_ROM_MISSING")
    {        
        //try to load roms from local storage
        setTimeout(async function() {
            if(call_param_wait_for_kickstart_injection)
            {
                //don't auto load existing kick roms
                //instead wait for external commands
            }
            else if(call_param_kickstart_rom_url != null)
            {//this needs to be samesite or cross origin with CORS enabled
                let byteArray=new Uint8Array(await (await fetch(call_param_kickstart_rom_url)).arrayBuffer());
                wasm_loadfile("kick.rom_file", byteArray);
                if(call_param_kickstart_ext_url != null)
                {
                    wasm_loadfile("kick.rom_ext_file", new Uint8Array(await (await fetch(call_param_kickstart_ext_url)).arrayBuffer()));
                }
                wasm_reset();
            }
            //try to load currently user selected kickstart
            else if(false == await load_roms(true))
            {
                get_parameter_link(); //just make sure the parameters are set
                if(call_param_openROMS==true)
                {
                    await fetchOpenROMS();        
                }
                else if(call_param_dialog_on_missing_roms != false)
                {
                    $('#modal_roms').modal();
                }
            }
        },0);
 
    }
    else if(msg == "MSG_RUN")
    {
        required_roms_loaded=true;
        emulator_currently_runs=true;
    }
    else if(msg == "MSG_PAUSE")
    {
        emulator_currently_runs=false;
    }
    else if(msg == "MSG_VIDEO_FORMAT")
    {
        $('#ntsc_pixel_ratio_switch').prop('checked', data==1);  
    }
    else if(msg == "MSG_DRIVE_STEP" || msg == "MSG_DRIVE_POLL")
    {
        if(wasm_has_disk("df"+data)){
            play_sound(audio_df_step);
            $("#drop_zone").html(`df${data} ${data2.toString().padStart(2, '0')}`);
        }
        else if (data==0)
        {//only for df0: play stepper sound in case of no disk
            play_sound(audio_df_step);
        }
    }
    else if(msg == "MSG_DISK_INSERT")
    {
        play_sound(audio_df_insert); 
    }
    else if(msg == "MSG_DISK_EJECT")
    {
        $("#drop_zone").html(`file slot`);
        play_sound(audio_df_eject); 
    }
    else if(msg == "MSG_HDR_STEP")
    {
        play_sound(audio_hd_step);
        $("#drop_zone").html(`dh${data} ${data2}`);
        on_hdr_step(data, data2);
    }
    else if(msg == "MSG_POWER_LED_DIM")
    {
        on_power_led_dim();
    }
    else if(msg == "MSG_SNAPSHOT_RESTORED")
    {
        let v=wasm_get_config_item("BLITTER_ACCURACY");
        $(`#button_OPT_BLITTER_ACCURACY`).text(`blitter accuracy=${v} (snapshot)`);
        
        v=wasm_get_config_item("DRIVE_SPEED");
        $(`#button_OPT_DRIVE_SPEED`).text(`drive speed=${v} (snapshot)`);

        v=wasm_get_config_item("CPU_REVISION");
        $(`#button_OPT_CPU_REVISION`).text(`CPU=680${v}0 (snapshot)`);
        v=wasm_get_config_item("CPU_OVERCLOCKING");
        $(`#button_OPT_CPU_OVERCLOCKING`).text(`${Math.round((v==0?1:v)*7.09)} MHz (snapshot)`);
        v=wasm_get_config_item("AGNUS_REVISION");
        let agnus_revs=['OCS_OLD','OCS','ECS_1MB','ECS_2MB'];
        $(`#button_OPT_AGNUS_REVISION`).text(`agnus revision=${agnus_revs[v]} (snapshot)`);

        v=wasm_get_config_item("DENISE_REVISION");
        let denise_revs=['OCS','ECS'];
        $(`#button_OPT_DENISE_REVISION`).text(`denise revision=${denise_revs[v]} (snapshot)`);
      
        $(`#button_${"OPT_CHIP_RAM"}`).text(`chip ram=${wasm_get_config_item('CHIP_RAM')} KB (snapshot)`);
        $(`#button_${"OPT_SLOW_RAM"}`).text(`slow ram=${wasm_get_config_item('SLOW_RAM')} KB (snapshot)`);
        $(`#button_${"OPT_FAST_RAM"}`).text(`fast ram=${wasm_get_config_item('FAST_RAM')} KB (snapshot)`);
    
        rom_restored_from_snapshot=true;
    } 
    else if(msg == "MSG_SER_OUT")
    {
      serial_port_out_handler(data);
    }
    else if(msg == "MSG_CTRL_AMIGA_AMIGA")
    {
        setTimeout(release_modifiers, 0);
        wasm_reset();
    }
}

async function fetchOpenROMS(){
    var installer = async function(suffix, response) {
        try{
            var arrayBuffer = await response.arrayBuffer();
            var byteArray = new Uint8Array(arrayBuffer);
            var rom_url_path = response.url.split('/');
            var rom_name = rom_url_path[rom_url_path.length-1];

            var romtype = wasm_loadfile(rom_name+suffix, byteArray);
            if(romtype != "")
            {
                local_storage_set(romtype, rom_name);
                await save_rom(rom_name,romtype, byteArray);
                await load_roms(true);
            }
        } catch {
            console.log ("could not install system rom file");
            fill_rom_icons();
            fill_ext_icons();
        }  
    }
    
    let response = await fetch("roms/aros.bin");
    await installer('.rom_file', response);
    response = await fetch("roms/aros_ext.bin");
    await installer('.rom_ext_file', response);   
}


function fill_rom_icons(){
    let rom_infos=JSON.parse(wasm_rom_info());
    let icon="img/rom.png";
    if(rom_infos.hasRom == "false")
    {
        icon="img/rom_empty.png";
    }
    else if(rom_infos.romTitle.toLowerCase().indexOf("aros")>=0)
    {
        icon="img/rom_aros.png";
    }
    else if(rom_infos.romTitle.toLowerCase().indexOf("unknown")>=0)
    {
        icon="img/rom_unknown.png";
    }
    else if(rom_infos.romTitle.toLowerCase().indexOf("patched")>=0)
    {
        icon="img/rom_patched.png";
    }
    else if(rom_infos.romTitle.toLowerCase().indexOf("hyperion")>=0)
    {
        icon="img/rom_hyperion.png";
    }
    else
    {
        icon="img/rom_original.png";
    }

    $("#rom_kickstart").attr("src", icon);
    $("#kickstart_title").html(`${rom_infos.romTitle}<br>${rom_infos.romVersion}<br>${rom_infos.romReleased}<br>${rom_infos.romModel}`);

    $("#button_delete_kickstart").show();
}

function fill_ext_icons()
{
    let rom_infos=JSON.parse(wasm_rom_info());
    let icon="img/rom.png";
    if(rom_infos.hasExt == "false")
    {
        icon="img/rom_empty.png";
    }
    else if(rom_infos.extTitle.toLowerCase().indexOf("aros")>=0)
    {
        icon="img/rom_aros.png";
    }
    else if(rom_infos.extTitle.toLowerCase().indexOf("unknown")>=0)
    {
        icon="img/rom_unknown.png";
    }
    else if(rom_infos.extTitle.toLowerCase().indexOf("patched")>=0)
    {
        icon="img/rom_patched.png";
    }
    else if(rom_infos.romTitle.toLowerCase().indexOf("hyperion")>=0)
    {
        icon="img/rom_hyperion.png";
    }
    else
    {
        icon="img/rom_original.png";
    }

    $("#rom_kickstart_ext").attr("src", icon);
    $("#kickstart_ext_title").html(`${rom_infos.extTitle}<br>${rom_infos.extVersion}<br>${rom_infos.extReleased}<br>${rom_infos.extModel}`);

    $("#button_delete_kickstart_ext").show();
}

/**
* load_roms
  if we open ROM-Dialog then we could 
        A) show current roms in c64 instance,
        or
        B) saved roms in local storage ... 
    
        we choose A) because there is no method in the core for B) , 
 *
 * 
 * @param {*} install_to_core true when we should load roms from local storage into the core. 
 *
 * TODO: maybe split up functionality into load_roms() and refresh_rom_dialog() 
 */

async function load_roms(install_to_core){
    var loadStoredItem= async function (item_name, type){
        if(item_name==null)
            return null;
        //var stored_item = local_storage_get(item_name); 
        let stored_item = await load_rom(item_name);
        if(stored_item != null)
        {
            //var restoredbytearray = Uint8Array.from(FromBase64(stored_item));
            let restoredbytearray = stored_item.data;
            if(install_to_core)
            {

                romtype = wasm_loadfile(item_name+type, restoredbytearray);
                if(!romtype.endsWith("rom") && !romtype.endsWith("rom_ext"))
                {//in case the core thinks rom is not valid anymore delete it
                    delete_rom(item_name);
//                    local_storage_remove(item_name);
                    return null;
                }
            }
            return restoredbytearray;
        }
        else
        {
            return null;
        }
    }

    fill_available_roms('rom', 'stored_roms');
    fill_available_roms('rom_ext', 'stored_exts');       

    
    var all_fine = true;
    try{
        let selected_rom=local_storage_get('rom');
        let the_rom=await loadStoredItem(selected_rom, ".rom_file");
        if (the_rom==null){
            all_fine=false;
            $("#rom_kickstart").attr("src", "img/rom_empty.png");
            $("#kickstart_title").html("empty socket<br>(required)");

            $("#button_delete_kickstart").hide();
        }
        else
        {
            fill_rom_icons();
        }
    } catch(e){
        all_fine=false; //maybe it needs a rom extension file
        console.error(e);
    }
    try{
        let selected_rom_ext=local_storage_get('rom_ext');
        let the_rom_ext=await loadStoredItem(selected_rom_ext,".rom_ext_file");
        if (the_rom_ext==null){
            $("#rom_kickstart_ext").attr("src", "img/rom_empty.png");
            $("#kickstart_ext_title").html("empty socket<br>(optional)");

            $("#button_delete_kickstart_ext").hide();
        }
        else
        {
            fill_ext_icons();
        }
    } catch(e){
        console.error(e);
    }

    return all_fine;
}

function dragover_handler(ev) {
    ev.preventDefault();
    ev.stopPropagation();
    ev.dataTransfer.dropEffect = 'copy';
}

async function drop_handler(ev) {
    ev.preventDefault();
    ev.stopPropagation();

    var dt = ev.dataTransfer;
 
    if( dt.items ) {
        for (item of dt.items) {
            if (item.kind == "file") 
            {
                var f = item.getAsFile();
                pushFile(f);
                break;
            }
            else if (item.kind == "string") 
            {
                var dropped_uri = dt.getData("text"); //dt.getData("text/uri-list");
                //e.g. dropped_uri=https://csdb.dk/release/download.php?id=244060"

/*   old way ...
                var dropped_html = dt.getData("text/html");
                //              e.g. dropped_html =
                //                "<meta http-equiv="Content-Type" content="text/html;charset=UTF-8">
                //                <a href="https://csdb.dk/release/download.php?id=244060">http://csdb.dk/getinternalfile.php/205910/joyride.prg</a>"
                var dropped_id_and_name = dropped_html.match(`id=([0-9]+)">(https?://csdb.dk/getinternalfile.php/.*?)</a>`); 
                if(dropped_id_and_name != null && dropped_id_and_name.length>1)
                {
                    var id = dropped_id_and_name[1];
                    var title_name = dropped_id_and_name[2].split("/");
                    title_name = title_name[title_name.length-1];
                    var parameter_link = dropped_id_and_name[2];
                    setTimeout(() => {
                        get_data_collector("csdb").run_link(title_name, id ,parameter_link);            
                    }, 200);
                }
*/
                if(dropped_uri.startsWith("https://csdb.dk/release/download.php?id="))
                {
                    setTimeout(() => {
                        get_data_collector("csdb").run_link("call_parameter", -1,dropped_uri);            
                    }, 150);
                }
                else if(dropped_uri.startsWith("https://csdb.dk/release/?id="))
                {
                    $('#snapshotModal').modal('show');                    

                    //current_browser_datasource
                    await switch_collector("csdb");
                    current_browser_datasource="csdb";

                    $('#search').val(dropped_uri);
                    document.getElementById('search_symbol').click();
                }
                else
                {
                    alert("Sorry only amiga disk files are currently supported by vAmigaWeb ...");
                }
                break;
            }
        }
    }
    else {
        for (var i=0; i < dt.files.length; i++) {
            pushFile(dt.files[i]);
            break;
        }
    }
}

function handleFileInput(event) 
{
    var myForm = document.getElementById('theFileInput');
    var myfiles = myForm.elements['theFileDialog'].files;
    for (var i=0; i < myfiles.length; i++) {
        pushFile(myfiles[i]);
        break;
    }
    return false;
}


file_slot_file_name = null;
file_slot_file =null;

last_zip_archive_name = null
last_zip_archive = null


function pushFile(file) {
    var fileReader  = new FileReader();
    fileReader.onload  = function() {
        file_slot_file_name = file.name;
        file_slot_file = new Uint8Array(this.result);
        configure_file_dialog();
        //we have to null the file input otherwise when ejecting and inserting the same
        //file again it would not trigger the onchange/onload event 
        document.getElementById('filedialog').value=null;
    }
    fileReader.readAsArrayBuffer(file);
}

function configure_file_dialog(reset=false)
{
    reset_before_load=reset;

    try{
        if($("#modal_roms").is(":visible"))
        {
            var romtype = wasm_loadfile(file_slot_file_name+
                (file_slot_file_name.indexOf("ext")<0?".rom_file":".rom_ext_file")
                , file_slot_file);
            if(romtype != "")
            {
                setTimeout(async ()=>{
                    try{
                        local_storage_set(romtype, file_slot_file_name);
                        await save_rom(file_slot_file_name, romtype, file_slot_file);
                        await load_roms(/*false*/ true); // true to load reload an already selected extension, when the rom was just changed
                    }
                    catch(e){
                        console.error(e.message);
                        fill_rom_icons();
                        fill_ext_icons();
                    }
                });

            }
        }
        else
        {
            $("#file_slot_dialog_label").html(" "+file_slot_file_name);
            //configure file_slot  
            $("#button_insert_file").removeAttr("disabled");
            $("#div_zip_content").hide();
            $("#button_eject_zip").hide();
            $("#no_disk_rom_msg").hide();

            var return_icon=`&nbsp;<svg width="1em" height="1em" viewBox="0 0 16 16" class="bi bi-arrow-return-left" fill="currentColor" xmlns="http://www.w3.org/2000/svg"><path fill-rule="evenodd" d="M14.5 1.5a.5.5 0 0 1 .5.5v4.8a2.5 2.5 0 0 1-2.5 2.5H2.707l3.347 3.346a.5.5 0 0 1-.708.708l-4.2-4.2a.5.5 0 0 1 0-.708l4-4a.5.5 0 1 1 .708.708L2.707 8.3H12.5A1.5 1.5 0 0 0 14 6.8V2a.5.5 0 0 1 .5-.5z"/></svg>`;

            if(file_slot_file_name.match(/[.](zip)$/i)) 
            {
                $("#div_zip_content").show();

                $("#button_eject_zip").show();
                $("#button_eject_zip").click(function(){

                    $("#modal_file_slot").modal('hide');

                    last_zip_archive_name = null;
                    last_zip_archive = null;
                    
                    $("#drop_zone").html("file slot");
                    $("#drop_zone").css("border", "");

                });

                var zip = new JSZip();
                zip.loadAsync(file_slot_file).then(function (zip) {
                    var list='<ul id="ui_file_list" class="list-group">';
                    var mountable_count=0;
                    zip.forEach(function (relativePath, zipfile){
                        if(!relativePath.startsWith("__MACOSX") && !zipfile.dir)
                        {
                            let mountable = true; //relativePath.toLowerCase().match(/[.](zip|adf|hdf|dms|exe|vAmiga)$/i) || true;
                            list+='<li '+
                            (mountable ? 'id="li_fileselect'+mountable_count+'"':'')
                            +' class="list-group-item list-group-item-action'+ 
                                (mountable ? '':' disabled')+'">'+relativePath+'</li>';
                            if(mountable)
                            {
                                mountable_count++;
                            }
                        }
                    });
                    list += '</ul>';
                    $("#div_zip_content").html("select a file<br><br>"+ list);                    
                    $('#ui_file_list li').click( function (e) {
                        e.preventDefault();
                        if(typeof uncompress_progress !== 'undefined' && uncompress_progress!=null)
                        {
                            return;
                        }
                        $(this).parent().find('li').removeClass('active');
                        $(this).addClass('active');
                        $("#button_insert_file").attr("disabled", true);
                        let path = $(this).text();
                        let f=zip.file(path);
                        if(f!==null)
                        {
                            uncompress_progress='0';
                            f.async("uint8array", 
                            function updateCallback(metadata) {
                                //console.log("progression: " + metadata.percent.toFixed(2) + " %");
                                let current_progress=metadata.percent.toFixed(0);
                                if(uncompress_progress != current_progress)
                                {
                                    uncompress_progress = current_progress;
                                    $("#button_insert_file").html(`extract ${uncompress_progress}%`);
                                }
                            }).then(function (u8) {
                                file_slot_file_name=path;
                                if(!path.toLowerCase().match(/[.](zip|adf|hdf|dms|exe|vAmiga)$/i))
                                {
                                    file_slot_file_name+=".disk";
                                }
                                file_slot_file=u8;

                                if(mountable_count==1)
                                {//in case that there was only one mountable file in the zip, auto mount it
                                    configure_file_dialog(false);
                                }
                                else
                                {//file is ready to insert
                                    $("#button_insert_file").html("mount file"+return_icon);
                                    $("#button_insert_file").removeAttr("disabled");
                                }
                                uncompress_progress=null;
                            });
                        }
                    });
                    if(mountable_count>1)
                    {
                        last_zip_archive_name = file_slot_file_name;
                        last_zip_archive = file_slot_file;

                        $("#drop_zone").html('<svg width="1.3em" height="1.3em" viewBox="0 0 16 16" class="bi bi-archive-fill" fill="currentColor" xmlns="http://www.w3.org/2000/svg"><path fill-rule="evenodd" d="M12.643 15C13.979 15 15 13.845 15 12.5V5H1v7.5C1 13.845 2.021 15 3.357 15h9.286zM5.5 7a.5.5 0 0 0 0 1h5a.5.5 0 0 0 0-1h-5zM.8 1a.8.8 0 0 0-.8.8V3a.8.8 0 0 0 .8.8h14.4A.8.8 0 0 0 16 3V1.8a.8.8 0 0 0-.8-.8H.8z"/></svg> zip');
                        $("#drop_zone").css("border", "3px solid var(--green)");
                    }
                    else
                    {
                        $("#drop_zone").html("file slot");
                        $("#drop_zone").css("border", "");

                        last_zip_archive_name = null;
                        last_zip_archive = null; 
                    }

                    if(mountable_count>=1)
                    {
                        $("#li_fileselect0").click();
                    }
                    if(mountable_count>1)
                    {
                        (async ()=>{
                            for(var i=0; i<mountable_count;i++)
                            {
                                var path = $("#li_fileselect"+i).text();
                                if(df_mount_list.includes(path) || hd_mount_list.includes(path))
                                {
                                    let drive_number= df_mount_list.indexOf(path);
                                    if(drive_number<0)
                                        drive_number=hd_mount_list.indexOf(path);
                                    file_slot_file_name=path;
                                    file_slot_file = await zip.file(path).async("uint8array");
                                    insert_file(drive_number);
                                } 
                            }
                            if(df_mount_list.length == 0 && hd_mount_list.length==0)
                            {//when there is no auto mount list let the user decide
                                $("#modal_file_slot").modal();
                            }
                            df_mount_list=[];//only do automount once
                            hd_mount_list=[];
                        })();
                    }
                });

                $("#button_insert_file").html("mount file"+return_icon);
                $("#button_insert_file").attr("disabled", true);
            }
            else if(file_slot_file_name.match(/[.](adf|hdf|dms|exe|vAmiga|disk)$/i))
            {
                if(df_mount_list.includes(file_slot_file_name) || hd_mount_list.includes(file_slot_file_name))
                {
                    let drive_number= df_mount_list.indexOf(file_slot_file_name);
                    if(drive_number<0)
                        drive_number=hd_mount_list.indexOf(file_slot_file_name);
                    insert_file(drive_number);
                }
                else
                    prompt_for_drive();            
            }
            else
            {
                const header =new TextDecoder().decode(file_slot_file.subarray(0,8));
                if(header == "UAE-1ADF"||header == "UAE--ADF")
                {//extended adf
                    file_slot_file_name+=".adf";
                    prompt_for_drive();
                }
                else
                {
                    file_slot_file_name+=".disk";
                    prompt_for_drive();
                }
            }

        }    
    } catch(e) {
        console.log(e);
    }
}

function prompt_for_drive()
{
    let cancel=`<div id="prompt_drive_cancel" class="close" style="position:absolute;top:0.2em;right:0.4em;cursor:pointer" onclick="show_drive_select(false)">×</div>`;
    let_drive_select_stay_open=false;
    show_drive_select=(show)=>{
        if(let_drive_select_stay_open)
        {
            let_drive_select_stay_open=false;
            if(!show) return;
        }

        document.getElementById("div_drive_select").setAttribute('class', `slide-${show?"in":"out"}`);
        if(show)
        {
            $("#div_drive_select").show();
            add_pencil_support_to_childs(document.getElementById("drive_select_choice"));
            add_pencil_support(document.getElementById("prompt_drive_cancel"));
        }
        else
        {
            setTimeout(()=>$("#div_drive_select").hide(),1000); 
        }
    }

    if(file_slot_file_name.match(/[.](adf|dms|exe|disk)$/i))
    {
        let df_count=0;
        for(let i = 0; i<4;i++)
            df_count+=wasm_get_config_item("DRIVE_CONNECT",i);

        if(df_count==1 || parameter_link_mount_in_df0)
        {
            show_drive_select(false);
            insert_file(0);
        }
        else
        {
            let drv_html=
                file_slot_file_name.endsWith('.disk')?
                `${cancel}
                <div id="drive_select_file" class="gc_choice_text">import <span class="mx-2 px-2">${file_slot_file_name.replace(".disk","")}</span> as disk and insert into</div>`
                :
                `${cancel}
                <div id="drive_select_file" class="gc_choice_text">insert <span class="mx-2 px-2">${file_slot_file_name}</span> into</div>`;
                       
            drv_html+=`<div id="drive_select_choice">`;

            for(var dn=0;dn<df_count;dn++)
            {
                drv_html+=`<button type="button" class="btn btn-primary m-1 mb-2" style="width:20vw" onclick="insert_file(${dn});show_drive_select(false);">df${dn}:</button>`;
            }
            drv_html+=`</div>`;
            $("#div_drive_select").html(drv_html);
            show_drive_select(true);
        }
    }
    else if(file_slot_file_name.match(/[.]hdf$/i))
    {
        $("#div_drive_select").html(`${cancel}
        <div id="drive_select_file" class="gc_choice_text">reset amiga and mount <span class="mx-2 px-2">${file_slot_file_name}</span> into</div>
        <div id="drive_select_choice">
            <button type="button" class="btn btn-primary m-1 mb-2" style="width:20vw" onclick="insert_file(0);show_drive_select(false);">dh0:</button>
            <button type="button" class="btn btn-primary m-1 mb-2" style="width:20vw" onclick="insert_file(1);show_drive_select(false);">dh1:</button>
            <button type="button" class="btn btn-primary m-1 mb-2" style="width:20vw" onclick="insert_file(2);show_drive_select(false);">dh2:</button>
            <button type="button" class="btn btn-primary m-1 mb-2" style="width:20vw" onclick="insert_file(3);show_drive_select(false);">dh3:</button>
        </div>`);
        show_drive_select(true);
    }
    else
    {
        show_drive_select(false);
        insert_file(0);
    }

}

var port1 = 'none';
var port2 = 'none';
joystick_keydown_map=[];
joystick_keydown_map[1]={
    'ArrowUp':'PULL_UP',
    'ArrowDown':'PULL_DOWN',
    'ArrowLeft':'PULL_LEFT',
    'ArrowRight':'PULL_RIGHT',
    'Space':'PRESS_FIRE',
} 
joystick_keydown_map[2]=Object.assign({}, joystick_keydown_map[1]);
joystick_keydown_map[2].KeyB='PRESS_FIRE2';
joystick_keydown_map[3]=Object.assign({}, joystick_keydown_map[2]);
joystick_keydown_map[3].KeyN='PRESS_FIRE3';
joystick_keyup_map=[]
joystick_keyup_map[1] = {
    'ArrowUp':'RELEASE_Y',
    'ArrowDown':'RELEASE_Y',
    'ArrowLeft':'RELEASE_X',
    'ArrowRight':'RELEASE_X',
    'Space':'RELEASE_FIRE',
}
joystick_keyup_map[2]=Object.assign({}, joystick_keyup_map[1]);
joystick_keyup_map[2].KeyB='RELEASE_FIRE2';
joystick_keyup_map[3]=Object.assign({}, joystick_keyup_map[2]);
joystick_keyup_map[3].KeyN='RELEASE_FIRE3';




function is_any_text_input_active()
{
    if(typeof editor !== 'undefined' && editor.hasFocus())
        return true;

    var active = false;
    var element = document.activeElement;
    if(element != null)
    {                 
        if(element.tagName != null)
        {
            var type_name = element.tagName.toLowerCase();
            active = type_name == 'input' || type_name == 'textarea';
        }     
    }
    return active;
}

function serialize_key_code(e){
    let mods = ""; let code = e.code;
    if(e.altKey && !code.startsWith('Alt'))
        mods+="Alt+";
    if(e.metaKey && !code.startsWith('Meta'))
        mods+="Meta+";
    if(e.shiftKey && !code.startsWith('Shift'))
        mods+="Shift+";
    if(e.ctrlKey && !code.startsWith('Control'))
        mods+="Ctrl+";
    return mods+code;
}

function keydown(e) {
    if(is_any_text_input_active())
        return;

    e.preventDefault();

    if(port1=='keys'||port2=='keys')
    {
        var joystick_cmd = joystick_keydown_map[joystick_button_count][e.code];
        if(joystick_cmd !== undefined)
        {
            emit_joystick_cmd((port1=='keys'?'1':'2')+joystick_cmd);
            return;
        }
    }

    let serialized_code=serialize_key_code(e);
    for(action_button of custom_keys)
    {
        if(action_button.key == e.key /* e.key only for legacy custom keys*/   
           || action_button.key == serialized_code)
        {
            if(e.repeat)
            {
              //if a key is being pressed for a long enough time, it starts to auto-repeat: 
              //the keydown triggers again and again, and then when it’s released we finally get keyup
              //we just have to ignore the autorepeats here
              return;
            }
            let running_script=get_running_script(action_button.id);                    
            if(running_script.running == false)
            {
                running_script.action_button_released = false;
            }
            execute_script(action_button.id, action_button.lang, action_button.script);
            return;
        }
    }



    var key_code = translateKey2(e.code, e.key);
    if(key_code !== undefined && key_code.raw_key[0] != undefined)
    {
        if(key_code.modifier != null)
        {
            wasm_schedule_key(key_code.modifier[0], key_code.modifier[1], 1, 0);
        }
        wasm_schedule_key(key_code.raw_key[0], key_code.raw_key[1], 1, 0);
    }
}

function keyup(e) {
    if(is_any_text_input_active())
        return;

    e.preventDefault();

    if(port1=='keys'||port2=='keys')
    {
        var joystick_cmd = joystick_keyup_map[joystick_button_count][e.code];
        if(joystick_cmd !== undefined)
        {
            let port_id=port1=='keys'?'1':'2';
            if(joystick_cmd.startsWith('RELEASE_FIRE')
                ||
                //only release axis on key_up if the last key_down for that axis was the same direction
                //see issue #737
                port_state[port_id+'x'] == joystick_keydown_map[joystick_button_count][e.code]
                ||
                port_state[port_id+'y'] == joystick_keydown_map[joystick_button_count][e.code]
            )
            {
                emit_joystick_cmd(port_id+joystick_cmd);
            }
            return;
        }
    }

    let serialized_code=serialize_key_code(e);
    for(action_button of custom_keys)
    {
        if(action_button.key == e.key /* e.key only for legacy custom keys*/   
           || action_button.key == serialized_code)
        {
            get_running_script(action_button.id).action_button_released = true;
            return;
        }
    }

    var key_code = translateKey2(e.code, e.key);
    if(key_code !== undefined && key_code.raw_key[0] != undefined)
    {
        wasm_schedule_key(key_code.raw_key[0], key_code.raw_key[1], 0, 1);
        if(key_code.modifier != null )
        {
            wasm_schedule_key(key_code.modifier[0], key_code.modifier[1], 0, 1);
        }
    }
}

timestampjoy1 = null;
timestampjoy2 = null;
last_touch_cmd = null;
last_touch_fire= null;
/* callback for wasm mainsdl.cpp */
function draw_one_frame()
{
    let gamepads=null;

    if(port1 == 'none' || port1 =='keys' || port1.startsWith('mouse'))
    {
    }
    else if(port1 == 'touch')
    {
        handle_touch("1");
    }
    else
    {
        gamepads = navigator.getGamepads();        
        var joy1= gamepads[port1];
        
        if(joy1 != null && timestampjoy1 != joy1.timestamp)
        {
            timestampjoy1 = joy1.timestamp;
            handleGamePad('1', joy1);
        }
    }

    if(port2 == 'none' || port2 =='keys' || port2.startsWith('mouse'))
    {
    }
    else if(port2 == 'touch')
    {
        handle_touch("2");
    }	
    else
    {
        if(gamepads==null)
        {
            gamepads = navigator.getGamepads();        
        }
        var joy2= gamepads[port2];
        
        if(joy2 != null && timestampjoy2 != joy2.timestamp)
        {
            timestampjoy2 = joy2.timestamp;
            handleGamePad('2', joy2);
        }
    }
}


function handle_touch(portnr)
{    
    if(v_joystick == null || v_fire == null)
        return;
    try {
        var new_touch_cmd_x = "";
        if(v_joystick.right())
        {
            new_touch_cmd_x = "PULL_RIGHT";
        }
        else if(v_joystick.left())
        {
            new_touch_cmd_x = "PULL_LEFT";
        }
        else
        {
            new_touch_cmd_x = "RELEASE_X";
        }

        var new_touch_cmd_y = "";
        if(v_joystick.up())
        {
            new_touch_cmd_y = "PULL_UP";
        }
        else if(v_joystick.down())
        {
            new_touch_cmd_y = "PULL_DOWN";
        }
        else
        {
            new_touch_cmd_y ="RELEASE_Y";
        }

        let fire_button_number=Math.round(v_fire._stickY/(window.innerHeight/(joystick_button_count-1)))+1;
        var new_fire   = 1==fire_button_number&&v_fire._pressed?"PRESS_FIRE":"RELEASE_FIRE";
        var new_fire_2 = 2==fire_button_number&&v_fire._pressed?"PRESS_FIRE2":"RELEASE_FIRE2";
        var new_fire_3 = 3==fire_button_number&&v_fire._pressed?"PRESS_FIRE3":"RELEASE_FIRE3";
        
        var new_touch_cmd = portnr + new_touch_cmd_x + new_touch_cmd_y;

        if( last_touch_cmd != new_touch_cmd)
        {
            last_touch_cmd = new_touch_cmd;
            emit_joystick_cmd(portnr+new_touch_cmd_x);
            emit_joystick_cmd(portnr+new_touch_cmd_y);
             //play_sound(audio_df_step);
             v_joystick.redraw_base(new_touch_cmd);
        }
        var new_touch_fire = portnr + new_fire + new_fire_2 + new_fire_3;
        if( last_touch_fire != new_touch_fire)
        {
            last_touch_fire = new_touch_fire;
            emit_joystick_cmd(portnr+new_fire);
            emit_joystick_cmd(portnr+new_fire_2);
            emit_joystick_cmd(portnr+new_fire_3);
        }
    } catch (error) {
        console.error("error while handle_touch: "+ error);        
    }
}

function handleGamePad(portnr, gamepad)
{
    if(live_debug_output)
    {
        var axes_output="";
        for(var axe of gamepad.axes)
        {
            axes_output += axe.toFixed(2)+", ";
        }
        
        var btns_output = "";
        for(var btn of gamepad.buttons)
        {
            btns_output += (btn.pressed ? "1":"0")+ ", ";
        }

        Module.print(`controller ${gamepad.id}, mapping= ${gamepad.mapping}`);
        Module.print(`${gamepad.axes.length} axes= ${axes_output}`);
        Module.print(`${gamepad.buttons.length} btns= ${btns_output}`);
    }

    const horizontal_axis = 0;
    const vertical_axis = 1;
    let bReleaseX=true;
    let bReleaseY=true;

    for(let stick=0; stick<=gamepad.axes.length/2;stick+=2)
    {
        if(bReleaseX && 0.5<gamepad.axes[stick+horizontal_axis])
        {
            emit_joystick_cmd(portnr+"PULL_RIGHT");
            bReleaseX=false;
        }
        else if(bReleaseX && -0.5>gamepad.axes[stick+horizontal_axis])
        {
            emit_joystick_cmd(portnr+"PULL_LEFT");
            bReleaseX=false;
        }
 
        if(bReleaseY && 0.5<gamepad.axes[stick+vertical_axis])
        {
            emit_joystick_cmd(portnr+"PULL_DOWN");
            bReleaseY=false;
        }
        else if(bReleaseY && -0.5>gamepad.axes[stick+vertical_axis])
        {
            emit_joystick_cmd(portnr+"PULL_UP");
            bReleaseY=false;
        }
    }

    if(gamepad.buttons.length > 15 && bReleaseY && bReleaseX)
    {
        if(gamepad.buttons[12].pressed)
        {bReleaseY=false;
            emit_joystick_cmd(portnr+"PULL_UP");   
        }
        else if(gamepad.buttons[13].pressed)
        {bReleaseY=false;
            emit_joystick_cmd(portnr+"PULL_DOWN");   
        }
        else
        {
            bReleaseY=true;
        }
        if(gamepad.buttons[14].pressed)
        {bReleaseX=false;
            emit_joystick_cmd(portnr+"PULL_LEFT");   
        }
        else if(gamepad.buttons[15].pressed)
        {bReleaseX=false;
            emit_joystick_cmd(portnr+"PULL_RIGHT");   
        }
        else
        {
            bReleaseX=true;
        }
    }


    if(bReleaseX && bReleaseY)
    {
        emit_joystick_cmd(portnr+"RELEASE_XY");
    }
    else if(bReleaseX)
    {
        emit_joystick_cmd(portnr+"RELEASE_X");

    }
    else if(bReleaseY)
    {
        emit_joystick_cmd(portnr+"RELEASE_Y");
    }

    if(joystick_button_count==1)
    {
        var bFirePressed=false;
        for(var i=0; i<gamepad.buttons.length && i<12;i++)
        {
            if(gamepad.buttons[i].pressed)
            {
                bFirePressed=true;
            }
        }
        emit_joystick_cmd(portnr + (bFirePressed?"PRESS_FIRE":"RELEASE_FIRE"));
    }
    else if(joystick_button_count>1)
    {
        var bFirePressed=[false,false,false];

        for(var i=0; i<gamepad.buttons.length && i<12;i++)
        {
            if(gamepad.buttons[i].pressed)
            {
                bFirePressed[i%joystick_button_count]=true;
            }
        }
        emit_joystick_cmd(portnr + (bFirePressed[0]?"PRESS_FIRE":"RELEASE_FIRE"));
        emit_joystick_cmd(portnr + (bFirePressed[1]?"PRESS_FIRE2":"RELEASE_FIRE2"));
        emit_joystick_cmd(portnr + (bFirePressed[2]?"PRESS_FIRE3":"RELEASE_FIRE3"));
    }

}

var port_state={};
function emit_joystick_cmd(command)
{
    var port = command.substring(0,1);
    var cmd  = command.substring(1); 
  
    if(cmd == "PULL_RIGHT")
    {
        port_state[port+'x'] = cmd;
    }
    else if(cmd == "PULL_LEFT")
    {
        port_state[port+'x'] = cmd;
    }
    else if(cmd == "RELEASE_X")
    {
        port_state[port+'x'] = cmd;
    }
    else if(cmd == "PULL_UP")
    {
        port_state[port+'y'] = cmd;
    }
    else if(cmd == "PULL_DOWN")
    {
        port_state[port+'y'] = cmd;
    }
    else if(cmd == "RELEASE_Y")
    {
        port_state[port+'y'] = cmd;
    }
    else if(cmd == "RELEASE_XY")
    {
        port_state[port+'x'] = "RELEASE_X";
        port_state[port+'y'] = "RELEASE_Y";
    }
    else if(cmd=="PRESS_FIRE")
    {
        port_state[port+'fire']= cmd;
    }
    else if(cmd=="RELEASE_FIRE")
    {
        port_state[port+'fire']= cmd;
    }
    else if(cmd=="PRESS_FIRE2")
    {
        port_state[port+'fire2']= cmd;
    }
    else if(cmd=="RELEASE_FIRE2")
    {
        port_state[port+'fire2']= cmd;
    }
    else if(cmd=="PRESS_FIRE3")
    {
        port_state[port+'fire3']= cmd;
    }
    else if(cmd=="RELEASE_FIRE3")
    {
        port_state[port+'fire3']= cmd;
    }

    send_joystick(PORT_ACCESSOR.MANUAL, port, command);
/*
    console.log("portstate["+port+"x]="+port_state[port+'x']);
    console.log("portstate["+port+"y]="+port_state[port+'y']);
*/

}

const PORT_ACCESSOR = {
    MANUAL: 'MANUAL',
    BOT: 'BOT'
}

var current_port_owner = {
    1:PORT_ACCESSOR.MANUAL,
    2:PORT_ACCESSOR.MANUAL,
}; 

function set_port_owner(port, new_owner)
{
    var previous_owner=current_port_owner[port];
    current_port_owner[port]=new_owner;
    if(new_owner==PORT_ACCESSOR.MANUAL)
    {
       restore_manual_state(port);
    }
    return previous_owner;
}
function send_joystick( accessor, port, command )
{
    if(accessor == current_port_owner[port])
    {
        wasm_joystick(command);
    }
}

function restore_manual_state(port)
{
    if(port_state[port+'x'] !== undefined && !port_state[port+'x'].includes("RELEASE")) 
    {
        wasm_joystick( port + port_state[port+'x'] );
    }
    if(port_state[port+'y'] !== undefined && !port_state[port+'y'].includes("RELEASE")) 
    {
        wasm_joystick( port + port_state[port+'y'] );
    }
    if(port_state[port+'fire'] !== undefined && !port_state[port+'fire'].includes("RELEASE")) 
    {
        wasm_joystick( port + port_state[port+'fire'] );
    }
}



function InitWrappers() {
    try{add_pencil_support_for_elements_which_need_it();} catch(e) {console.error(e)}
    wasm_loadfile = function (file_name, file_buffer, drv_number=0) {
        var file_slot_wasmbuf = Module._malloc(file_buffer.byteLength);
        Module.HEAPU8.set(file_buffer, file_slot_wasmbuf);
        var retVal=Module.ccall('wasm_loadFile', 'string', ['string','number','number', 'number'], [file_name,file_slot_wasmbuf,file_buffer.byteLength, drv_number]);
        Module._free(file_slot_wasmbuf);
        return retVal;                    
    }
    wasm_write_bytes_to_ser = function (bytes_to_send) {
        for(let b of bytes_to_send)
        {
            Module._wasm_write_byte_to_ser(b);
        }
    }
    wasm_write_byte_to_ser = function (byte_to_send) {
            Module._wasm_write_byte_to_ser(byte_to_send);
    }
    wasm_key = Module.cwrap('wasm_key', 'undefined', ['number', 'number']);
    wasm_toggleFullscreen = Module.cwrap('wasm_toggleFullscreen', 'undefined');
    wasm_joystick = Module.cwrap('wasm_joystick', 'undefined', ['string']);
    wasm_reset = Module.cwrap('wasm_reset', 'undefined');

    stop_request_animation_frame=true;
    wasm_halt=function () {
        Module._wasm_halt();
        stop_request_animation_frame=true;
    }
    wasm_draw_one_frame= Module.cwrap('wasm_draw_one_frame', 'undefined');

    do_animation_frame=null;
    queued_executes=0;
    
    wasm_run = function () {
        Module._wasm_run();       
        if(do_animation_frame == null)
        {
            execute_amiga_frame=()=>{
                Module._wasm_execute(); 
                queued_executes--;
            };

            render_frame= (now)=>{
                if(current_renderer=="gpu shader")
                    render_canvas_gl(now);
                else
                    render_canvas(now);
            }
            if(Module._wasm_is_worker_built()){
                rendered_frame_id=0;
                calculate_and_render=(now)=>
                {
                    draw_one_frame(); // to gather joystick information 
                    Module._wasm_worker_run();                    
                    let current_rendered_frame_id=Module._wasm_frame_info();
                    if(rendered_frame_id !== current_rendered_frame_id)
                    {
                        render_frame(now);
                        rendered_frame_id = current_rendered_frame_id;
                    }
                }
            }
            else
            {
                calculate_and_render=(now)=>
                {
                    draw_one_frame(); // to gather joystick information 
                    let behind = Module._wasm_draw_one_frame(now);
                    if(behind<0)
                        return;
                    render_frame(now);
                    while(behind>queued_executes)
                    {
                        queued_executes++;
                        setTimeout(execute_amiga_frame);
                    }
                }
            }

            do_animation_frame = function(now) {
                calculate_and_render(now);

                // request another animation frame
                if(!stop_request_animation_frame)
                {
                    requestAnimationFrame(do_animation_frame);   
                }
            }
        }  
        if(stop_request_animation_frame)
        {
            stop_request_animation_frame=false;
            requestAnimationFrame(do_animation_frame);   
        }
    }

    wasm_take_user_snapshot = Module.cwrap('wasm_take_user_snapshot', 'undefined');
    wasm_pull_user_snapshot_file = Module.cwrap('wasm_pull_user_snapshot_file', 'string');
    wasm_delete_user_snapshot = Module.cwrap('wasm_delete_user_snapshot', 'undefined');

    wasm_create_renderer = Module.cwrap('wasm_create_renderer', 'number', ['string']);
    wasm_set_warp = Module.cwrap('wasm_set_warp', 'undefined', ['number']);
    wasm_sprite_info = Module.cwrap('wasm_sprite_info', 'string');

    wasm_cut_layers = Module.cwrap('wasm_cut_layers', 'undefined', ['number']);
    wasm_rom_info = Module.cwrap('wasm_rom_info', 'string');

    wasm_get_cpu_cycles = Module.cwrap('wasm_get_cpu_cycles', 'number');
    wasm_set_color_palette = Module.cwrap('wasm_set_color_palette', 'undefined', ['string']);

    wasm_schedule_key = Module.cwrap('wasm_schedule_key', 'undefined', ['number', 'number', 'number', 'number']);

    wasm_peek = Module.cwrap('wasm_peek', 'number', ['number']);
    wasm_poke = Module.cwrap('wasm_poke', 'undefined', ['number', 'number']);
    wasm_has_disk = Module.cwrap('wasm_has_disk', 'number', ['string']);
    wasm_eject_disk = Module.cwrap('wasm_eject_disk', 'undefined', ['string']);
    wasm_export_disk = Module.cwrap('wasm_export_disk', 'string', ['string']);
    wasm_configure = Module.cwrap('wasm_configure', 'string', ['string', 'string']);
    wasm_configure_key = Module.cwrap('wasm_configure_key', 'string', ['string', 'string', 'string']);
    wasm_write_string_to_ser = Module.cwrap('wasm_write_string_to_ser', 'undefined', ['string']);
    wasm_print_error = Module.cwrap('wasm_print_error', 'undefined', ['number']);
    wasm_power_on = Module.cwrap('wasm_power_on', 'string', ['number']);
    wasm_get_sound_buffer_address = Module.cwrap('wasm_get_sound_buffer_address', 'number');
    wasm_copy_into_sound_buffer = Module.cwrap('wasm_copy_into_sound_buffer', 'number');
    wasm_set_sample_rate = Module.cwrap('wasm_set_sample_rate', 'undefined', ['number']);
    wasm_mouse = Module.cwrap('wasm_mouse', 'undefined', ['number','number','number']);
    wasm_mouse_button = Module.cwrap('wasm_mouse_button', 'undefined', ['number','number','number']);
    wasm_set_display = Module.cwrap('wasm_set_display', 'undefined',['string']);
    wasm_auto_type = Module.cwrap('wasm_auto_type', 'undefined', ['number', 'number', 'number']);

    wasm_set_target_fps = Module.cwrap('wasm_set_target_fps', 'undefined', ['number']);
    wasm_get_renderer = Module.cwrap('wasm_get_renderer', 'number');
    wasm_get_config_item = Module.cwrap('wasm_get_config_item', 'number', ['string']);
    wasm_get_core_version = Module.cwrap('wasm_get_core_version', 'string');

    resume_audio=async ()=>{
        try {
            await audioContext.resume();  
        }
        catch(e) {
            console.error(e); console.error("try to setup audio from scratch...");
            try {
                await audioContext.close();
            }
            finally
            {
                audio_connected=false; 
                audioContext=new AudioContext();
            }
        }
    }
    
    connect_audio_processor_standard = async () => {
        if(audioContext.state !== 'running') {
            await resume_audio();
        }
        if(audio_connected==true)
            return; 
        if(audioContext.state === 'suspended') {
            return;  
        }
        audio_connected=true;
        wasm_set_sample_rate(audioContext.sampleRate);
        console.log("try connecting audioprocessor...");
        if(audioContext.audioWorklet==undefined)
        {
            console.error("audioContext.audioWorklet == undefined");
            return;
        }
        await audioContext.audioWorklet.addModule('js/vAmiga_audioprocessor.js');
        worklet_node = new AudioWorkletNode(audioContext, 'vAmiga_audioprocessor', {
            outputChannelCount: [2],
            numberOfInputs: 0,
            numberOfOutputs: 1
        });

        init_sound_buffer=function(){
            console.log("get wasm sound buffer adresses");
            let sound_buffer_address = wasm_get_sound_buffer_address();
            soundbuffer_slots=[];
            for(slot=0;slot<16;slot++)
            {
                soundbuffer_slots.push(
                    new Float32Array(Module.HEAPF32.buffer, sound_buffer_address+(slot*2048)*4, 2048));
            }
        }
        init_sound_buffer();

        empty_shuttles=new RingBuffer(16);
        worklet_node.port.onmessage = (msg) => {
            //direct c function calls with preceeding Module._ are faster than cwrap
            let samples=Module._wasm_copy_into_sound_buffer();
            let shuttle = msg.data;
            if(samples<1024)
            {
                if(shuttle!="empty")
                {
                    empty_shuttles.write(shuttle);
                }
                return;
            }
            let slot=0;
            while(samples>=1024)
            {
                if(shuttle == null || shuttle=="empty")
                {
                    if(!empty_shuttles.isEmpty())
                    {
                        shuttle = empty_shuttles.read();
                    }
                    else
                    {
                      return;
                    }
                }
                let wasm_buffer_slot = soundbuffer_slots[slot++];
                if(wasm_buffer_slot.byteLength==0)
                {//slot can be detached when wasm had grown memory, adresses are wrong then so lets reinit
                    init_sound_buffer();
                    wasm_buffer_slot = soundbuffer_slots[slot-1];
                }
                shuttle.set(wasm_buffer_slot);
                worklet_node.port.postMessage(shuttle, [shuttle.buffer]);
                shuttle=null;
                samples-=1024;
            }            
        };
        worklet_node.port.onmessageerror = (msg) => {
            console.log("audio processor error:"+msg);
        };
        worklet_node.connect(audioContext.destination);        
    }

    connect_audio_processor_shared_memory= async ()=>{
        if(audioContext.state !== 'running') {
            await resume_audio();
        }
        if(audio_connected==true)
            return; 
        if(audioContext.state === 'suspended') {
            return;  
        }
        audio_connected=true;

        audioContext.onstatechange = () => console.log('Audio Context: state = ' + audioContext.state);
        let gainNode = audioContext.createGain();
        gainNode.gain.value = 0.15;
        gainNode.connect(audioContext.destination);
        wasm_set_sample_rate(audioContext.sampleRate);
        await audioContext.audioWorklet.addModule('js/vAmiga_audioprocessor_sharedarraybuffer.js');
        const audioNode = new AudioWorkletNode(audioContext, 'vAmiga_audioprocessor_sharedarraybuffer', {
            outputChannelCount: [2],
            processorOptions: {
                pointers: [Module._wasm_leftChannelBuffer(), Module._wasm_rightChannelBuffer()],
                buffer: Module.HEAPF32.buffer,
                length: 2048
            }
        });
        audioNode.port.onmessage = (e) => {
            Module._wasm_update_audio(e.data);
        };
        audioNode.connect(audioContext.destination);
        if (audioContext.state === 'suspended') {
            await audioContext.resume();
        }
    }

    if(Module._wasm_is_worker_built())
    {
        connect_audio_processor=connect_audio_processor_shared_memory;
    }
    else
    {
        connect_audio_processor=connect_audio_processor_standard;
    }

    //when app becomes hidden/visible
    window.addEventListener("visibilitychange", async () => {
        if(document.visibilityState == "hidden") {
           console.log("visibility=hidden");
           let is_full_screen=document.fullscreenElement!=null;
           console.log("fullscreen="+is_full_screen);
           if(!is_full_screen)
           {//safari bug: goes visible=hidden when entering fullscreen
            //in that case don't disable the audio 
               try { audioContext.suspend(); } catch(e){ console.error(e);}
           }
        }
        else if(emulator_currently_runs)
        {
            try { await connect_audio_processor(); } catch(e){ console.error(e);}
            add_unlock_user_action();
        }
        if(document.visibilityState === "visible" && wakeLock !== null)
        {
            if(is_running())
            {
//                alert("req wakelock again "+document.visibilityState);
                set_wake_lock(true);
            }
        }
    });

    //when app is going to background either visible or hidden
//    window.addEventListener('blur', ()=>{});

    //when app is coming to foreground again
    window.addEventListener('focus', async ()=>{
        if(emulator_currently_runs)
        {
            try { await connect_audio_processor(); } catch(e){ console.error(e);}
            add_unlock_user_action();
        }
    });

    add_unlock_user_action = function(){
        //in case we did go suspended reinstall the unlock events
        document.removeEventListener('click',click_unlock_WebAudio);
        document.addEventListener('click',click_unlock_WebAudio, false);

        //iOS safari does not bubble click events on canvas so we add this extra event handler here
        let canvas=document.getElementById('canvas');
        canvas.removeEventListener('touchstart',touch_unlock_WebAudio);
        canvas.addEventListener('touchstart',touch_unlock_WebAudio,false);        
    }
    remove_unlock_user_action = function(){
        //if it runs we dont need the unlock handlers, has no effect when handler already removed 
        document.removeEventListener('click',click_unlock_WebAudio);
        document.getElementById('canvas').removeEventListener('touchstart',touch_unlock_WebAudio);
    }

    audioContext.onstatechange = () => {
        let state = audioContext.state;
        console.error(`audioContext.state=${state}`);
        if(state!=='running'){
            add_unlock_user_action();
        }
        else {
            remove_unlock_user_action();
        }
    }

    click_unlock_WebAudio=async function() {
        try { 
            await connect_audio_processor(); 
            if(audioContext.state=="running")
                remove_unlock_user_action();
        } catch(e){ console.error(e);}
    }
    touch_unlock_WebAudio=async function() {
        try { 
            await connect_audio_processor(); 
            if(audioContext.state=="running")
                remove_unlock_user_action();
        } catch(e){ console.error(e);}
    }    

    add_unlock_user_action();
    
    window.addEventListener('message', event => {
        if(event.data == "poll_state")
        {
            window.parent.postMessage({ msg: 'render_run_state', value: is_running()},"*");
            window.parent.postMessage({ msg: 'render_current_audio_state', 
                value: audioContext == null ? 'suspended' : audioContext.state},"*"); 
        }
        else if(event.data == "button_run()")
        {
            if(required_roms_loaded)
            {
                $('#button_run').click();
                window.parent.postMessage({ msg: 'render_run_state', value: is_running()},"*");
            }
        }
        else if(event.data == "toggle_audio()")
        {
            if (audioContext !=null)
            {
                if(audioContext.state == 'suspended') {
                    audioContext.resume();
                }
                else if (audioContext.state == 'running')
                {
                    audioContext.suspend();
                }
            }
            window.parent.postMessage({ msg: 'render_current_audio_state', 
                value: audioContext == null ? 'suspended' : audioContext.state },"*");
        }
        else if(event.data == "open_zip()")
        {
            var modal = $('#modal_file_slot'); 
            if(modal.is(':visible'))
            {
                modal.modal('hide');
            }
            else
            {
                if(required_roms_loaded && last_zip_archive_name != null)
                {
                    file_slot_file_name = last_zip_archive_name;
                    file_slot_file = last_zip_archive;
                    configure_file_dialog();
                }
            }
        }
        else if(event.data.cmd == "script")
        {
            let AsyncFunction = Object.getPrototypeOf(async function(){}).constructor
            let js_script_function=new AsyncFunction(event.data.script);
            js_script_function();
        }
        else if(event.data.cmd == "load")
        {
            async function copy_to_local_storage(romtype, byteArray)
            {
                if(romtype != "")
                {
                    try{
                        local_storage_set(romtype+".bin", ToBase64(byteArray));
                        await save_rom(romtype+".bin", romtype,  byteArray);                    
                        await load_roms(false);
                    }
                    catch(e){
                        console.error(e.message)
                        fill_rom_icons();
                        fill_ext_icons();
                    }
                }
            }

            let with_reset=false;
            //put whdload kickemu into disk drive
            if(event.data.mount_kickstart_in_dfn &&
                event.data.mount_kickstart_in_dfn >=0 )
            {
                wasm_loadfile("kick-rom.disk", event.data.kickemu_rom, event.data.mount_kickstart_in_dfn);
            }
            //check if any roms should be preloaded first... 
            if(event.data.kickstart_rom !== undefined)
            {
                wasm_loadfile("kick.rom_file", event.data.kickstart_rom);
                //copy_to_local_storage(rom_type, byteArray);
                if(event.data.kickstart_ext !== undefined)
                {
                    wasm_loadfile("kick.rom_ext_file", event.data.kickstart_ext);
                }
                with_reset=true;
            }
            if(with_reset){
                wasm_reset();
            }
            if(event.data.file_name !== undefined && event.data.file !== undefined)
            {
                file_slot_file_name = event.data.file_name;
                file_slot_file = event.data.file;
                //if there is still a zip file in the fileslot, eject it now
                $("#button_eject_zip").click();
                configure_file_dialog(reset=false);
            }
        }
        else if(event.data.cmd == "ser:")
        {
            if(event.data.text !== undefined)
            {
                wasm_write_string_to_ser(event.data.text);
            }
            else if (event.data.byte !== undefined )
            {
                Module._wasm_write_byte_to_ser(event.data.byte);
            }
            else if (event.data.bytes !== undefined )
            {
                wasm_write_bytes_to_ser(event.data.bytes);
            }
        }
    });
    
    dark_switch = document.getElementById('dark_switch');


    $('#modal_roms').on('hidden.bs.modal', async function () {
        //check again if required roms are there when user decides to exit rom-dialog 
        if(required_roms_loaded == false)
        {//if they are still missing ... we make the decision for the user and 
         //just load the open roms for him instead ...
            await fetchOpenROMS();
        }
        load_parameter_link();
    });

    loadTheme();
    dark_switch.addEventListener('change', () => {
        setTheme();
    });
    
    //--- mouse pointer lock
    canvas = document.querySelector('canvas');
    canvas.requestPointerLock = canvas.requestPointerLock ||
                                canvas.mozRequestPointerLock;

    document.exitPointerLock = document.exitPointerLock ||
                            document.mozExitPointerLock;

    has_pointer_lock=false;
    try_to_lock_pointer=0;
    has_pointer_lock_fallback=false;
    window.last_mouse_x=0;
    window.last_mouse_y=0;

    request_pointerlock = async function() {
        if(canvas.requestPointerLock === undefined)
        {
            if(!has_pointer_lock_fallback)
            {
                add_pointer_lock_fallback();      
            }
            return;
        }
        if(!has_pointer_lock && try_to_lock_pointer <20)
        {
            try_to_lock_pointer++;
            try {
                if(has_pointer_lock_fallback) {remove_pointer_lock_fallback();}
                await canvas.requestPointerLock();
                try_to_lock_pointer=0;
            } catch (error) {
                await sleep(100);
                await request_pointerlock();                
            }
        }
    };
    
    window.add_pointer_lock_fallback=()=>{
        document.addEventListener("mousemove", updatePosition_fallback, false); 
        document.addEventListener("mousedown", mouseDown, false);
        document.addEventListener("mouseup", mouseUp, false);
        has_pointer_lock_fallback=true;
    };
    window.remove_pointer_lock_fallback=()=>{
        document.removeEventListener("mousemove", updatePosition_fallback, false); 
        document.removeEventListener("mousedown", mouseDown, false);
        document.removeEventListener("mouseup", mouseUp, false);
        has_pointer_lock_fallback=false;
    };
    document.addEventListener('pointerlockerror', add_pointer_lock_fallback, false);

    // Hook pointer lock state change events for different browsers
    document.addEventListener('pointerlockchange', lockChangeAlert, false);
    document.addEventListener('mozpointerlockchange', lockChangeAlert, false);

    function lockChangeAlert() {
        if (document.pointerLockElement === canvas ||
            document.mozPointerLockElement === canvas) {
            has_pointer_lock=true;
//            console.log('The pointer lock status is now locked');
            document.addEventListener("mousemove", updatePosition, false);
            document.addEventListener("mousedown", mouseDown, false);
            document.addEventListener("mouseup", mouseUp, false);

        } else {
//            console.log('The pointer lock status is now unlocked');  
            document.removeEventListener("mousemove", updatePosition, false);
            document.removeEventListener("mousedown", mouseDown, false);
            document.removeEventListener("mouseup", mouseUp, false);

            has_pointer_lock=false;
        }
    }
    var mouse_port=1;
    function updatePosition(e) {
        Module._wasm_mouse(mouse_port,e.movementX,e.movementY);
    }
    function updatePosition_fallback(e) {
        let movementX=e.screenX-window.last_mouse_x;
        let movementY=e.screenY-window.last_mouse_y;
        window.last_mouse_x=e.screenX;
        window.last_mouse_y=e.screenY;
        let border_speed=4;
        let border_pixel=2;
    
        if(e.screenX<=border_pixel)
          movementX=-border_speed;
        if(e.screenX>=window.innerWidth-border_pixel)
          movementX=border_speed;
        if(e.screenY<=border_pixel)
          movementY=-border_speed;
        if(e.screenY>=window.innerHeight-border_pixel)
          movementY=border_speed;        
        Module._wasm_mouse(mouse_port,movementX,movementY);  
    }
    function mouseDown(e) {
        Module._wasm_mouse_button(mouse_port,e.which, 1/* down */);
    }
    function mouseUp(e) {
        Module._wasm_mouse_button(mouse_port,e.which, 0/* up */);
    }

    //--
    mouse_touchpad_port=1;
    mouse_touchpad_move_touch=null;
    mouse_touchpad_left_button_touch=null;
    mouse_touchpad_right_button_touch=null;

    function emulate_mouse_touchpad_start(e)
    {
        for (var i=0; i < e.changedTouches.length; i++) {
            let touch = e.changedTouches[i];
        
            if(mouse_touchpad_pattern=='mouse touchpad')
            {
                let mouse_touchpad_move_area= touch.clientX > window.innerWidth/10 &&
                touch.clientX < window.innerWidth-window.innerWidth/10;
                let mouse_touchpad_button_area=!mouse_touchpad_move_area;

                if(mouse_touchpad_button_area)
                {
                    let left_button = touch.clientX < window.innerWidth/10;
                    if(left_button)
                    {
                        mouse_touchpad_left_button_touch=touch; 
                        Module._wasm_mouse_button(mouse_touchpad_port,1, 1/* down */);                
                    }
                    else
                    {
                        mouse_touchpad_right_button_touch=touch; 
                        Module._wasm_mouse_button(mouse_touchpad_port,3, 1/* down */);                
                    }
                }
                else
                {
                    mouse_touchpad_move_touch=touch;
                }
            }
            else if(mouse_touchpad_pattern=='mouse touchpad2')
            {
                let mouse_touchpad_move_area= touch.clientX < window.innerWidth/2;
                let mouse_touchpad_button_area=!mouse_touchpad_move_area;

                if(mouse_touchpad_button_area)
                {
                    let left_button = touch.clientY >= window.innerHeight/2;
                    if(left_button)
                    {
                        mouse_touchpad_left_button_touch=touch; 
                        Module._wasm_mouse_button(mouse_touchpad_port,1, 1/* down */);                
                    }
                    else
                    {
                        mouse_touchpad_right_button_touch=touch; 
                        Module._wasm_mouse_button(mouse_touchpad_port,3, 1/* down */);                
                    }
                }
                else
                {
                    mouse_touchpad_move_touch=touch;
                }
            }

        }
    }
    function emulate_mouse_touchpad_move(e)
    {
        for (var i=0; i < e.changedTouches.length; i++) {
            let touch = e.changedTouches[i];
            if(mouse_touchpad_move_touch!=null && 
                mouse_touchpad_move_touch.identifier== touch.identifier)
            {
                Module._wasm_mouse(mouse_touchpad_port,touch.clientX-mouse_touchpad_move_touch.clientX,
                    touch.clientY-mouse_touchpad_move_touch.clientY);
                mouse_touchpad_move_touch=touch;
            }
        }
    }
    function emulate_mouse_touchpad_end(e)
    {
        for (var i=0; i < e.changedTouches.length; i++) {
            let touch = e.changedTouches[i];
            if(mouse_touchpad_move_touch!=null && 
                mouse_touchpad_move_touch.identifier== touch.identifier)
            {
                mouse_touchpad_move_touch=null;
            }
            else if(mouse_touchpad_left_button_touch != null && 
                mouse_touchpad_left_button_touch.identifier == touch.identifier)
            {
                Module._wasm_mouse_button(mouse_touchpad_port,1, 0/* down */);
                mouse_touchpad_left_button_touch=null;
            }
            else if(mouse_touchpad_right_button_touch != null && 
                mouse_touchpad_right_button_touch.identifier == touch.identifier)
            {
                Module._wasm_mouse_button(mouse_touchpad_port,3, 0/* down */);
                mouse_touchpad_right_button_touch=null;
            }
        }
    }
    //check if call_param_mouse is set
    if(call_param_mouse)
    {
        if(call_param_touch==true)
        {
            port1="mouse touchpad2";
            $('#port1').val(port1);
            mouse_touchpad_pattern=port1;
            mouse_touchpad_port=1;
            document.addEventListener('touchstart',emulate_mouse_touchpad_start, false);
            document.addEventListener('touchmove',emulate_mouse_touchpad_move, false);
            document.addEventListener('touchend',emulate_mouse_touchpad_end, false);
            if(port2=="touch")
            {
                port2="none";
                $('#port2').val(port2);
            }    
        }
        else
        { 
            port1="mouse";
            $('#port1').val(port1);
            canvas.addEventListener('click', request_pointerlock);
        }
    }
    //--

    installKeyboard();
    $("#button_keyboard").click(function(){
        if(virtual_keyboard_clipping==false)
        {
            let body_width =$("body").innerWidth();
            let vk_abs_width=750+25; //+25 border
            let vk=$("#virtual_keyboard");

            //calculate scaled width
            let scaled= vk_abs_width/body_width;
            if(scaled < 1)
            {
                scaled = 1;
            }
            vk.css("width", `${scaled*100}vw`);
            vk.css("transform", `scale(${1/scaled})`);
            vk.css("transform-origin", `left bottom`);    
        }
        setTimeout( scaleVMCanvas, 500);
    });


    window.addEventListener("orientationchange", function() {
        setTimeout(()=>wasm_set_display(""), 500);
    });

    window.addEventListener("resize", function() {
        setTimeout(()=>wasm_set_display(""), 0);
    });
    
    $('#navbar').on('hide.bs.collapse', function () {
        //close all open tooltips on hiding navbar
        hide_all_tooltips();
    });

    $('#navbar').on('shown.bs.collapse', function () { 
    });

    burger_time_out_handle=null
    burger_button=null;
    menu_button_fade_in = function () {
        if(burger_button == null)
        {
            burger_button = $("#button_show_menu");
        }
        
        burger_button.fadeTo( "slow", 1.0 );
        
        if(burger_time_out_handle != null)
        {
            clearTimeout(burger_time_out_handle);
        }
        burger_time_out_handle = setTimeout(function() {
            if($("#navbar").is(":hidden"))
            {
                burger_button.fadeTo( "slow", 0.0 );
            }
        },5000);    
    };

    //make the menubutton not visible until a click or a touch
    menu_button_fade_in();
    burger_button.hover(function(){ menu_button_fade_in();});

    window.addEventListener("click", function() {
        menu_button_fade_in();
    });
    $("#canvas").on({ 'touchstart' : function() {
        menu_button_fade_in();
    }});

//----
    auto_selecting_app_title_switch = $('#auto_selecting_app_title_switch');
    auto_selecting_app_title=load_setting('auto_selecting_app_title', true);
    auto_selecting_app_title_switch.prop('checked', auto_selecting_app_title);
    auto_selecting_help = ()=>{
        if(auto_selecting_app_title)
            {
                $("#auto_select_on_help").show();
                $("#auto_select_off_help").hide();
            }
            else
            {
                $("#auto_select_on_help").hide();
                $("#auto_select_off_help").show();
            }    
    }
    auto_selecting_help();
    
    
    auto_selecting_app_title_switch.change( function() {
        auto_selecting_app_title=this.checked;
        save_setting('auto_selecting_app_title', auto_selecting_app_title);
        auto_selecting_help();
    });
    //----
    movable_action_buttons_in_settings_switch = $('#movable_action_buttons_in_settings_switch');
    movable_action_buttons=load_setting('movable_action_buttons', true);
 
    movable_action_buttons_in_settings_switch.prop('checked', movable_action_buttons);
    movable_action_buttons_in_settings_switch.change( function() {
        movable_action_buttons=this.checked;
        install_custom_keys();
        save_setting('movable_action_buttons', movable_action_buttons);
        $('#move_action_buttons_switch').prop('checked',movable_action_buttons);
        set_move_action_buttons_label();
    });

    $('#move_action_buttons_switch').prop('checked',movable_action_buttons);

    let set_move_action_buttons_label=()=>{
        $('#move_action_buttons_label').html(
            movable_action_buttons ? 
            `Once created, you can <span>move any 'action button' by dragging</span>… A <span>long press will enter 'edit mode'</span>… While 'moveable action buttons' is switched on, <span>scripts can not detect release</span> state (to allow this, you must disable the long press gesture by turning 'moveable action buttons' off)`
            :
            `All <span>'action button' positions are now locked</span>… <span>scripts are able to trigger actions when the button is released</span>… <span>long press edit mode is disabled</span> (instead use the <span>+</span> from the top menu bar and choose any buttons from the list to edit)`
        );
        $('#move_action_buttons_label_settings').html(
            movable_action_buttons ? 
            `long press to enter edit mode. movable by dragging. action scripts are unable to detect buttons release state.`
            :
            `action button positions locked. action scripts can detect release state. Long press edit gesture disabled, use <span>+</span> from the top menu bar and choose any buttons from list to edit`
        );
    }
    set_move_action_buttons_label();
    $('#move_action_buttons_switch').change( 
        ()=>{
                movable_action_buttons=!movable_action_buttons;
                set_move_action_buttons_label();
                install_custom_keys();
                movable_action_buttons_in_settings_switch.prop('checked', movable_action_buttons);
                save_setting('movable_action_buttons', movable_action_buttons);
            }
    ); 

    //----
let set_game_controller_buttons_choice = function (choice) {
    $(`#button_game_controller_type_choice`).text('button count='+choice);
    joystick_button_count=choice;
    save_setting("game_controller_buttons",choice);   

    for(el of document.querySelectorAll(".gc_choice_text"))
    {
        el.style.display="none";
    }
    document.getElementById('gc_buttons_'+choice).style.display="inherit";
}
joystick_button_count=load_setting("game_controller_buttons", 1);
set_game_controller_buttons_choice(joystick_button_count);

$(`#choose_game_controller_type a`).click(function () 
{
    let choice=$(this).text();
    set_game_controller_buttons_choice(choice);
    $("#modal_settings").focus();
});

//----
    let set_vbk_choice = function (choice) {
        $(`#button_vbk_touch`).text('keycap touch behaviour='+choice);
        current_vbk_touch=choice;
        save_setting("vbk_touch",choice);   

        for(el of document.querySelectorAll(".vbk_choice_text"))
        {
            el.style.display="none";
        }
        document.getElementById(choice.replace(" ","_").replace(" ","_")+"_text").style.display="inherit";
    }
    current_vbk_touch=load_setting("vbk_touch", "mix of both");
    set_vbk_choice(current_vbk_touch);

    $(`#choose_vbk_touch a`).click(function () 
    {
        let choice=$(this).text();
        set_vbk_choice(choice);
        $("#modal_settings").focus();
    });

//---
let set_vjoy_choice = function (choice) {
    $(`#button_vjoy_touch`).text('positioning='+choice);
    current_vjoy_touch=choice;

    let need_re_register=false;
    if(v_joystick != null)
    {
        need_re_register=true;
        unregister_v_joystick()
    }

    if(v_joystick != null)
    {
        v_joystick._stationaryBase=false;    
    }
    if(choice == "base moves")
    {
        stationaryBase = false;
        fixed_touch_joystick_base=false;
    }
    else if(choice == "base fixed on first touch")
    {
        stationaryBase = false;
        fixed_touch_joystick_base=true;
    }
    else if(choice.startsWith("stationary"))
    {
        stationaryBase = true;
        fixed_touch_joystick_base=true;
    }
    if(need_re_register)
    {
        register_v_joystick()
    }

    save_setting("vjoy_touch",choice);   

    for(el of document.querySelectorAll(".vjoy_choice_text"))
    {
        el.style.display="none";
    }
    let text_label=document.getElementById(choice.replaceAll(" ","_")+"_text");
    if(text_label !== null)
    {
        text_label.style.display="inherit";
    }
}
current_vjoy_touch=load_setting("vjoy_touch", "base moves");
set_vjoy_choice(current_vjoy_touch);

$(`#choose_vjoy_touch a`).click(function () 
{
    let choice=$(this).text();
    set_vjoy_choice(choice);
    $("#modal_settings").focus();
});
//---
set_vjoy_dead_zone(load_setting('vjoy_dead_zone', 14));
function set_vjoy_dead_zone(vjoy_dead_zone) {
    rest_zone=vjoy_dead_zone;
    $("#button_vjoy_dead_zone").text(`virtual joysticks dead zone=${vjoy_dead_zone}`);
}
$('#choose_vjoy_dead_zone a').click(function () 
{
    var vjoy_dead_zone=$(this).text();
    set_vjoy_dead_zone(vjoy_dead_zone);
    save_setting('vjoy_dead_zone',vjoy_dead_zone);
    $("#modal_settings").focus();
});

//--
set_keycap_size(load_setting('keycap_size', '1.00'));
function set_keycap_size(keycap_size) {
    document.querySelector(':root').style.setProperty('--keycap_zoom', keycap_size);
    $("#button_keycap_size").text(`keycap size=${keycap_size}`);
}
$('#choose_keycap_size a').click(function () 
{
    var keycap_size=$(this).text();
    set_keycap_size(keycap_size);
    save_setting('keycap_size',keycap_size);
    $("#modal_settings").focus();
});
//--
set_keyboard_bottom_margin(load_setting('keyboard_bottom_margin', '0px'));
function set_keyboard_bottom_margin(keyboard_bottom_margin) {
    document.querySelector(':root').style.setProperty('--keyboard_bottom_margin', keyboard_bottom_margin);
    $("#button_keyboard_bottom_margin").text(`keyboard bottom margin=${keyboard_bottom_margin}`);
}
$('#choose_keyboard_bottom_margin a').click(function () 
{
    var keyboard_bottom_margin=$(this).text();
    set_keyboard_bottom_margin(keyboard_bottom_margin);
    save_setting('keyboard_bottom_margin',keyboard_bottom_margin);
    $("#modal_settings").focus();
});
//----
set_keyboard_sound_volumne(load_setting('keyboard_sound_volumne', '50'));
function set_keyboard_sound_volumne(volumne) {
    keyboard_sound_volumne = 0.01 * volumne;
    $("#button_keyboard_sound_volumne").text(`key press sound volumne=${volumne}%`);
}
$('#choose_keyboard_sound_volumne a').click(function () 
{
    var sound_volumne=$(this).text();
    set_keyboard_sound_volumne(sound_volumne);
    save_setting('keyboard_sound_volumne',sound_volumne);
    $("#modal_settings").focus();
});
//----
set_keyboard_transparency(load_setting('keyboard_transparency', '0'));
function set_keyboard_transparency(value) {
    document.querySelector(':root').style.setProperty('--keyboard_opacity', `${100-value}%`);
    $("#button_keyboard_transparency").text(`keyboard transparency=${value}%`);
}
$('#choose_keyboard_transparency a').click(function () 
{
    let val=$(this).text();
    set_keyboard_transparency(val);
    save_setting('keyboard_transparency',val);
    $("#modal_settings").focus();
});
//---
key_haptic_feedback_switch = $('#key_haptic_feedback');
set_key_haptic_feedback = function(value){
    if ('vibrate' in navigator) {
        key_haptic_feedback = value;
        $('#key_haptic_feedback').prop('checked', value);
    } else {
        key_haptic_feedback=false;
        key_haptic_feedback_switch.prop('disabled', true);
    }
}

set_key_haptic_feedback(load_setting('key_haptic_feedback', true));
key_haptic_feedback_switch.change( function() {
    save_setting('key_haptic_feedback', this.checked);
    set_key_haptic_feedback(this.checked);
});



//----
    set_renderer_choice = function (choice) {
        $(`#button_renderer`).text('video renderer='+choice);
        save_setting("renderer",choice);
    }
    current_renderer=load_setting("renderer", "gpu shader");
    set_renderer_choice(current_renderer);

    $(`#choose_renderer a`).click(function () 
    {
        let choice=$(this).text();
        set_renderer_choice(choice);
        $("#modal_settings").focus();
    });

    if(call_param_gpu==true)
    {
        current_renderer="gpu shader";
    }
    let got_renderer=false;
    if(current_renderer=="gpu shader")
    {
        try{
            initWebGL();
            got_renderer=true;
        }
        catch(e){ console.error(e)}
    }
    else
    {
        create2d_context();
    }
    //got_renderer=wasm_create_renderer(current_renderer); 
    if(!got_renderer && current_renderer!='software')
    {
        alert('MESSAGE: gpu shader can not be created on your system configuration... switching back to software renderer...');
        set_renderer_choice('software')
        current_renderer='software';
        create2d_context();
    }


//----

set_display_choice = function (choice) {
    $(`#button_display`).text('visible display area='+choice);
    wasm_set_display(choice);
}
let current_display=load_setting("display", "standard");
set_display_choice(current_display);

$(`#choose_display a`).click(function () 
{
    let choice=$(this).text();
    set_display_choice(choice);
    save_setting("display",choice);
    $("#modal_settings").focus();
});

//---



    warp_switch = $('#warp_switch');
    var use_warp=load_setting('use_warp', false);
    warp_switch.prop('checked', use_warp);
    wasm_set_warp(use_warp ? 1:0);
    warp_switch.change( function() {
        wasm_set_warp(this.checked ? 1:0);
        save_setting('use_warp', this.checked);
    });

//----------

activity_monitor_switch

activity_monitor_switch = $('#activity_monitor_switch');
set_activity_monitor = function(value){
    if(value)
    {
        show_activity();
    }
    else
    {
        hide_activity(); 
    }
    activity_monitor_switch.prop('checked', value);
}    
set_activity_monitor(load_setting('activity_monitor', false));
activity_monitor_switch.change( function() {
    
    save_setting('activity_monitor', this.checked);
    set_activity_monitor(this.checked);
});
//----------
    pixel_art_switch = $('#pixel_art_switch');
    set_pixel_art = function(value){
        if(value)
        {
            $("#canvas").addClass("pixel_art");
        }
        else
        {
            $("#canvas").removeClass("pixel_art");
        }
        $('#pixel_art_switch').prop('checked', value);
    }    
    set_pixel_art(load_setting('pixel_art', true));
    pixel_art_switch.change( function() {
        pixel_art=this.checked;
        save_setting('pixel_art', this.checked);
        set_pixel_art(this.checked);
    });

//------
function bind_config(key, default_value){
    let config_switch = $('#'+key);
    let use_config=load_setting(key, default_value);
    config_switch.prop('checked', use_config);
    wasm_configure(key.substring(4),''+use_config);
    config_switch.change( function() {
        wasm_configure(key.substring(4),''+this.checked);
        save_setting(key, this.checked);
    });
}
bind_config("OPT_CLX_SPR_SPR", true);
bind_config("OPT_CLX_SPR_PLF", true);
bind_config("OPT_CLX_PLF_PLF", true);


function set_hardware(key, value)
{
    save_setting(key,value);
    wasm_configure(key.substring(4),value);
}

function validate_hardware()
{
    let agnes=load_setting("OPT_AGNUS_REVISION", 'ECS_1MB');
    let chip_ram=load_setting("OPT_CHIP_RAM", '512');
    if(agnes.startsWith("OCS") && chip_ram > 512)
    {
        alert(`${agnes} agnus can address max. 512KB. Correcting to highest possible setting.`);
        set_hardware("OPT_CHIP_RAM", '512');
        $(`#button_${"OPT_CHIP_RAM"}`).text("chip ram"+'='+'512 (corrected)');
    }
    else if(agnes== "ECS_1MB" && chip_ram > 1024)
    {
        alert(`${agnes} agnus can address max. 1024KB. Correcting to highest possible setting.`);
        set_hardware("OPT_CHIP_RAM", '1024');
        $(`#button_${"OPT_CHIP_RAM"}`).text("chip ram"+'='+'1024 (corrected)');
    }
}

validate_hardware();

function bind_config_choice(key, name, values, default_value, value2text=null, text2value=null, targetElement=null, updated_func=null){
    value2text = value2text == null ? (t)=>t: value2text;
    text2value = text2value == null ? (t)=>t: text2value;
    
    $(targetElement==null?'#hardware_settings':targetElement).append(
    `
    <div class="dropdown mr-1 mt-3">
        <button id="button_${key}" class="btn btn-primary dropdown-toggle text-right" type="button" data-toggle="dropdown" aria-haspopup="true" aria-expanded="false">
        ${name}
        </button>
        <div id="choose_${key}" class="dropdown-menu dropdown-menu-right" aria-labelledby="dropdownMenuButton">
        ${
            (function(vals){
                let list='';
                for(val of vals){
                    list+=`<a class="dropdown-item" href="#">${value2text(val)}</a>`;
                }
                return list;
            })(values)
        }
        </div>
    </div>
    `);

    let set_choice = function (choice) {
        $(`#button_${key}`).text(`${name}${name.length>0?'=':''}${choice}`);
        save_setting(key, text2value(choice));
        validate_hardware();

        let result=wasm_configure(key.substring(4),`${text2value(choice)}`);
        if(result.length>0)
        {
            alert(result);
            validate_hardware();
            wasm_power_on(1);
            return;
        }
        if(updated_func!=null)
            updated_func(choice);
    }
    set_choice(value2text(load_setting(key, default_value)));

    $(`#choose_${key} a`).click(function () 
    {
        let choice=$(this).text();
        set_choice(choice);
        $("#modal_settings").focus();
    });
}


bind_config_choice("OPT_BLITTER_ACCURACY", "blitter accuracy",['0','1','2'],'2');

show_drive_config = (c)=>{
    $('#div_drives').html(`
    ${wasm_get_config_item("DRIVE_CONNECT",0)==1?"<span>df0</span>":""} 
    ${wasm_get_config_item("DRIVE_CONNECT",1)==1?"<span>df1</span>":""} 
    ${wasm_get_config_item("DRIVE_CONNECT",2)==1?"<span>df2</span>":""} 
    ${wasm_get_config_item("DRIVE_CONNECT",3)==1?"<span>df3</span>":""}
    <br>(kickstart needs a reset to recognize new connected drives)
    `);
}

bind_config_choice("OPT_floppy_drive_count", "floppy drives",['1', '2', '3', '4'],'2',
null,null,null,show_drive_config);
$('#hardware_settings').append(`<div id="div_drives"style="font-size: smaller" class="ml-3 vbk_choice_text"></div>`);
show_drive_config();

bind_config_choice("OPT_DRIVE_SPEED", "floppy drive speed",['-1', '1', '2', '4', '8'],'1');

$('#hardware_settings').append(`<div class="mt-4">hardware settings</div><span style="font-size: smaller;">(shuts machine down on agnus model or memory change)</span>`);

bind_config_choice("OPT_AGNUS_REVISION", "agnus revision",['OCS_OLD','OCS','ECS_1MB','ECS_2MB'],'ECS_2MB');
bind_config_choice("OPT_DENISE_REVISION", "denise revision",['OCS','ECS'],'OCS');
bind_config_choice("OPT_CHIP_RAM", "chip ram",['256', '512', '1024', '2048'],'2048', (v)=>`${v} KB`, t=>parseInt(t));
bind_config_choice("OPT_SLOW_RAM", "slow ram",['0', '256', '512'],'0', (v)=>`${v} KB`, t=>parseInt(t));
bind_config_choice("OPT_FAST_RAM", "fast ram",['0', '256', '512','1024', '2048', '8192'],'2048', (v)=>`${v} KB`, t=>parseInt(t));

$('#hardware_settings').append("<div id='divCPU' style='display:flex;flex-direction:row'></div>");
bind_config_choice("OPT_CPU_REVISION", "CPU",[0,1,2], 0, 
(v)=>{ return (68000+v*10)},
(t)=>{
    let val = t;
    val = (val-68000)/10;
    return val;
}, "#divCPU");

bind_config_choice("OPT_CPU_OVERCLOCKING", "",[0,2,3,4,5,6,8,12,14], 0, 
(v)=>{ return Math.round((v==0?1:v)*7.09)+' MHz'},
(t)=>{
    let val =t.replace(' MHz','');
    val = Math.round(val /7.09);
    return val == 1 ? 0: val;
},"#divCPU");
$('#hardware_settings').append(`<div style="font-size: smaller" class="ml-3 vbk_choice_text">
<span>7.09 Mhz</span> is the original speed of a stock A1000 or A500 machine. For effective overclocking be sure to enable fast ram and disable slow ram otherwise the overclocked CPU will get blocked by chipset DMA. CPU speed is proportional to energy consumption.
</div>`);



//------

auto_snapshot_switch = $('#auto_snapshot_switch');
var take_auto_snapshots=load_setting('auto_snapshot_switch', false);
auto_snapshot_switch.prop('checked', take_auto_snapshots);
set_take_auto_snapshots(take_auto_snapshots ? 1:0);
auto_snapshot_switch.change( function() {
    set_take_auto_snapshots(this.checked ? 1:0);
    save_setting('auto_snapshot_switch', this.checked);
});

//------

ntsc_pixel_ratio_switch = $('#ntsc_pixel_ratio_switch');
use_ntsc_pixel=load_setting('ntsc_pixel', false);
wasm_set_display(use_ntsc_pixel ? 'ntsc':'pal');

ntsc_pixel_ratio_switch.prop('checked', use_ntsc_pixel);
ntsc_pixel_ratio_switch.change( function() {
    use_ntsc_pixel  = this.checked;
    save_setting('ntsc_pixel', this.checked);
    wasm_set_display(use_ntsc_pixel ? 'ntsc':'pal');
});
//------

wide_screen_switch = $('#wide_screen_switch');
use_wide_screen=load_setting('widescreen', false);
wide_screen_switch.prop('checked', use_wide_screen);
wide_screen_switch.change( function() {
    use_wide_screen  = this.checked;
    save_setting('widescreen', this.checked);
    scaleVMCanvas();
});
//------
  // create a reference for the wake lock
  wakeLock = null;


check_wake_lock = async () => {
    if(is_running())
    {
        if(wakeLock != null)
        {
//            alert("req");
            requestWakeLock();
        }
    }
    else
    {
        if(wakeLock != null)
        {
//            alert("release");
            wakeLock.release();
        }
    }
}

// create an async function to request a wake lock
requestWakeLock = async () => {
    try {
      wakeLock = await navigator.wakeLock.request('screen');

      // change up our interface to reflect wake lock active
      $("#wake_lock_status").text("(wake lock active, app will stay awake, no auto off)");
      wake_lock_switch.prop('checked', true);

      // listen for our release event
      wakeLock.onrelease = function(ev) {
        console.log(ev);
      }
      wakeLock.addEventListener('release', () => {
        // if wake lock is released alter the button accordingly
        if(wakeLock==null)
            $("#wake_lock_status").text(`(no wake lock, system will probably auto off and sleep after a while)`);
        else
            $("#wake_lock_status").text(`(wake lock released while pausing, system will probably auto off and sleep after a while)`);
        wake_lock_switch.prop('checked', false);

      });
    } catch (err) {
      // if wake lock request fails - usually system related, such as battery
      $("#wake_lock_status").text(`(no wake lock, system will probably auto off and sleep after a while). ${err.name}, ${err.message}`);
      wake_lock_switch.prop('checked', false);
      console.error(err);
//      alert(`error while requesting wakelock: ${err.name}, ${err.message}`);
    }
}

set_wake_lock = (use_wake_lock)=>{
    let is_supported=false;
    if ('wakeLock' in navigator) {
        is_supported = true;
    } else {
        wake_lock_switch.prop('disabled', true);
        $("#wake_lock_status").text("(wake lock is not supported on this browser, your system will decide when it turns your device off)");
    }
    if(is_supported && use_wake_lock)
    {
        requestWakeLock();
    }
    else if(wakeLock != null)
    {
        let current_wakelock=wakeLock;
        wakeLock = null;
        current_wakelock.release();
    }
}

wake_lock_switch = $('#wake_lock_switch');
let use_wake_lock=load_setting('wake_lock', false);
set_wake_lock(use_wake_lock);
wake_lock_switch.change( function() {
    let use_wake_lock  = this.checked;
    set_wake_lock(use_wake_lock);
    save_setting('wake_lock', this.checked);
});
//---
fullscreen_switch = $('#button_fullscreen');
if(document.fullscreenEnabled)
{
    fullscreen_switch.show();
    svg_fs_on=`<path d="M1.5 1a.5.5 0 0 0-.5.5v4a.5.5 0 0 1-1 0v-4A1.5 1.5 0 0 1 1.5 0h4a.5.5 0 0 1 0 1h-4zM10 .5a.5.5 0 0 1 .5-.5h4A1.5 1.5 0 0 1 16 1.5v4a.5.5 0 0 1-1 0v-4a.5.5 0 0 0-.5-.5h-4a.5.5 0 0 1-.5-.5zM.5 10a.5.5 0 0 1 .5.5v4a.5.5 0 0 0 .5.5h4a.5.5 0 0 1 0 1h-4A1.5 1.5 0 0 1 0 14.5v-4a.5.5 0 0 1 .5-.5zm15 0a.5.5 0 0 1 .5.5v4a1.5 1.5 0 0 1-1.5 1.5h-4a.5.5 0 0 1 0-1h4a.5.5 0 0 0 .5-.5v-4a.5.5 0 0 1 .5-.5z"/>`;
    $('#svg_fullscreen').html(svg_fs_on);

    addEventListener("fullscreenchange", () => {
        $('#svg_fullscreen').html(
            document.fullscreenElement?
            `<path d="M5.5 0a.5.5 0 0 1 .5.5v4A1.5 1.5 0 0 1 4.5 6h-4a.5.5 0 0 1 0-1h4a.5.5 0 0 0 .5-.5v-4a.5.5 0 0 1 .5-.5zm5 0a.5.5 0 0 1 .5.5v4a.5.5 0 0 0 .5.5h4a.5.5 0 0 1 0 1h-4A1.5 1.5 0 0 1 10 4.5v-4a.5.5 0 0 1 .5-.5zM0 10.5a.5.5 0 0 1 .5-.5h4A1.5 1.5 0 0 1 6 11.5v4a.5.5 0 0 1-1 0v-4a.5.5 0 0 0-.5-.5h-4a.5.5 0 0 1-.5-.5zm10 1a1.5 1.5 0 0 1 1.5-1.5h4a.5.5 0 0 1 0 1h-4a.5.5 0 0 0-.5.5v4a.5.5 0 0 1-1 0v-4z"/>`:
            svg_fs_on
        );
        $('#svg_fullscreen').attr("data-original-title",
            document.fullscreenElement? "exit fullscreen":"fullscreen"
        );
    });

    fullscreen_switch.click( ()=>{	
        if(!document.fullscreenElement)
            document.documentElement.requestFullscreen({navigationUI: "hide"});
        else
            document.exitFullscreen();            
    });
}
else
{
    fullscreen_switch.hide();
}
//------

$('.layer').change( function(event) {
    //recompute stencil cut out layer value
    const layers={
        sprite0: 0x01,
        sprite1: 0x02,
        sprite2: 0x04,
        sprite3: 0x08,
        sprite4: 0x10,
        sprite5: 0x20,
        sprite6: 0x40,
        sprite7: 0x80,
        playfield1: 0x100,
        playfield2: 0x200        
    };

    var layer_value = 0;
    for(var layer_id in layers)
    {
        if(document.getElementById(layer_id).checked)
        {
            layer_value |= layers[layer_id];
        }
    }
    wasm_cut_layers( layer_value );
});

//------
    load_console=function () { var script = document.createElement('script'); script.src="js/eruda.js"; document.body.appendChild(script); script.onload = function () { eruda.init(
    {
        defaults: {
            displaySize: 50,
            transparency: 0.9,
            theme: load_setting('dark_switch', true) ? 'dark':'light'
        }
        }) } 
    }

    live_debug_output=load_setting('live_debug_output', false);
    wasm_configure("log_on", live_debug_output.toString());
    $("#cb_debug_output").prop('checked', live_debug_output);
    if(live_debug_output)
    {
        load_console();
     //   $("#output_row").show(); 
        $("#output_row").hide(); 
    }
    else
    {
//        eruda.destroy();
        $("#output_row").hide(); 
    }

    $("#cb_debug_output").change( function() {
        live_debug_output=this.checked;
        wasm_configure("log_on",live_debug_output.toString());
        save_setting('live_debug_output', this.checked);
        if(this.checked)
        {
           load_console();
           //$("#output_row").show();
        }
        else
        {
           eruda.destroy();
        //    $("#output_row").hide();
        }
        $("#output_row").hide();
    });



    $('#modal_reset').keydown(event => {
            if(event.key === "Enter")
            {
                $( "#button_reset_confirmed" ).click();                        
            }
            return true;
        }
    );
    document.getElementById('button_reset').onclick = function() {
        $("#modal_reset").modal('show');
    }
    document.getElementById('button_reset_confirmed').onclick = function() {
        setTimeout(release_modifiers, 0);
        wasm_reset();

        if(!is_running())
        {
            $("#button_run").click();
        }
        $("#modal_reset").modal('hide').blur();
    }


    running=false;
    emulator_currently_runs=false;
    $("#button_run").click(function() {
        hide_all_tooltips();
        if(running)
        {        
            wasm_halt();
            try { audioContext.suspend(); } catch(e){ console.error(e);}
            running = false;
            //set run icon
            $('#button_run').html(`<svg class="bi bi-play-fill" width="1.6em" height="1.6em" viewBox="0 0 16 16" fill="currentColor" xmlns="http://www.w3.org/2000/svg">
            <path d="M11.596 8.697l-6.363 3.692c-.54.313-1.233-.066-1.233-.697V4.308c0-.63.692-1.01 1.233-.696l6.363 3.692a.802.802 0 0 1 0 1.393z"/>
          </svg>`).parent().attr("title", "run").attr("data-original-title", "run");
        }
        else
        {
            //set pause icon
            $('#button_run').html(`<svg class="bi bi-pause-fill" width="1.6em" height="1.6em" viewBox="0 0 16 16" fill="currentColor" xmlns="http://www.w3.org/2000/svg">
            <path d="M5.5 3.5A1.5 1.5 0 0 1 7 5v6a1.5 1.5 0 0 1-3 0V5a1.5 1.5 0 0 1 1.5-1.5zm5 0A1.5 1.5 0 0 1 12 5v6a1.5 1.5 0 0 1-3 0V5a1.5 1.5 0 0 1 1.5-1.5z"/>
          </svg>`).parent().attr("title", "pause").attr("data-original-title", "pause");

            //have to catch an intentional "unwind" exception here, which is thrown
            //by emscripten_set_main_loop() after emscripten_cancel_main_loop();
            //to simulate infinity gamelloop see emscripten API for more info ... 
            try{wasm_run();} catch(e) {}        
            try {connect_audio_processor();} catch(e){ console.error(e);}
            running = true;
        }
        check_wake_lock();        
        //document.getElementById('canvas').focus();
    });


    $('#modal_file_slot').on('hidden.bs.modal', function () {
        $("#filedialog").val(''); //clear file slot after file has been loaded
        document.getElementById('modal_file_slot').blur(); //needed for safari issue#136
    });

    $( "#modal_file_slot" ).keydown(event => {
            if(event.key === "Enter" && $("#button_insert_file").attr("disabled")!=true)
            {
                $( "#button_insert_file" ).click();                        
            }
            return false;
        }
    );

    reset_before_load=false;
    insert_file = function(drive=0) 
    {   
        $('#modal_file_slot').modal('hide');

        var execute_load = async function(drive){
            var filetype = wasm_loadfile(file_slot_file_name, file_slot_file, drive);

            if(auto_selecting_app_title)
            {
                //if it is a disk from a multi disk zip file, apptitle should be the name of the zip file only
                //instead of disk1, disk2, etc....
                if(last_zip_archive_name !== null)
                {
                    global_apptitle = last_zip_archive_name;
                }
                else
                {
                    global_apptitle = file_slot_file_name;
                }

                get_custom_buttons(global_apptitle, 
                    function(the_buttons) {
                        custom_keys = the_buttons;
                        for(let param_button of call_param_buttons)
                        {
                            custom_keys.push(param_button);
                        }
                        install_custom_keys();
                    }
                );
            }
            if(call_param_dialog_on_disk == false)
            {//loading is probably done by scripting
            }
        };

        if(!is_running())
        {
            $("#button_run").click();
        }

        if(reset_before_load == false)
        {
            execute_load(drive);
        }
        else
        {
            setTimeout(async ()=> {
                await execute_load(drive);
                wasm_reset();
                if(call_param_warpto !=null){
                    wasm_configure("warp_to_frame", `${call_param_warpto}`);
                }
            },0);
        }
    }
    $("#button_insert_file").click(()=>{
         prompt_for_drive();
    });
    
    $('#modal_take_snapshot').on('hidden.bs.modal', function () {
        if(is_running())
        {
            setTimeout(function(){try{wasm_run();} catch(e) {}},200);
        }
    }).keydown(event => {
            if(event.key === "Enter")
            {
                $( "#button_save_snapshot" ).click();                        
            }
            return true;
        }
    );
   

    $('#modal_settings').on('show.bs.modal', function() 
    {    
        for(var dn=0; dn<4; dn++)
        {
            if(wasm_has_disk("df"+dn))
            {
                $("#button_eject_df"+dn).show();
            }
            else
            {
                $("#button_eject_df"+dn).hide();
            }
            $('#button_eject_df'+dn).click(function() 
            {
                wasm_eject_disk("df"+this.id.at(-1));
                $("#button_eject_df"+this.id.at(-1)).hide();
            });
            if(wasm_has_disk("dh"+dn))
            {
                $("#button_eject_hd"+dn).show();
            }
            else
            {
                $("#button_eject_hd"+dn).hide();
            }
            $('#button_eject_hd'+dn).click(function() 
            {
                wasm_eject_disk("dh"+this.id.at(-1));
                $("#button_eject_hd"+this.id.at(-1)).hide();
            });
        }
    });

    document.getElementById('button_take_snapshot').onclick = function() 
    {       
        wasm_halt();
        $("#modal_take_snapshot").modal('show');
        $("#input_app_title").val(global_apptitle);
        $("#input_app_title").focus();

        for(var dfn=0; dfn<4; dfn++)
        {
            if(wasm_has_disk("df"+dfn))
            {
                $("#button_export_df"+dfn).show();
            }
            else
            {
                $("#button_export_df"+dfn).hide();
            }
            if(wasm_has_disk("dh"+dfn))
            {
                $("#button_export_hd"+dfn).show();
            }
            else
            {
                $("#button_export_hd"+dfn).hide();
            }
        }
    }
    for(var dn=0; dn<4; dn++)
    {
        $('#button_export_df'+dn).click(function() 
        {
            let d64_json = wasm_export_disk("df"+this.id.at(-1));
            let d64_obj = JSON.parse(d64_json);
            let d64_buffer = new Uint8Array(Module.HEAPU8.buffer, d64_obj.address, d64_obj.size);
            let filebuffer = d64_buffer.slice(0,d64_obj.size);
            let blob_data = new Blob([filebuffer], {type: 'application/octet-binary'});
            Module._wasm_delete_disk();
            
            const url = window.URL.createObjectURL(blob_data);
            const a = document.createElement('a');
            a.style.display = 'none';
            a.href = url;

            let app_name = $("#input_app_title").val();
            let extension_pos = app_name.indexOf(".");
            if(extension_pos >=0)
            {
                app_name = app_name.substring(0,extension_pos);
            }
            a.download = app_name+'.adf';
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
        });
        $('#button_export_hd'+dn).click(function() 
        {
            let d64_json = wasm_export_disk("dh"+this.id.at(-1));
            let d64_obj = JSON.parse(d64_json);
            let d64_buffer = new Uint8Array(Module.HEAPU8.buffer, d64_obj.address, d64_obj.size);
            let filebuffer = d64_buffer.slice(0,d64_obj.size);
            let blob_data = new Blob([filebuffer], {type: 'application/octet-binary'});
            Module._wasm_delete_disk();

            const url = window.URL.createObjectURL(blob_data);
            const a = document.createElement('a');
            a.style.display = 'none';
            a.href = url;

            let app_name = $("#input_app_title").val();
            let extension_pos = app_name.indexOf(".");
            if(extension_pos >=0)
            {
                app_name = app_name.substring(0,extension_pos);
            }
            a.download = app_name+'.hdf';
            document.body.appendChild(a);
            a.click();
            window.URL.revokeObjectURL(url);
        });
    }
    $('#button_save_snapshot').click(async function() 
    {       
        let app_name = $("#input_app_title").val();
        wasm_take_user_snapshot();
        var snapshot_json= wasm_pull_user_snapshot_file();
        var snap_obj = JSON.parse(snapshot_json);
//        var ptr=wasm_pull_user_snapshot_file();
//        var size = wasm_pull_user_snapshot_file_size();
        var snapshot_buffer = new Uint8Array(Module.HEAPU8.buffer, snap_obj.address, snap_obj.size);
   
        //snapshot_buffer is only a typed array view therefore slice, which creates a new array with byteposition 0 ...
        await save_snapshot(app_name, snapshot_buffer.slice(0,snap_obj.size));
        wasm_delete_user_snapshot();
        $("#modal_take_snapshot").modal('hide');
        //document.getElementById('canvas').focus();
    });

    set_color_palette(load_setting('color_palette', 'color'));
    function set_color_palette(color_palette) {
        $("#button_color_palette").text(color_palette.replace("_", " "));
        wasm_set_color_palette(color_palette);
    }
    $('#choose_color_palette a').click(function () 
    {
        var color_palette=$(this).text();
        set_color_palette(color_palette);
        save_setting('color_palette',color_palette);
        $("#modal_settings").focus();
    });

//---------- update management --------

    set_settings_cache_value = async function (key, value)
    {
        let settings = await caches.open('settings');
        await settings.put(key, new Response(value) );
    }
    get_settings_cache_value= async function (key)
    {
        try {
            let settings = await caches.open('settings');
            let response=await settings.match(key)
            if(response==undefined)
                return null;
            return await response.text();    
        }
        catch(e){
            console.error(e);
            return "can't read version";
        }
    }

    execute_update = async function() 
    {
        let current_version= await get_settings_cache_value('active_version');
        if(current_version == null)
        {//no version management then clear all caches
            let keys = await caches.keys();
            console.log('deleting cache files:'+keys);
            await Promise.all(keys.map(key => caches.delete(key)));
        }
        if(typeof sw_version != 'undefined')
        {
            set_settings_cache_value('active_version', sw_version.cache_name);        
        }
        window.location.reload();
    }
    
    $("#div_toast").hide();
    show_new_version_toast= ()=>{
        $("#div_toast").show();
        $(".toast").toast({autohide: false});
        $('.toast').toast('show');
    }
    $('.toast').on('hidden.bs.toast', function () {
        $("#div_toast").hide();
    });

    has_installed_version=async function (cache_name){
        let cache_names=await caches.keys();
        for(c_name of cache_names)
            if(c_name == cache_name)
                return true;        
        return false;
    }
    get_current_ui_version = async function (){
        current_version = await get_settings_cache_value("active_version");
        
        current_ui='unkown';
        if(current_version != null)
        {
            current_ui=current_version.split('@')[1];
        }
    }
    try{
        //when the serviceworker talks with us ...  
        navigator.serviceWorker.addEventListener("message", async (evt) => {
            await get_current_ui_version();
            let cache_names=null;
            try{
                cache_names=await caches.keys();
            }
            catch(e)
            {
                console.error(e);
                return;
            }
            let version_selector = `
            manage already installed versions:
            <br>
            <div style="display:flex">
            <select id="version_selector" class="ml-2">`;
            for(c_name of cache_names)
            {
                let name_parts=c_name.split('@');
                let core_name= name_parts[0];
                let ui_name= name_parts[1];
                let selected=c_name==current_version?"selected":"";

                if(c_name.includes('@'))
                {   
                    if(//uat version should not show regular versions and vice versa
                        location.pathname.startsWith("/uat") ?
                            ui_name.endsWith("uat")
                        :
                            !ui_name.endsWith("uat")
                    )
                    {
                        version_selector+=`<option ${selected} value="${c_name}">core ${core_name}, ui ${ui_name}</option>`;
                    }
                }
            }
            version_selector+=
            `</select>
            
            <button type="button" id="activate_version" disabled class="btn btn-primary btn-sm px-1 mx-1">activate</button>
            <button type="button" id="remove_version" class="btn btn-danger btn-sm px-1 mx-1"><svg style="width:1.5em;height:1.5em"><use xlink:href="img/sprites.svg#trash"/></svg>
            </button>
            </div>
            `;

            //2. diese vergleichen mit der des Service workers
            sw_version=evt.data;
            if(sw_version.cache_name != current_version)
            {
                let new_version_already_installed=await has_installed_version(sw_version.cache_name);
                let new_version_installed_or_not = new_version_already_installed ?
                `newest version (already installed)`:
                `new version available`;

                let activate_or_install = `
                <button type="button" id="activate_or_install" class="btn btn-${new_version_already_installed ?"primary":"success"} btn-sm px-1 mx-1">${
                    new_version_already_installed ? "activate": "install"
                }</button>`;



                let upgrade_info = `    
                currently active version (old):<br>
                <div style="display:flex">
                <span class="ml-2 px-1 py-1 outlined">core <i>${wasm_get_core_version()}</i></span> <span class="ml-2 px-1 py-1 outlined">ui <i>${current_ui}</i></span>
                </div><br>
                <span id="new_version_installed_or_not">${new_version_installed_or_not}</span>:<br> 
                <div style="display:flex">
                <span class="ml-2 px-1 py-1 outlined">core <i>${sw_version.core}</i></span> <span class="ml-2 px-1 py-1 outlined">ui <i>${sw_version.ui}</i></span> ${activate_or_install}
                </div>
                <div id="install_warning" class="my-1">Did you know that upgrading the core may break your saved snapshots?<br/>
                In that case you can still select and activate an older compatible installation to run it ...
                </div>`;

                $('#update_dialog').html(upgrade_info);
                $('#activate_or_install').remove();
                $('#install_warning').remove();
                $('#version_display').html(`${upgrade_info} 
                <br>
                ${version_selector}`);
                if(!new_version_already_installed)
                {
                    show_new_version_toast();
                }
            }
            else
            {
                $("#version_display").html(`
                currently active version (newest):<br>
                <div style="display:flex">
                <span class="ml-2 px-1 py-1 outlined">core <i>${wasm_get_core_version()}</i></span> <span class="ml-2 px-1 py-1 outlined">ui <i>${current_ui}</i></span>
                <button type="button" id="activate_or_install" class="btn btn-success btn-sm px-1 py-1">
                install</button>
                </div>
                <br>
                ${version_selector}`
                );
                $("#activate_or_install").hide();
            }
            document.getElementById('version_selector').onchange = function() {
                let select = document.getElementById('version_selector');
                document.getElementById('activate_version').disabled=
                    (select.options[select.selectedIndex].value == current_version);
            }
            document.getElementById('remove_version').onclick = function() {
                let select = document.getElementById('version_selector');
                let cache_name = select.value;
                if(cache_name == sw_version.cache_name)
                {
                    $("#new_version_installed_or_not").text("new version available");
                    $("#activate_or_install").text("install").attr("class","btn btn-success btn-sm px-1 mx-1").show();
                }
                caches.delete(cache_name);
                select.options[select.selectedIndex].remove();
                if(current_version == cache_name)
                {//when removing the current active version, activate another installed version
                    if(select.options.length>0)
                    {
                        select.selectedIndex=select.options.length-1;
                        set_settings_cache_value("active_version",select.options[select.selectedIndex].value); 
                    }
                    else
                    {
                        set_settings_cache_value("active_version",sw_version.cache_name); 
                    }   
                }
                if(select.options.length==0)
                {
                    document.getElementById('remove_version').disabled=true;        
                    document.getElementById('activate_version').disabled=true;
                }
                else 
                {
                    document.getElementById('activate_version').disabled=
                    (select.options[select.selectedIndex].value == current_version);
                }
            }
            document.getElementById('activate_version').onclick = function() {
                let cache_name = document.getElementById('version_selector').value; 
                set_settings_cache_value("active_version",cache_name);
                window.location.reload();
            }
            let activate_or_install_btn = document.getElementById('activate_or_install');
            if(activate_or_install_btn != null)
            {
                activate_or_install_btn.onclick = () => {
                    (async ()=>{
                        let new_version_already_installed=await has_installed_version(sw_version.cache_name); 
                        if(new_version_already_installed)
                        {
                            set_settings_cache_value("active_version",sw_version.cache_name);
                            window.location.reload();
                        }
                        else
                        {
                            execute_update();
                        }
                    })();
                }
            }        
        });


        // ask service worker to send us a version message
        // wait until it is active
        navigator.serviceWorker.ready
        .then( (registration) => {
            if (registration.active) {
                registration.active.postMessage('version');
            }
        });

        //in the meantime until message from service worker has not yet arrived show this
        let init_version_display= async ()=>{
            await get_current_ui_version();
            $("#version_display").html(`
            currently active version:<br>
            <span class="ml-2 px-1 outlined">core <i>${wasm_get_core_version()}</i></span> <span class="ml-2 px-1 outlined">ui <i>${current_ui}</i></span>
            <br><br>
            waiting for service worker...`
            );
        };
        init_version_display();
    } catch(e)
    {
        console.error(e.message);
    }
//------- update management end ---

    setup_browser_interface();

    document.getElementById('port1').onchange = function() {
        port1 = document.getElementById('port1').value; 
        if(port1 == port2 || 
           port1.indexOf("touch")>=0 && port2.indexOf("touch")>=0)
        {
            port2 = 'none';
            document.getElementById('port2').value = 'none';
        }
        //document.getElementById('canvas').focus();

        if(v_joystick == null && port1 == 'touch')
        {
            register_v_joystick();
            install_custom_keys();
        }
        if(port1 != 'touch' && port2 != 'touch')
        {
            unregister_v_joystick();
        }
        if(port1 == 'mouse')
        {                
            mouse_port=1;    
            canvas.addEventListener('click', request_pointerlock);
            request_pointerlock();
        }
        else if(port2 != 'mouse')
        {
            canvas.removeEventListener('click', request_pointerlock);
            remove_pointer_lock_fallback();
        }
        if(port1.startsWith('mouse touch'))
        {
            mouse_touchpad_pattern=port1;
            mouse_touchpad_port=1;
            document.addEventListener('touchstart',emulate_mouse_touchpad_start, false);
            document.addEventListener('touchmove',emulate_mouse_touchpad_move, false);
            document.addEventListener('touchend',emulate_mouse_touchpad_end, false);
        }
        else if(!port2.startsWith('mouse touch'))
        {
            document.removeEventListener('touchstart',emulate_mouse_touchpad_start, false);
            document.removeEventListener('touchmove',emulate_mouse_touchpad_move, false);
            document.removeEventListener('touchend',emulate_mouse_touchpad_end, false);
        }
        this.blur();
    }
    document.getElementById('port2').onchange = function() {
        port2 = document.getElementById('port2').value;
        if(port1 == port2 || 
           port1.indexOf("touch")>=0 && port2.indexOf("touch")>=0)
        {
            port1 = 'none';
            document.getElementById('port1').value = 'none';
        }
        //document.getElementById('canvas').focus();

        if(v_joystick == null && port2 == 'touch')
        {
            register_v_joystick();
            install_custom_keys();
        }
        if(port1 != 'touch' && port2 != 'touch')
        {
            unregister_v_joystick();
        }
        if(port2 == 'mouse')
        {                
            mouse_port=2;
            canvas.addEventListener('click', request_pointerlock);
            request_pointerlock();
        }
        else if(port1 != 'mouse')
        {
            canvas.removeEventListener('click', request_pointerlock);
            remove_pointer_lock_fallback();
        }
        if(port2.startsWith('mouse touch'))
        {
            mouse_touchpad_pattern=port2;
            mouse_touchpad_port=2;
            document.addEventListener('touchstart',emulate_mouse_touchpad_start, false);
            document.addEventListener('touchmove',emulate_mouse_touchpad_move, false);
            document.addEventListener('touchend',emulate_mouse_touchpad_end, false);
        }
        else if(!port1.startsWith('mouse touch'))
        {
            document.removeEventListener('touchstart',emulate_mouse_touchpad_start, false);
            document.removeEventListener('touchmove',emulate_mouse_touchpad_move, false);
            document.removeEventListener('touchend',emulate_mouse_touchpad_end, false);
        }
        this.blur();
    }


    document.getElementById('theFileInput').addEventListener("submit", function(e) {
        e.preventDefault();
        handleFileInput();
    }, false);

    document.getElementById('drop_zone').addEventListener("click", function(e) {
        if(last_zip_archive_name != null)
        {
            file_slot_file_name = last_zip_archive_name;
            file_slot_file = last_zip_archive;
            configure_file_dialog();
        }
        else
        {
            document.getElementById('theFileInput').elements['theFileDialog'].click();
        }
    }, false);

    document.getElementById('drop_zone').addEventListener("dragover", function(e) {
        dragover_handler(e);
    }, false);

    document.getElementById('drop_zone').addEventListener("drop", function(e) {
        drop_handler(e);
    }, false);
    document.getElementById('filedialog').addEventListener("change", function(e) {
          handleFileInput();
    }, false);

    $("html").on('dragover', function(e) {dragover_handler(e.originalEvent); return false;}) 
    .on('drop', function (e) {
        drop_handler( e.originalEvent );
        return false;
    });



//---- rom dialog start
    rom_restored_from_snapshot=false;
    fill_available_roms=async function (rom_type, select_id){
        let stored_roms=await list_rom_type_entries(rom_type);
        let html_rom_list=`<option value="empty">empty</option>`;
        if(rom_restored_from_snapshot)
        {
            html_rom_list+=`<option value="restored_from_snapshot" hidden selected>restored from snapshot</option>`
        }
        let selected_rom=local_storage_get(rom_type);
        for(rom of stored_roms)
        {
            html_rom_list+= `<option value="${rom.id}"`;
            if(!rom_restored_from_snapshot)
            {
                html_rom_list+=`${selected_rom ==rom.id?"selected":""}`;
            }
            html_rom_list+= `>${rom.id}</option>`;
        }
        $(`#${select_id}`).html(html_rom_list);

        document.getElementById(select_id).onchange = function() {
            let selected_rom = document.getElementById(select_id).value; 
            save_setting(rom_type, selected_rom);
            rom_restored_from_snapshot=false;
            load_roms(true);
        }
    }

   document.getElementById('button_rom_dialog').addEventListener("click", async function(e) {
     $('#modal_settings').modal('hide');
     load_roms(false); //update to current roms

     setTimeout(function() { $('#modal_roms').modal('show');}, 500);
   }, false);


   document.getElementById('button_fetch_open_roms').addEventListener("click", function(e) {
       fetchOpenROMS();
   }, false);

   
   var bindROMUI = function (id_dropzone, id_delete, id_local_storage) 
   {
        document.getElementById(id_dropzone).addEventListener("click", function(e) {
            document.getElementById('theFileInput').elements['theFileDialog'].click();
        }, false);

        document.getElementById(id_dropzone).addEventListener("dragover", function(e) {
            dragover_handler(e);
        }, false);

        document.getElementById(id_dropzone).addEventListener("drop", function(e) {
            drop_handler(e);
        }, false);

        document.getElementById(id_delete).addEventListener("click", function(e) {
            let selected_rom=local_storage_get(id_local_storage);
            save_setting(id_local_storage, null);
            delete_rom(selected_rom);
            load_roms(true);
        }, false);
    }
    bindROMUI('rom_kickstart', 'button_delete_kickstart', "rom");
    bindROMUI('rom_kickstart_ext', 'button_delete_kickstart_ext', "rom_ext");

//---- rom dialog end

    document.addEventListener('keyup', keyup, false);
    document.addEventListener('keydown', keydown, false);

    window.addEventListener("gamepadconnected", function(e) {
        var gp = navigator.getGamepads()[e.gamepad.index];
        console.log("Gamepad connected at index %d: %s. %d buttons, %d axes.",
            gp.index, gp.id,
            gp.buttons.length, gp.axes.length);

        var sel1 = document.getElementById('port1');
        var opt1 = document.createElement('option');
        opt1.appendChild( document.createTextNode(gp.id) );
        opt1.value = e.gamepad.index; 
        sel1.appendChild(opt1); 

        var sel2 = document.getElementById('port2');
        var opt2 = document.createElement('option');
        opt2.appendChild( document.createTextNode(gp.id) );
        opt2.value = e.gamepad.index; 
        sel2.appendChild(opt2); 
    });
    window.addEventListener("gamepaddisconnected", (event) => {
        console.log("A gamepad disconnected:");
        console.log(event.gamepad);
        var sel1 = document.getElementById('port1');       
        for(var i=0; i<sel1.length; i++)
        {
            if(sel1.options[i].value == event.gamepad.index)
            {
                sel1.removeChild( sel1.options[i] ); 
                break;
            }
        }
        var sel2 = document.getElementById('port2');       
        for(var i=0; i<sel2.length; i++)
        {
            if(sel2.options[i].value == event.gamepad.index)
            {
                sel2.removeChild( sel2.options[i] ); 
                break;
            }
        }
    });

    scaleVMCanvas();


    var bEnableCustomKeys = true;
    if(!bEnableCustomKeys)
    {
        $("#button_custom_key").remove();
    }
    if(bEnableCustomKeys)
    {
        create_new_custom_key = false;
        $("#button_custom_key").click(
            function(e) 
            {  
                create_new_custom_key = true;
                $('#input_button_text').val('');
                $('#input_action_script').val('');
 
                $('#modal_custom_key').modal('show');
            }
        );

        reconfig_editor = function(new_lang){
            if(typeof editor !== 'undefined' )
            {
                editor.setOption("lineNumbers", new_lang == 'javascript');
                if(new_lang=='javascript')
                {
                    editor.setOption("gutters", ["CodeMirror-lint-markers"]);
                    editor.setOption("lint", { esversion: 10});
                    $('#check_autocomplete').show();
                    $('#check_livecomplete').prop('checked', true);                 
                    editor.setOption('placeholder', "add example code with the menu button 'add'->'javascript'");
                }
                else
                {
                    editor.setOption("gutters", false);
                    editor.setOption("lint", false);
                    $('#check_autocomplete').hide();
                    editor.setOption('placeholder', "type a single key like 'B' for bomb ... or compose a sequence of actions separated by '=>'");
                }
            }
        }
        set_complete_label = function(){
          $('#check_livecomplete_label').text(  
              $('#check_livecomplete').prop('checked') ?
              "live complete":"autocomplete on ctrl+space");
        }
        $('#check_livecomplete').change( function(){ 
            set_complete_label();
            editor.focus();
        });

        short_cut_input = document.getElementById('input_button_shortcut');
        button_delete_shortcut=$("#button_delete_shortcut");
        short_cut_input.addEventListener(
            'keydown',
            (e)=>{
                e.preventDefault();
                e.stopPropagation();
                short_cut_input.value=serialize_key_code(e);
                button_delete_shortcut.prop('disabled', false);
                validate_custom_key();
            }
        );
        short_cut_input.addEventListener(
            'keyup',
            (e)=>{
                e.preventDefault();
                e.stopPropagation();
            }
        );
        short_cut_input.addEventListener(
            'blur',
            (e)=>{
                e.preventDefault();
                e.stopPropagation();
                short_cut_input.value=short_cut_input.value.replace('^','').replace('^','');
            }
        );
        button_delete_shortcut.click(()=>{
            
            short_cut_input.value='';
            button_delete_shortcut.prop('disabled', true);
            validate_custom_key();
        });

        $('#modal_custom_key').on('show.bs.modal', function () {
            bind_custom_key();    
        });

        bind_custom_key = async function () {
            $('#choose_padding a').click(function () 
            {
                 $('#button_padding').text('btn size = '+ $(this).text() ); 
            });
            $('#choose_opacity a').click(function () 
            {
                 $('#button_opacity').text('btn opacity = '+ $(this).text() ); 
            });

            function set_script_language(script_language) {
                $("#button_script_language").text(script_language);;
            }
            $('#choose_script_language a').click(function () 
            {
                let new_lang = $(this).text();
                set_script_language(new_lang);
                validate_action_script();

                reconfig_editor(new_lang);
                editor.focus();
            });

            let otherButtons=`<a class="dropdown-item ${create_new_custom_key?"active":""}" href="#" id="choose_new">&lt;new&gt;</a>`;
            for(let otherBtn of custom_keys)
            {
                let keys = otherBtn.key.split('+');
                let key_display="";
                for(key of keys)
                {
                    key_display+=`<div class="px-1" style="border-radius: 0.25em;margin-left:0.3em;background-color: var(--gray);color: white">
                    ${html_encode(key)}
                    </div>`;
                }
                otherButtons+=`<a class="dropdown-item ${!create_new_custom_key &&haptic_touch_selected.id == 'ck'+otherBtn.id ? 'active':''}" href="#" id="choose_${otherBtn.id}">
                <div style="display:flex;justify-content:space-between">
                    <div>${html_encode(otherBtn.title)}</div>
                    <div style="display:flex;margin-left:0.3em;" 
                        ${otherBtn.key==''?'hidden':''}>
                        ${key_display}
                    </div>
                </div></a>`;
            }
            
            group_list = await stored_groups();
            group_list = group_list.filter((g)=> g !== '__global_scope__');
            if(!group_list.includes(global_apptitle))
            {
                group_list.push(global_apptitle);
            }
            other_groups=``;
            for(group_name of group_list)
            {   
                other_groups+=`
            <option ${group_name === global_apptitle?"selected":""} value="${group_name}">${group_name}</option>
            `;
            }

            $('#select_other_group').tooltip('hide');

            if($("#other_buttons").html().length==0)
            {
                $("#other_buttons").html(`
                <div class="dropdown">
                    <button id="button_other_action" class="ml-4 py-0 btn btn-primary dropdown-toggle text-right" type="button" data-toggle="dropdown" aria-haspopup="true" aria-expanded="false">
                    list (${custom_keys.length})
                    </button>
                    <div id="choose_action" style="min-width:250px" class="dropdown-menu dropdown-menu-right" aria-labelledby="dropdownMenuButton">
                    
                    <div style="width:100%;display:flex;justify-content:center">
                        <select onclick="event.stopPropagation();" id="select_other_group" 
                            style="width:95%"
                            class="custom-select" data-placement="left" data-toggle="tooltip" title="action button group">
                        ${other_groups}
                        </select>
                    </div>
                    <div id="other_buttons_content">
                    ${otherButtons}
                    </div>
                    <div class="mt-3 px-2" style="width:100%;display: grid;grid-template-columns: 1.5fr 1fr;">
                    <button type="button" id="button_delete_group" class="btn btn-danger justify-start">delete group</button>
                    <button type="button" id="button_new_group" class="btn btn-primary justify-end" style="grid-column:2/2">+ group</button>
                    </div>

                    </div>
                </div>`
                );

                document.getElementById("button_delete_group").addEventListener("click",
                    (e)=>{
                        e.stopPropagation();
                        if(confirm(`delete all actions specific to group ${global_apptitle}?`))
                        {
                            delete_button_group(global_apptitle);
                            switch_to_other_group(default_app_title);                       
                        }    
                    }, false
                );
                document.getElementById("button_new_group").addEventListener("click",
                    (e)=>{
                        e.stopPropagation();    
                        let new_group_name = prompt(`group name`);
                        if(new_group_name!==null)
                        {
                            save_new_empty_group(new_group_name);
                            switch_to_other_group(new_group_name);
                        }
                    }, false
                );
                document.getElementById('select_other_group').onchange = function() {
                    let title=document.getElementById('select_other_group').value; 
                    switch_to_other_group(title)
                }
            }
            else
            {
                $("#button_other_action").html(`list (${custom_keys.length})`);
                $("#other_buttons_content").html(otherButtons);
                $("#select_other_group").html(other_groups);
            }

            //dont show delete group when on default group and no actions in it
            if(otherButtons.length===0 && global_apptitle === default_app_title)
                $("#button_delete_group").hide();
            else
                $("#button_delete_group").show();


            $('#choose_action a').click(function () 
            {
                let btn_id = this.id.replace("choose_","");
                if(btn_id == "new"){
                    if(create_new_custom_key)
                        return;
                    create_new_custom_key=true;
                }
                else
                {
                    create_new_custom_key=false;
                    var btn_def = custom_keys.find(el=>el.id == btn_id);
                   
                    haptic_touch_selected= {id: 'ck'+btn_def.id};
                }
                bind_custom_key();
                reset_validate();
            });


            switch_to_other_group=(title)=>
            {
                global_apptitle = title; 
             
                get_custom_buttons(global_apptitle, 
                    function(the_buttons) {
                        custom_keys = the_buttons;
                        for(let param_button of call_param_buttons)
                        {
                            custom_keys.push(param_button);
                        }

                        create_new_custom_key=true;
                        
                        install_custom_keys();
                        bind_custom_key();
                    }
                );
            }

            reset_validate();
            if(create_new_custom_key)
            {
                $('#button_delete_custom_button').hide();
                $('#check_app_scope').prop('checked',true);
                $('#input_button_text').val('');
                $('#input_button_shortcut').val('');

                $('#input_action_script').val('');
                if(typeof(editor) !== 'undefined') editor.getDoc().setValue("");
                $('#button_reset_position').prop('disabled', true);
                button_delete_shortcut.prop('disabled', true);
                $('#button_padding').prop('disabled', true);
                $('#button_opacity').prop('disabled', true);
     
            }
            else
            {
                var btn_def = custom_keys.find(el=> ('ck'+el.id) == haptic_touch_selected.id);

                $('#button_reset_position').prop('disabled', 
                    btn_def.currentX !== undefined &&
                    btn_def.currentX==0 && btn_def.currentY==0);
     
                set_script_language(btn_def.lang);
                $('#input_button_text').val(btn_def.title);
                $('#input_button_shortcut').val(btn_def.key);
                
                let padding = btn_def.padding == undefined ? 'default':btn_def.padding ;
                $('#button_padding').text('btn size = '+ padding );
                let opacity = btn_def.opacity == undefined ? 'default':btn_def.opacity ;
                $('#button_opacity').text('btn opacity = '+ opacity);
                
                $('#check_app_scope').prop('checked',btn_def.app_scope);
                $('#input_action_script').val(btn_def.script);
                if(typeof(editor) !== 'undefined') editor.getDoc().setValue(btn_def.script);

                $('#button_delete_custom_button').show();
                
                button_delete_shortcut.prop('disabled',btn_def.key == "");
                $('#button_padding').prop('disabled', btn_def.title=='');
                $('#button_opacity').prop('disabled', btn_def.title=='');
                //show errors
                validate_action_script();
            }

            if(btn_def !== undefined && btn_def.transient !== undefined && btn_def.transient)
            {
                $('#check_app_scope_label').html(
                    '[ transient button from preconfig, globally visible ]'
                );
                $('#check_app_scope').prop("disabled",true);
            }
            else
            {
                $('#check_app_scope').prop("disabled",false);
                set_scope_label = function (){
                    $('#check_app_scope_label').html(
                        $('#check_app_scope').prop('checked') ? 
                        '[ currently visible only for '+global_apptitle+' ]' :
                        '[ currently globally visible ]'
                    );
                }
                set_scope_label();
                $('#check_app_scope').change( set_scope_label ); 
            }

            if(is_running())
            {
                wasm_halt();
            }

            //click function
            var on_add_action = function() {
                var txt= $(this).text();

                let doc = editor.getDoc();
                let cursor = doc.getCursor();
                doc.replaceRange(txt, cursor);
                editor.focus();
                validate_action_script();
            };

            $('#predefined_actions').collapse('hide');

            //Special Keys action
            var list_actions=['Space','Comma','F1','F3','F5','F8','leftAmiga','rightAmiga', 'AltLeft', 'AltRight','Delete','Enter','ArrowLeft','ArrowRight','ArrowUp','ArrowDown','ShiftLeft', 'ControlLeft', 'CapsLock'];
            var html_action_list='';
            list_actions.forEach(element => {
                html_action_list +='<a class="dropdown-item" href="#">'+element+'</a>';
            });
            $('#add_special_key').html(html_action_list);
            $('#add_special_key a').click(on_add_action);

            //joystick1 action
            var list_actions=['j1fire1','j1fire0','j1down1','j1down0','j1up1','j1up0','j1right1','j1right0','j1left1','j1left0'];
            var html_action_list='';
            list_actions.forEach(element => {
                html_action_list +='<a class="dropdown-item" href="#">'+element+'</a>';
            });
            $('#add_joystick1_action').html(html_action_list);
            $('#add_joystick1_action a').click(on_add_action);


            //joystick2 action
            var list_actions=['j2fire1','j2fire0','j2down1','j2down0','j2up1','j2up0','j2right1','j2right0','j2left1','j2left0'];
            var html_action_list='';
            list_actions.forEach(element => {
                html_action_list +='<a class="dropdown-item" href="#">'+element+'</a>';
            });
            $('#add_joystick2_action').html(html_action_list);
            $('#add_joystick2_action a').click(on_add_action);

            //timer action
            var list_actions=['100ms','300ms','1000ms', 'loop2{','loop3{','loop6{', '}','await_action_button_released'];
            html_action_list='';
            list_actions.forEach(element => {
                html_action_list +='<a class="dropdown-item" href="#">'+element+'</a>';
            });
            $('#add_timer_action').html(html_action_list);
            $('#add_timer_action a').click(on_add_action);
            
            //system action
            var list_actions=['toggle_run','toggle_warp','take_snapshot', 'restore_last_snapshot', 'swap_joystick', 'keyboard', 'fullscreen', 'menubar', 'pause', 'run', 'clipboard_paste', 'warp_always', 'warp_never', 'warp_auto', 'activity_monitor', 'toggle_action_buttons'];
            html_action_list='';
            list_actions.forEach(element => {
                html_action_list +='<a class="dropdown-item" href="#">'+element+'</a>';
            });
            $('#add_system_action').html(html_action_list);
            $('#add_system_action a').click(on_add_action);

            //script action
            var list_actions=['simple while', 'API example', 'aimbot', 'keyboard combos'];
            html_action_list='';
            list_actions.forEach(element => {
                html_action_list +='<a class="dropdown-item" href="#">'+element+'</a>';
            });
            $('#add_javascript').html(html_action_list);
            $('#add_javascript a').click(
                function() {
                    var txt= $(this).text();

                    var action_script_val = $('#input_action_script').val();
                    if(action_script_val.trim().length==0)
                    {
                        if(txt=='simple while')                
                            action_script_val = 
`
//do as longs as the actionbutton is not pressed again
while(not_stopped(this_id))
{
    //type 'A' and wait 200 milliseconds
    await action("A=>200ms");
}
`;
                        else if(txt=='API example')
                            action_script_val = '//example of the API\nwhile(not_stopped(this_id))\n{\n  //wait some time\n  await action("100ms");\n\n  //get information about the sprites 0..7\n  var y_light=sprite_ypos(0);\n  var y_dark=sprite_ypos(0);\n\n  //reserve exclusive port 1..2 access (manual joystick control is blocked)\n  set_port_owner(1,PORT_ACCESSOR.BOT);\n  await action(`j1left1=>j1up1=>400ms=>j1left0=>j1up0`);\n  //give control back to the user\n  set_port_owner(1,PORT_ACCESSOR.MANUAL);\n}';
                        else if(txt=='aimbot')
                            action_script_val = '//archon aimbot\nconst port_light=2, port_dark=1, sprite_light=4, sprite_dark=6;\n\nwhile(not_stopped(this_id))\n{\n  await aim_and_shoot( port_light /* change bot side here ;-) */ );\n  await action("100ms");\n}\n\nasync function aim_and_shoot(port)\n{ \n  var y_light=sprite_ypos(sprite_light);\n  var y_dark=sprite_ypos(sprite_dark);\n  var x_light=sprite_xpos(sprite_light);\n  var x_dark=sprite_xpos(sprite_dark);\n\n  var y_diff=Math.abs(y_light - y_dark);\n  var x_diff=Math.abs(x_light - x_dark);\n  var angle = shoot_angle(x_diff,y_diff);\n\n  var x_aim=null;\n  var y_aim=null;\n  if( y_diff<10 || 26<angle && angle<28 )\n  {\n     var x_rel = (port == port_dark) ? x_dark-x_light: x_light-x_dark;  \n     x_aim=x_rel < 0 ?"left":"right";   \n  }\n  if( x_diff <10 || 26<angle && angle<28)\n  {\n     var y_rel = (port == port_dark) ? y_dark-y_light: y_light-y_dark;  \n     y_aim=y_rel < 0 ?"up":"down";   \n  }\n  \n  if(x_aim != null || y_aim != null)\n  {\n    set_port_owner(port, \n      PORT_ACCESSOR.BOT);\n    await action(`j${port}left0=>j${port}up0`);\n\n    await action(`j${port}fire1`);\n    if(x_aim != null)\n     await action(`j${port}${x_aim}1`);\n    if(y_aim != null)\n      await action(`j${port}${y_aim}1`);\n    await action("60ms");\n    if(x_aim != null)\n      await action(`j${port}${x_aim}0`);\n    if(y_aim != null)\n      await action(`j${port}${y_aim}0`);\n    await action(`j${port}fire0`);\n    await action("60ms");\n\n    set_port_owner(\n      port,\n      PORT_ACCESSOR.MANUAL\n    );\n    await action("500ms");\n  }\n}\n\nfunction shoot_angle(x, y) {\n  return Math.atan2(y, x) * 180 / Math.PI;\n}';
                        else if(txt=='keyboard combos')
                            action_script_val =
`//example for key combinations
press_key('ControlLeft');
press_key('1');
release_key('1');
release_key('ControlLeft');`;
                        set_script_language('javascript');
                    }
                    else
                    {
                        alert('first empty manually the existing script code then try again to insert '+txt+' template')
                    }

                    editor.getDoc().setValue(action_script_val);
                    //$('#input_action_script').val(action_script_val);
                    editor.focus();
                    validate_action_script();
                }
            );
        };

        turn_on_full_editor = ()=>{
            require.config(
                {
                    packages: [{
                        name: "codemirror",
                        location: "js/cm",
                        main: "lib/codemirror"
                    }]
                });
                require(["codemirror", "codemirror/mode/javascript/javascript",
                            "codemirror/addon/hint/show-hint", "codemirror/addon/hint/javascript-hint",
                            "codemirror/addon/edit/closebrackets","codemirror/addon/edit/matchbrackets", 
                            "codemirror/addon/selection/active-line", "codemirror/addon/display/placeholder",
                            "codemirror/addon/lint/lint", "codemirror/addon/lint/javascript-lint",
      //                      "codemirror/lib/jshint", not working with require.js
                            ], function(CodeMirror) 
                {
                    editor = CodeMirror.fromTextArea(document.getElementById("input_action_script"), {
                        lineNumbers: true,
                        styleActiveLine: true,
                        mode:  {name: "javascript", globalVars: true},
                        lineWrapping: true,
                        autoCloseBrackets: true,
                        matchBrackets: true,
                        tabSize: 2,
                        gutters: ["CodeMirror-lint-markers"],
                        lint: { esversion: 10},
                        extraKeys: {"Ctrl-Space": "autocomplete"}
                    });

                    let check_livecomplete=$('#check_livecomplete'); 
                    editor.on("keydown",function( cm, event ) {
                        if(event.key === "Escape")
                        {//prevent that ESC closes the complete modal when in editor
                            event.stopPropagation();
                            return false;
                        }
                        if (!cm.state.completionActive &&
                            event.key !== undefined &&
                            event.key.length == 1  &&
                            event.metaKey == false && event.ctrlKey == false &&
                            event.key != ';' && event.key != ' ' && event.key != '(' 
                            && event.key != ')' && 
                            event.key != '{' && event.key != '}'
                            ) 
                        {
                            if(check_livecomplete.is(":visible") && 
                               check_livecomplete.prop('checked'))
                            {
                                cm.showHint({completeSingle: false});
                            }
                        }
                    });
                    editor.on("change", (cm) => {
                        cm.save();
                        validate_action_script();
                    });

                    if( (call_param_dark == null || call_param_dark)
                       && load_setting('dark_switch', true))
                    {
                        editor.setOption("theme", "vc64dark");
                    }
                    reconfig_editor($("#button_script_language").text());
                    $(".CodeMirror").css("width","100%").css("min-height","60px");
                    editor.setSize("100%", 'auto');

                    $("#button_script_add, #button_script_language").each(function(){
                        $(this).prop('disabled', false).
                        removeClass( "btn-secondary" ).
                        addClass("btn-primary");
                    });
                });
        }



        $('#modal_custom_key').on('shown.bs.modal', async function () 
        {
            if(typeof jshint_loaded != 'undefined')
            {
                turn_on_full_editor();
            }
            else
            {   
                $("#button_script_add, #button_script_language").each(function(){
                    $(this).prop('disabled', true).
                    removeClass( "btn-primary" ).
                    addClass("btn-secondary");
                });
                function load_css(url) {
                    var link = document.createElement("link");
                    link.type = "text/css";
                    link.rel = "stylesheet";
                    link.href = url;
                    document.getElementsByTagName("head")[0].appendChild(link);
                }
                load_css("css/cm/codemirror.css");
                load_css("css/cm/lint.css");
                load_css("css/cm/show-hint.css");
                load_css("css/cm/theme/vc64dark.css");

                //lazy load full editor now
                await load_script("js/cm/lib/jshint.js");
                jshint_loaded=true;  
                await load_script("js/cm/lib/require.js");
                turn_on_full_editor();
            }
        });



        $('#modal_custom_key').on('hidden.bs.modal', function () {
            if(typeof editor != 'undefined')
            {
                editor.toTextArea();
            }
            create_new_custom_key=false;
        
            if(is_running())
            {
                wasm_run();
            }

        });


        $('#input_button_text').keyup( function () {
            validate_custom_key(); 
            let empty=document.getElementById('input_button_text').value =='';
            $('#button_padding').prop('disabled', empty);
            $('#button_opacity').prop('disabled', empty);
            return true;
        } );
        $('#input_action_script').keyup( function () {validate_action_script(); return true;} );


        $('#button_reset_position').click(function(e) 
        {
            var btn_def = custom_keys.find(el=> ('ck'+el.id) == haptic_touch_selected.id);
            if(btn_def != null)
            {
                btn_def.currentX=0;
                btn_def.currentY=0;
                btn_def.position= "top:50%;left:50%";
                install_custom_keys();
                $('#button_reset_position').prop('disabled',true);
                save_custom_buttons(global_apptitle, custom_keys);
            }
        });

        $('#button_save_custom_button').click(async function(e) 
        {
            editor.save();
            if( (await validate_custom_key_form()) == false)
                return;

            let padding = $('#button_padding').text().split("=")[1].trim();
            let opacity = $('#button_opacity').text().split("=")[1].trim();
            if(create_new_custom_key)
            {
                //create a new custom key buttom  
                let new_button={  id: custom_keys.length
                      ,title: $('#input_button_text').val()
                      ,key: $('#input_button_shortcut').val()
                      ,app_scope: $('#check_app_scope').prop('checked')
                      ,script:  $('#input_action_script').val()
                      ,position: "top:50%;left:50%"
                      ,lang: $('#button_script_language').text()
                    };
                if(padding != 'default')
                {
                    new_button.padding=padding;
                }
                if(opacity != 'default')
                {
                    new_button.opacity=opacity;
                }
                custom_keys.push(new_button);

                movable_action_buttons=true;
                $('#movable_action_buttons_in_settings_switch').prop('checked', movable_action_buttons);
                $('#move_action_buttons_switch').prop('checked',movable_action_buttons);


                install_custom_keys();
                create_new_custom_key=false;
            }
            else
            {
                var btn_def = custom_keys.find(el=> ('ck'+el.id) == haptic_touch_selected.id);
                btn_def.title = $('#input_button_text').val();
                btn_def.key = $('#input_button_shortcut').val();
                btn_def.app_scope = $('#check_app_scope').prop('checked');
                btn_def.script = $('#input_action_script').val();
                btn_def.lang = $('#button_script_language').text();
                btn_def.padding=padding;
                if(padding == 'default')
                {
                    delete btn_def.padding;    
                }
                btn_def.opacity=opacity;
                if(opacity == 'default')
                {
                    delete btn_def.opacity;    
                }
                install_custom_keys();
            }
            $('#modal_custom_key').modal('hide');
            save_custom_buttons(global_apptitle, custom_keys);
        });

        $('#button_delete_custom_button').click(function(e) 
        {
            let id_to_delete =haptic_touch_selected.id.substring(2);

            get_running_script(id_to_delete).stop_request=true;

            custom_keys =custom_keys.filter(el=> +el.id != id_to_delete);            
            install_custom_keys();
            $('#modal_custom_key').modal('hide');
            save_custom_buttons(global_apptitle, custom_keys);
        });

        custom_keys = [];
        action_scripts= {};

        get_custom_buttons(global_apptitle, 
            function(the_buttons) {
                custom_keys = the_buttons;                
                for(let param_button of call_param_buttons)
                {
                    custom_keys.push(param_button);
                }
                install_custom_keys();
            }
        );
        //install_custom_keys();
    }

    $("#navbar").collapse('show')
    return;
}

//---- start custom keys ------
    function install_custom_keys(){
        //requesting to stop all scripts ... because the custom keys are going to be removed
        //and there is no way to stop their script by clicking the removed button again ...
        stop_all_scripts();

        //remove all existing custom key buttons
        $(".custom_key").remove();
        
        //insert the new buttons
        custom_keys.forEach(function (element, i) {
            element.id = element.transient !== undefined && element.transient ? element.id : i;

            if(element.transient && element.title == undefined ||
                element.title=="")
            {//don't render transient buttons if no title
                return;
            }

            var btn_html='<button id="ck'+element.id+'" class="btn btn-secondary btn-sm custom_key" style="position:absolute;'+element.position+';';
            if(element.currentX)
            {
                btn_html += 'transform:translate3d(' + element.currentX + 'px,' + element.currentY + 'px,0);';
            } 
            if(element.transient)
            {
                btn_html += 'border-width:3px;border-color: rgb(100, 133, 188);'; //cornflowerblue=#6495ED
            }
            else if(element.app_scope==false)
            {
                btn_html += 'border-width:3px;border-color: #99999999;';
            }
            if(element.padding != undefined && element.padding != 'default')
            {
                btn_html += 'padding:'+element.padding+'em;';
            }
            if(element.opacity != undefined && element.opacity != 'default')
            {
                btn_html += 'opacity:'+element.opacity+' !important;';
            }
            if(movable_action_buttons)
            {
                btn_html += 'box-shadow: 0.2em 0.2em 0.6em rgba(0, 0, 0, 0.9);';
            }

            btn_html += 'touch-action:none">'+html_encode(element.title)+'</button>';

            $('#div_canvas').append(btn_html);
            action_scripts["ck"+element.id] = element.script;

            let custom_key_el = document.getElementById(`ck${element.id}`);
            if(!movable_action_buttons)
            {//when action buttons locked
             //process the mouse/touch events immediatly, there is no need to guess the gesture
                let action_function = function(e) 
                {
                    e.stopImmediatePropagation();
                    e.preventDefault();
                    var action_script = action_scripts['ck'+element.id];

                    let running_script=get_running_script(element.id);                    
                    if(running_script.running == false)
                    {
                      running_script.action_button_released = false;
                    }
                    execute_script(element.id, element.lang, action_script);

                };
                let mark_as_released = function(e) 
                {
                    e.stopImmediatePropagation();
                    e.preventDefault();
                    get_running_script(element.id).action_button_released = true;
                };

                custom_key_el.addEventListener("pointerdown", (e)=>{
                    custom_key_el.setPointerCapture(e.pointerId);
                    action_function(e);
                },false);
                custom_key_el.addEventListener("pointerup", mark_as_released,false);
                custom_key_el.addEventListener("lostpointercapture", mark_as_released);
                custom_key_el.addEventListener("touchstart",(e)=>e.stopImmediatePropagation());

            }
            else
            {
                add_pencil_support(custom_key_el);
                custom_key_el.addEventListener("click",(e)=>
                {
                    e.stopImmediatePropagation();
                    e.preventDefault();
                    //at the end of a drag ignore the click
                    if(just_dragged)
                        return;
    
                    var action_script = action_scripts['ck'+element.id];
                    get_running_script(element.id).action_button_released = true;
                    execute_script(element.id, element.lang, action_script);
                });
            }


        });

        if(movable_action_buttons)
        {
            install_drag();
        }
        for(b of call_param_buttons)
        {   //start automatic run actions built from a call param
            if(b.run)
            {
                if(b.auto_started === undefined)
                {//only start one time
                    execute_script(b.id, b.lang, b.script);
                    b.auto_started = true;
                }

                if(get_running_script(b.id).running)
                {//if it still runs  
                    $('#ck'+b.id).css("background-color", "var(--red)");
                }
            }
        }
    }


    function install_drag()
    {
        dragItems = [];
        container = document;

        active = false;
        currentX=0;
        currentY=0;
        initialX=0;
        initialY=0;
    
        xOffset = { };
        yOffset = { };

        custom_keys.forEach(function (element, i) {
            dragItems.push(document.querySelector("#ck"+element.id));
            xOffset["ck"+element.id] = element.currentX;
            yOffset["ck"+element.id] = element.currentY;
        });

        container.addEventListener("pointerdown", dragStart, false);
        container.addEventListener("pointerup", dragEnd, false);
        container.addEventListener("pointermove", drag, false);
    }


    function dragStart(e) {
      if (dragItems.includes(e.target)) {  
        //console.log('drag start:' +e.target.id);  
        dragItem = e.target;
        active = true;
        haptic_active=false;
        timeStart = Date.now(); 

        if(xOffset[e.target.id] === undefined)
        {
            xOffset[e.target.id] = 0;
            yOffset[e.target.id] = 0;
        }
        currentX = xOffset[e.target.id];
        currentY = yOffset[e.target.id];
        startX = currentX;
        startY = currentY;        

        
        setTimeout(() => {
            checkForHapticTouch(e);
        }, 600);
        

        if (e.type === "touchstart") {
            initialX = e.touches[0].clientX - xOffset[e.target.id];
            initialY = e.touches[0].clientY - yOffset[e.target.id];
        } else {
            initialX = e.clientX - xOffset[e.target.id];
            initialY = e.clientY - yOffset[e.target.id];
        }

        just_dragged=false;
      }
    }



    function checkForHapticTouch(e)
    {
        if(active)
        {
            var dragTime = Date.now()-timeStart;
            if(!just_dragged && dragTime > 300)
            {
                haptic_active=true;
                haptic_touch_selected= e.target;
                $('#modal_custom_key').modal('show');
            }
        }
    }


    function dragEnd(e) {
      if (active) {
        //console.log('drag end:' +e.target.id);  
 
        if(!haptic_active)
        {
            checkForHapticTouch(e);
        }
        initialX = currentX;
        initialY = currentY;

        var ckdef = custom_keys.find(el => ('ck'+el.id) == dragItem.id); 
        
        if(ckdef.currentX === undefined)
        {
            ckdef.currentX = 0;
            ckdef.currentY = 0;
        }

        if(just_dragged)
        {
            ckdef.currentX = currentX;
            ckdef.currentY = currentY;
         
            //save new position
            save_custom_buttons(global_apptitle, custom_keys);
        }
        
        dragItem = null;
        active = false;
      }
    }

    function drag(e) {
      if (active && !haptic_active) {
        e.preventDefault();

        if(dragItems.includes(e.target) && e.target != dragItem)
          return; // custom key is dragged onto other custom key, don't allow that
 
       // console.log('drag:' +e.target.id);  

        if (e.type === "touchmove") {
          currentX = e.touches[0].clientX - initialX;
          currentY = e.touches[0].clientY - initialY;
        } else {
          currentX = e.clientX - initialX;
          currentY = e.clientY - initialY;
        }

        if(!just_dragged)
        {
            const magnetic_force=5.25;
            just_dragged = Math.abs(xOffset[e.target.id]-currentX)>magnetic_force || Math.abs(yOffset[e.target.id]-currentY)>magnetic_force;
        }
        if(just_dragged)
        { 
            xOffset[e.target.id] = currentX;
            yOffset[e.target.id] = currentY;

            setTranslate(currentX, currentY, dragItem);
        }
      }
    }

    function setTranslate(xPos, yPos, el) {
     //   console.log('translate: x'+xPos+' y'+yPos+ 'el=' +el.id);  
      el.style.transform = "translate3d(" + xPos + "px, " + yPos + "px, 0)";
    }

//---- end custom key ----

function loadTheme() {
    var dark_theme_selected = load_setting('dark_switch', true);;
    get_parameter_link();
    if(call_param_dark!=null)
    {
        dark_theme_selected= call_param_dark;
    }
    dark_switch.checked = dark_theme_selected;
    dark_theme_selected ? document.body.setAttribute('data-theme', 'dark') :
        document.body.removeAttribute('data-theme');
}

function setTheme() {
  if (dark_switch.checked) {
    document.body.setAttribute('data-theme', 'dark');
    save_setting('dark_switch', true);
  } else {
    document.body.removeAttribute('data-theme');
    save_setting('dark_switch', false);
  }
}
  




    function register_v_joystick()
    {
        if(v_joystick!=null)
            return;
        v_joystick	= new VirtualJoystick({
            container	: document.getElementById('div_canvas'),
            mouseSupport	: true,
            strokeStyle	: 'white',
            stickRadius	: 118,
            limitStickTravel: true,
            stationaryBase: stationaryBase

        });
        v_joystick.addEventListener('touchStartValidation', function(event){
            var touches = event.changedTouches;
            var touch =null;
            if(touches !== undefined)
                touch= touches[0];
            else
                touch = event;//mouse emulation    
            return touch.pageX < window.innerWidth/2;  
        });
       
        // one on the right of the screen
        v_fire	= new VirtualJoystick({
            container	: document.getElementById('div_canvas'),
            strokeStyle	: 'red',
            limitStickTravel: true,
            stickRadius	: 0,
            mouseSupport	: true
        });
        v_fire.addEventListener('touchStartValidation', function(event){
            var touches = event.changedTouches;
            var touch =null;
            if(touches !== undefined)
                touch= touches[0];
            else
                touch = event;//mouse emulation    
            return touch.pageX >= window.innerWidth/2;
        });
    }

    function unregister_v_joystick()
    {   
        if(v_joystick != null)
        {
            v_joystick.destroy();
            v_joystick=null;
        }
        if(v_fire != null)
        {
            v_fire.destroy();
            v_fire=null;
        }
    }


    function is_running()
    {
        return running;
        //return $('#button_run').attr('disabled')=='disabled';
    }
        
    

async function emit_string_autotype(keys_to_emit_array, type_first_key_time=0, release_delay_in_ms=100)
{  
    var delay = type_first_key_time;
    var release_delay = release_delay_in_ms;
    if(release_delay<50)
    {
        release_delay = 50;
    }
    for(the_key of keys_to_emit_array)
    {
        var key_code = translateSymbol(the_key);
        if(key_code !== undefined)
        {
            if(key_code.modifier != null)
            {
                wasm_auto_type(key_code.modifier[0], release_delay, delay);
            }
            wasm_auto_type(key_code.raw_key[0], release_delay, delay);
            delay+=release_delay;
        }
    }
}

async function emit_string(keys_to_emit_array, type_first_key_time=0, release_delay_in_ms=100)
{  
    if(type_first_key_time>0) await sleep(type_first_key_time);
    for(the_key of keys_to_emit_array)
    {
        var key_code = translateSymbol(the_key);
        if(key_code !== undefined)
        {
            if(key_code.modifier != null)
            {
                wasm_key(key_code.modifier[0], 1);
            }
            wasm_key(key_code.raw_key[0], 1);    
            await sleep(release_delay_in_ms);     
            if(key_code.modifier != null)
            {
                wasm_key(key_code.modifier[0], 0);
            }
            wasm_key(key_code.raw_key[0],0);                
        }
    }
}

function hide_all_tooltips()
{
    //close all open tooltips
    $('[data-toggle="tooltip"]').tooltip('hide');
}
    
add_pencil_support = (element) => {
    let isPointerDown = false;
    let pointerId = null;

    element.addEventListener('pointerdown', (event) => {
        if (event.pointerType === 'pen') {
            isPointerDown = true;
            pointerId = event.pointerId;
        }
    });

    element.addEventListener('pointerup', (event) => {
        if (isPointerDown && event.pointerId === pointerId) {
            isPointerDown = false;
            pointerId = null;

            const clickEvent = new MouseEvent('click', {
                bubbles: true,
                cancelable: true,
            });

            element.focus();
            element.dispatchEvent(clickEvent);      
        }
    });

    element.addEventListener('pointercancel', (event) => {
        if (event.pointerType === 'pen') {
            isPointerDown = false;
            pointerId = null;
        }
    });
}
function add_pencil_support_to_childs(element) {
    element.childNodes.forEach(child => {
        if (child.nodeType === Node.ELEMENT_NODE)
          add_pencil_support(child);
    });  
}
function add_pencil_support_for_elements_which_need_it()
{
    let elements_which_need_pencil_support=
        ["button_show_menu","button_run", "button_reset", "button_take_snapshot",
        "button_snapshots", "button_keyboard", "button_custom_key", "drop_zone",
        "button_fullscreen", "button_settings", "port1", "port2" ]
    for(let element_id of elements_which_need_pencil_support)
    {
        add_pencil_support(document.getElementById(element_id));
    }
}

function copy_to_clipboard(element) {
    var textToCopy = element.innerText;
    navigator.clipboard.writeText(textToCopy).then(function() {
        alert(`copied to clipboard: ${textToCopy}`);
    }, function(err) {
        console.error(err);
    });
}

//--- activity monitors and dma bus visualisation
let activity_intervall=null;

function add_monitor(id, label, splitted=false)
{
    $("#activity").append(
    `
        <div>
            <div id="monitor_${id}" class="monitor"></div>
            <div class="monitor_label">${label}</div>
        <div>
    `
    );

    color=[];
    color.copper={start: '51,51,0', end:'255,255,0'}
    color.blitter={start: `50,${parseInt('cc',16)*0.2},0`, end:`${parseInt('ff',16)},${parseInt('cc',16)},0`}
    color.disk={start: `0,51,0`, end:`0,255,0`}
    color.audio={start: '50,0,50', end:'255,0,255'}
    color.sprite={start: `0,${parseInt('88',16)*0.2},51`, end:`0,${parseInt('88',16)},255`}
    color.bitplane={start: '0,50,50', end:'0,255,255'}
    color.CPU={start: '50,50,50', end:'255,255,255'}


    document.querySelector(`#monitor_${id}`).addEventListener('click', 
        (e)=>{
            let id=e.currentTarget.id.replace('monitor_','');
            if(id.includes("Ram") || id.includes("Rom"))
                id="CPU";
            
            if(dma_channels[id] !==true )
            {
                e.currentTarget.style.setProperty("--color_start",color[id].start);
                e.currentTarget.style.setProperty("--color_end",color[id].end);   
            }
            else
            {
                e.currentTarget.style.setProperty("--color_start",'50,50,50');
                e.currentTarget.style.setProperty("--color_end",'200,200,200');   
            }
            dma_debug(id);
        }
    );
    dma_channel_history[id] = [];
    dma_channel_history_values[id] = [];
    for(let i=0;i<20;i++)
    {
        if(splitted==false)
        {
            $(`#monitor_${id}`).append(
                `<div id="${id}_bar_${i}" class="bar" 
                  style="--barval:0;grid-column:${i+1};"></div>`
            );
            dma_channel_history[id].push(document.querySelector(`#${id}_bar_${i}`));
            dma_channel_history_values[id].push(0);
        }
        else
        {
            $(`#monitor_${id}`).append(
                `<div id="${id}_bar_${i}_upper" class="bar_splitted_upper" 
                  style="--barval:0;grid-column:${i+1};"></div>`
            );
            $(`#monitor_${id}`).append(
                `<div id="${id}_bar_${i}_lower" class="bar_splitted_lower" 
                  style="--barval:0;grid-column:${i+1};"></div>`
            );
            dma_channel_history[id].push(
                [
                    document.querySelector(`#${id}_bar_${i}_upper`),
                    document.querySelector(`#${id}_bar_${i}_lower`)
                ]
            );
            dma_channel_history_values[id].push([0,0]);
        }
        
    }

    const activity_id = {
        copper: 0,
        blitter: 1,
        disk: 2,
        audio: 3,
        sprite: 4,
        bitplane: 5,
        chipRam: 6,
        slowRam: 7,
        fastRam: 8,
        kickRom: 9,
        waveformL:10, waveformR:11
    }

    if(!activity_intervall)
    {
        activity_intervall = setInterval(()=>{
            if(!running) return;
            
            for(id in dma_channels){
                if(dma_channel_history[id]===undefined)
                    continue;
                let value=_wasm_activity(activity_id[id]);
                value = (Math.log(1+19*value) / Math.log(20)) * 100;
                value = value>100 ? 100: Math.round(value);

                if(Array.isArray(dma_channel_history[id][0]))
                {
                    for(let i=0;i<20-1;i++)
                    {
                        let newer_upper_value=dma_channel_history_values[id][i+1][0];
                        if(dma_channel_history_values[id][i][0] !==newer_upper_value)
                        {
                            dma_channel_history[id][i][0].style.setProperty("--barval", newer_upper_value);
                            dma_channel_history_values[id][i][0]=newer_upper_value;
                        }

                        let newer_lower_value=dma_channel_history_values[id][i+1][1];
                        if(dma_channel_history_values[id][i][1] !==newer_lower_value)
                        {
                            dma_channel_history[id][i][1].style.setProperty("--barval", newer_lower_value);
                            dma_channel_history_values[id][i][1]=newer_lower_value;
                        }
                    }

                    if(value !== dma_channel_history_values[id][20-1][0])
                    {    
                        dma_channel_history_values[id][20-1][0]= value;           
                        dma_channel_history[id][20-1][0].style.setProperty("--barval", value);
                    }

                    value=_wasm_activity(activity_id[id],1);
                    value = (Math.log(1+19*value) / Math.log(20)) * 100;
                    value = value>100 ? 100: Math.round(value);    
                    if(value !== dma_channel_history_values[id][20-1][1])
                    {         
                        dma_channel_history_values[id][20-1][1]= value;  
                        dma_channel_history[id][20-1][1].style.setProperty("--barval", value);
                    }
                }
                else
                {
                    for(let i=0;i<20-1;i++)
                    {
                        let newer_value = dma_channel_history_values[id][i+1];
                        if(dma_channel_history_values[id][i] !== newer_value)
                        {
                            dma_channel_history[id][i].style.setProperty("--barval", newer_value);
                            dma_channel_history_values[id][i]=newer_value;
                        }
                    }
                    if(value !== dma_channel_history_values[id][20-1])
                    {
                        dma_channel_history_values[id][20-1]= value;
                        dma_channel_history[id][20-1].style.setProperty("--barval", value);
                    }
                }
            }
        },400);
    }
}


function show_activity()
{
    $("#activity_help").show();

    wasm_configure_key("DMA_DEBUG_CHANNEL", "COPPER", "0");
    wasm_configure_key("DMA_DEBUG_CHANNEL", "BLITTER", "0");
    wasm_configure_key("DMA_DEBUG_CHANNEL", "DISK", "0");
    wasm_configure_key("DMA_DEBUG_CHANNEL", "AUDIO", "0");
    wasm_configure_key("DMA_DEBUG_CHANNEL", "SPRITE", "0");
    wasm_configure_key("DMA_DEBUG_CHANNEL", "BITPLANE", "0");
    wasm_configure_key("DMA_DEBUG_CHANNEL", "CPU", "0");
    wasm_configure_key("DMA_DEBUG_CHANNEL", "REFRESH", "0");


    dma_channels={
        copper: false,
        blitter: false,
        disk:false,
        audio:false,
        sprite:false,
        bitplane:false,
        chipRam:false,
        slowRam:false,
        fastRam:false,
        kickRom:false,

    };


    wasm_configure("DMA_DEBUG_ENABLE","1");

    $("#activity").remove();
    $("body").append(`<div id="activity" class="monitor_grid"></div>`);

    dma_channel_history = [];
    dma_channel_history_values = [];

    add_monitor("blitter", "Blitter DMA");
    add_monitor("copper", "Copper DMA");
    add_monitor("disk", "Disk DMA");
    add_monitor("audio", "Audio DMA");
    add_monitor("sprite", "Sprite DMA");
    add_monitor("bitplane", "Bitplane DMA");
    add_monitor("chipRam", "CPU (chipRAM)", true);
    add_monitor("slowRam", "CPU (slowRAM)", true);
    add_monitor("fastRam", "CPU (fastRAM)", true);
    add_monitor("kickRom", "CPU (kickROM)", true);

 }
function hide_activity()
{
    wasm_configure("DMA_DEBUG_ENABLE","0");

    $("#activity").remove();
    clearInterval(activity_intervall);
    activity_intervall=null;
    $("#activity_help").hide();
}

function dma_debug(channel)
{
/**
 * activity monitor on => all aktivity monitorings are enabled 
 * they will be rendered black and white
 * on touch werden they become colored and the dma bus activity will be visualized (dma debugger)
 * again touch and they become black white  and dma channel visualisation on that particular channel will be disabled
 
Settings
 (x) activity monitor
 (x?) tap on monitor to visualise dma channel
*/
    if(channel.includes("Ram") || channel.includes("Rom"))
        channel="CPU";


    if(dma_channels[channel]=== undefined)
    {
        dma_channels[channel]=false;
    }

    dma_channels[channel]=!dma_channels[channel];
   
    wasm_configure_key("DMA_DEBUG_CHANNEL", channel, dma_channels[channel] ? "1" : "0");
}