const sleep = (milliseconds) => {
  return new Promise(resolve => setTimeout(resolve, milliseconds));
}

var map_of_running_scripts = [];
function get_running_script(id)
{
    let running_script=map_of_running_scripts[id];
    if(running_script == null)
    {
        running_script = {
            id: id,
            running: false,
            stop_request: false,
            action_button_released: false
         };
        map_of_running_scripts[id]=running_script;
    }
    return running_script;
}

var execute_script = function(id, script_lang, action_script) {    
    let running_script=get_running_script(id);
    if(running_script.running)
    {
        //alert("action script with "+id+" is still running, double execution is prevented... requesting stop");
        running_script.stop_request=true;
    }
    else
    {
        running_script.running=true;
        running_script.stop_request=false;

        setTimeout(async function() { 
            let button_element=$('#ck'+id);
            button_element.css("background-color", "var(--red)");
            try{
              await parse_script(script_lang, action_script, true, id);
            }
            finally
            {
              if(button_element.length >0 && button_element[0].parentElement == null)
              {//if the button_element is decoupled because button was redrawn as a new DOM instance
                  button_element=$('#ck'+id); //then get the new dom element
              }
              button_element.css("background-color", "");
              
              let running_script=get_running_script(id);
              running_script.running = false;
              running_script.stop_request=false;
            }
        });
    }
}

function stop_all_scripts()
{
    for(id in map_of_running_scripts)
    {
        let action_button = custom_keys.find(el => el.id == id); 
        if(action_button === undefined ||
           action_button.auto_started !== undefined) 
        {//skip auto started scripts
            continue;
        }

        let running_script=get_running_script(id);
        running_script.stop_request=true;
    }
}

function not_stopped(id)
{
    return id<0 ? true : get_running_script(id).stop_request == false;
}

async function parse_script(script_lang, action_script, execute = false, execution_id = -1) {
    action_script = action_script.trim();
    if(action_script.length==0)
    {
        $('#action_button_syntax_error').html('you have to enter at least one action ...');
        return false;
    }
    if(script_lang == 'javascript')
    {
        var valid = true;
        var js_script=action_script;
        var js_script_function;
        let AsyncFunction = Object.getPrototypeOf(async function(){}).constructor
        try {            
            js_script_function=new AsyncFunction(`let this_id=${execution_id};let still_active=function(){return not_stopped(this_id);};${js_script}`);
        } catch (error) {
            valid=false;
            if(execute==false)
            {
                $('#action_button_syntax_error').html(error.message);
            }
        }
        if(execute)
        {
            await js_script_function();
        }
    }
    else
    {
       valid = await execute_action_sequence_script(action_script, execute, execution_id);
    }
    return valid;
};

async function action(cmd, execute=true)
{
    await execute_action_sequence_script(cmd, execute);
}


async function execute_action_sequence_script(action_script, execute=false, execution_id=-1)
{
    action_script=action_script.replace(/[{]/g,'{=>')
    action_script=action_script.replace(/[}]/g,'=>}')
    var cmd_sequence = action_script.split('=>');
    var valid = true;
    var error_message = "";

    var pc=0;
    var pc_loop_begin=[];
    var lc=[];
    var loop_depth=0;

    while (pc < cmd_sequence.length && not_stopped(execution_id)) {
        var cmd = cmd_sequence[pc].trim();
        pc++;

        if(cmd.match(/^loop[0-9]+[{]$/) != null)
        {
            loop_depth++;
            lc[loop_depth]=execute ? parseInt(cmd.match(/[0-9]+/)) : 1;
            pc_loop_begin[loop_depth]=pc;
        }
        else if(cmd.length == 1 && cmd =='}')
        {
            lc[loop_depth]--;
            if(lc[loop_depth]>0)
            {
                pc=pc_loop_begin[loop_depth];
            }
            else
            {
                loop_depth--;
                if(loop_depth<0)
                {
                    error_message="too many closing loop bracket at pc="+pc;
                    valid=false;
                    break;
                }
            }
        }
        else if(await execute_single_action(cmd, execute, execution_id)==true)
        {            
        }
        else
        {
            error_message='unknown command "'+cmd+'" at pc=' +pc;
            valid = false;
            break;
        }
    }

    if(loop_depth>0)
    {
        error_message="missing closing loop "+pc_loop_begin+" brackets";
        valid=false;
    }

    if(execute == false)
    {
        $('#action_button_syntax_error').html(error_message);
    }
    return valid;
}


async function execute_single_action(cmd, execute=true, execution_id=-1)
{
    cmd=cmd.trim();
    var valid = true;
    var joy_cmd_tokens=null;
    if(cmd.match(/^'.+?'$/sm) != null)
    {
        if(execute)
        {
            var chars = cmd.substring(1,cmd.length-1).split("");
            var time_to_emit_next_char = 100;

            //blocking execution of action script and wait for all keys emitted
            await emit_string(chars,0,time_to_emit_next_char);
        }
    }
    else if(cmd == 'pause')
    {
        if(execute)
        {
            if(is_running())
            {
                $('#button_run').click();   
            } 
        }
    
    }
    else if(cmd == 'run')
    {
        if(execute)
        {
            if(!is_running())
            {
                $('#button_run').click();   
            } 
        }
    }
    else if(cmd == 'toggle_run')
    {
        if(execute)
        {
            $('#button_run').click();   
        }
    }
    else if(cmd == 'toggle_warp')
    {
        if(execute)
        {//0==auto, 1==never, 2==always
            let mode = wasm_get_config_item("WARP_MODE");
            if(mode <= 1) auto_or_never_warp_mode = mode ==0 ?"WARP_AUTO":"WARP_NEVER";
            wasm_configure('WARP_MODE', mode<=1 ? 'WARP_ALWAYS': auto_or_never_warp_mode);
        }
    }
    else if(cmd == 'warp_always')
    {
        if(execute)
        {
            wasm_configure('WARP_MODE', 'WARP_ALWAYS');
        }
    }
    else if(cmd == 'warp_never')
    {
        if(execute)
        {
            wasm_configure('WARP_MODE', 'WARP_NEVER');
        }
    }
    else if(cmd == 'warp_auto')
    {
        if(execute)
        {
            wasm_configure('WARP_MODE', 'WARP_AUTO');
        }
    }
    else if(cmd == 'toggle_speed')
    {
        if(execute)
        {
            $('#button_speed_toggle').click();
        }
    }    
    else if(cmd.match(/^[0-9]+ms$/) != null)
    {
        if(execute)
        {
            await sleep(parseInt(cmd.match(/[0-9]+/)));                 
        }
    }
    else if(cmd == 'take_snapshot')
    {
        if(execute)
        {
            $('#button_take_snapshot').click();
        }
    }
    else if(cmd == 'keyboard')
    {
        if(execute)
        {
            $('#button_keyboard').click();
        }
    }
    else if(cmd == 'menubar')
    {
        if(execute)
        {
            $("#button_show_menu").click();
        }
    }
    else if(cmd == 'clipboard_paste')
    {
        if(execute)
        {
            let cliptext = await navigator.clipboard.readText();
        action(`'${cliptext/*.toLowerCase()*/}'`);
        }
    }
    else if(cmd == 'fullscreen')
    {
        if(execute)
        {
            $('#button_fullscreen').click();
        }
    }
    else if(cmd == 'restore_last_snapshot')
    {
        if(execute)
        {
            await load_last_snapshot();
        }
    }
    else if(cmd == 'swap_joystick')
    {
        if(execute)
        {
            var port1_value=port1;
            port1=port2;
            port2=port1_value;
            port1_value= $('#port1').val();
            $('#port1').val($('#port2').val());
            $('#port2').val(port1_value);
        }
    }
    else if(
        (
            joy_cmd_tokens=cmd.match(/^j([12])(fire|up|down|right|left)([01])$/)
        )
        != null
    )
    {
        if(execute)
        {
            execute_joystick_script(joy_cmd_tokens);
        }
    }
    else if(translateSymbol(cmd) !== undefined)
    {
        if(execute)
        {            
            emit_string([cmd],0,100); 
        }
    }
    else if(
        cmd.startsWith("press")
        &&
        translateKey2(cmd.replace("press",""),cmd.replace("press","")) !== undefined        
    )
    {
        if(execute)
        {            
            press_key(cmd.replace("press","")); 
        }
    }
    else if(
        cmd.startsWith("release")
        &&
        translateKey2(cmd.replace("release",""),cmd.replace("release","")) !== undefined        
    )
    {
        if(execute)
        {            
            release_key(cmd.replace("release","")); 
        }
    }
    else if(cmd == "await_action_button_released")
    {
        if(execute)
        { 
            await action_button_released(execution_id);
        }
    }
    else if(cmd == 'activity_monitor')
    {
        if(execute)
        {
            let activity_monitor_visible=document.querySelector("#activity")
            if(activity_monitor_visible)
                hide_activity();
            else
                show_activity();

            activity_monitor_switch.prop('checked', !activity_monitor_visible);
        }
    }
    else if(cmd == 'toggle_action_buttons')
    {
        if(execute)
        {
            let this_buttons_label = $('#ck'+execution_id).text();
            $(".custom_key").not(`":contains('${this_buttons_label}')"`).toggle();
        }
    }    
    else
    {
        valid=false;
    }
    return valid;
}



async function validate_custom_key_form(){
    var is_valid=true;
    is_valid &=await validate_custom_key();
    is_valid &=await validate_action_script();
    return is_valid;
}

async function reset_validate(){
    document.querySelectorAll(".is-invalid").forEach(el=>el.classList.remove("is-invalid"))
    document.querySelectorAll(".is-valid").forEach(el=>el.classList.remove("is-valid"))
}

async function validate_custom_key(){
    var is_valid=true;
    var input_button_text=$('#input_button_text');
    var input_button_shortcut=$('#input_button_shortcut');

    input_button_text.removeClass("is-valid").removeClass("is-invalid");
    input_button_shortcut.removeClass("is-valid").removeClass("is-invalid");

    if( input_button_text.val().trim().length==0 && 
        input_button_shortcut.val().trim().length==0)
    {
        input_button_text.addClass("is-invalid");
        input_button_shortcut.addClass("is-invalid");
        is_valid=false;
    }
    else
    {  
        input_button_text.addClass("is-valid");
        input_button_shortcut.addClass("is-valid");
    }
  
    return is_valid;
}

async function validate_action_script()
{
    let is_valid=true;
    let input_action_script=$('#input_action_script');
    let script_lang = $('#button_script_language').text();
    input_action_script.removeClass("is-valid").removeClass("is-invalid");
    if( (await parse_script(script_lang, input_action_script.val())) == false)
    {
        input_action_script.addClass("is-invalid");
        is_valid=false;
    }
    else
    {
        input_action_script.addClass("is-valid");
    }
    return is_valid;
};



function execute_joystick_script(cmd_tokens)
{
    var portnr=cmd_tokens[1];
    var dir= cmd_tokens[2].toUpperCase();
    var down_or_release = cmd_tokens[3];

    var previous_owner = set_port_owner(portnr,PORT_ACCESSOR.BOT);
    var native_joy_cmd=null;
    if(dir == "FIRE")
    {
        native_joy_cmd= portnr+(down_or_release == 1 ?"PRESS_"+dir:"RELEASE_"+dir);
    }
    else
    {
       native_joy_cmd= portnr+(down_or_release == 1 ?"PULL_"+dir:"RELEASE_"+((dir=="LEFT" || dir=="RIGHT")?"X":"Y"));
    }
    send_joystick(PORT_ACCESSOR.BOT,portnr,native_joy_cmd);

    set_port_owner(portnr,previous_owner);
 }

 async function load_last_snapshot()
 {
    let app_snaps = await get_snapshots_for_app_title(global_apptitle);  
    if(app_snaps.length>0)
    {
        var snapshot = app_snaps[app_snaps.length-1];
        if(is_running())
        {
            wasm_halt();
        }
        wasm_loadfile(
            snapshot.title+".vAmiga",
            snapshot.data);
        if(!is_running())
        {
            $("#button_run").click();
        }  
        {
            wasm_run();
        } 
    }
 }

 function sprite_xpos(sprite_id)
 {
   var all_sprite_pos = wasm_sprite_info().split(",");
   return all_sprite_pos[sprite_id*2];
 }

 function sprite_ypos(sprite_id)
 {
   var all_sprite_pos = wasm_sprite_info().split(",");
   return all_sprite_pos[sprite_id*2+1];
 }


class BotEvent {
    log() {
        console.log(`BotEvent { other_sprite=${this.other_sprite}, x_dir=${this.x_dir}, y_dir=${this.y_dir}, distance=${this.distance}}`)
    }
}

class Bot {
    constructor(proccess_id, sprite_id, min_diagonal_range_angle=26, max_diagonal_range_angle=28) {
        this.sprite_id = sprite_id;
        this.process_id = proccess_id;
        this.min_diagonal_range_angle=min_diagonal_range_angle; 
        this.max_diagonal_range_angle=max_diagonal_range_angle; 
        this.process_event=false;
    }
    fetch_sprite_info(){
        this.all_sprite_pos = wasm_sprite_info().split(",");
    }


    get _x() {
        return this.x(this.sprite_id);
    }
    get _y() {
        return this.y(this.sprite_id);
    }

    get port() {
        this.controller_port;
    }

    next_event(bot_event){
        console.log(` event=${bot_event.other_sprite} ${bot_event.x_dir} ${bot_event.y_dir} \n`);
    }

    distance(other_sprite) {
        const dx = this._x - this.x(other_sprite);
        const dy = this._y - this.y(other_sprite);
        return Math.hypot(dx, dy);
    }
    
    x(sprite){ return this.all_sprite_pos[sprite*2]};
    y(sprite){ return this.all_sprite_pos[sprite*2+1]};

    async aim_and_shoot(bot_event) {
        var x_aim = bot_event.x_dir;
        var y_aim = bot_event.y_dir;
        
        if(x_aim != null || y_aim != null)
        {
          set_port_owner(port, 
            PORT_ACCESSOR.BOT);
          await action(`j${port}left0=>j${port}up0`);
      
          await action(`j${port}fire1`);
          if(x_aim != null)
           await action(`j${port}${x_aim}1`);
          if(y_aim != null)
            await action(`j${port}${y_aim}1`);
          await action("60ms");
          if(x_aim != null)
            await action(`j${port}${x_aim}0`);
          if(y_aim != null)
            await action(`j${port}${y_aim}0`);
          await action(`j${port}fire0`);
          await action("60ms");
      
          set_port_owner(
            port,
            PORT_ACCESSOR.MANUAL
          );
        }

    }

    move(direction, time) { 

    }

    sprite_in_shooting_range(other_sprite)
    {
        var y_light=this.y(this.sprite_id);
        var y_dark=this.y(other_sprite);
        var x_light=this.x(this.sprite_id);
        var x_dark=this.x(other_sprite);
      
        var y_diff=Math.abs(y_light - y_dark);
        var x_diff=Math.abs(x_light - x_dark);
        var angle =  Math.atan2(y_diff, x_diff) * 180 / Math.PI; 
    
        var x_aim=null;
        var y_aim=null;
        if( y_diff<10 || this.min_diagonal_range_angle<angle && angle<this.max_diagonal_range_angle )
        {
            var x_rel = x_light-x_dark;  
            x_aim=x_rel > 0 ?'left':'right';   
        }
        if( x_diff <10 || this.min_diagonal_range_angle<angle && angle<this.max_diagonal_range_angle)
        {
            var y_rel = y_light-y_dark;  
            y_aim=y_rel > 0 ?'up':'down';   
        }
        if(x_aim != null || y_aim != null)
        {
            var event = new BotEvent();
            event.other_sprite = other_sprite;
            event.x_dir = x_aim;
            event.y_dir = y_aim;
            event.distance = this.distance(other_sprite);
            return event; 
        }
        else
        {
            return null;
        }
    }

    async run(controller_port=1) {
        this.controller_port=controller_port;

        while(not_stopped(this.process_id))
        {
          //check for events
          this.fetch_sprite_info();
          for(var id = 0; id<8; id++)
          {
            if(id != this.sprite_id)
            {
                var sprite_event = await this.sprite_in_shooting_range(id);
                if(sprite_event != null)
                {
                    this.next_event(sprite_event);
                }
            }
          }
          await action("20ms"); // time of one frame 1000/50=20ms
        }
    }
}


class ArchonBot extends Bot {
    async next_event(event) {
        if( event.other_sprite == 1 )
        {
            this.aim_and_shoot(event);
        }
    }
}

class FalconBot extends Bot {
    constructor(proccess_id, sprite_id) {
        super(proccess_id, sprite_id);
        this.min_diagonal_range_angle = 30;
        this.max_diagonal_range_angle = 35;
    }

    async next_event(event) {
        if( 0<event.other_sprite  )
        {
            event.log();

            if(this._y > 100)
            {//prevent crash on ground
                await action("j1up1=>300ms=>j1up0");
            }
            else if(event.y_dir == 'up' )
            {
            }
            else if(event.x_dir == 'left' //|| event.x_dir == 'right'  
               && event.distance < 150 /*&& this.process_event == false */
            )
            {
                this.process_event = true; 
                //await this.aim_and_shoot(event);
                if(event.y_dir == 'down')
                {
                    await action("j1down1");
                }
    
                await action("j1fire1=>100ms=>j1fire0")

                if(event.y_dir == 'down')
                {
                    await action("j1down0=>10ms=>j1up1=>60ms=>j1up0");
                }
                console.log("shoot");
              
                this.process_event = false;
            }
            else
            {
                console.log("no shoot");
            }
        }
    }
}

function wasm_runstop_restore()
{
    wasm_schedule_key(7, 7, 1, 0); //press runstop 
    wasm_schedule_key(9, 9, 1, 2); //press restore 
    wasm_schedule_key(9, 9, 0, 4); //release restore
    wasm_schedule_key(7, 7, 0, 6); //release runstop
}


function press_key(key)
{
    let c64code=translateSymbol(key);
    if(c64code !== undefined )
    {
        wasm_key(c64code.raw_key[0], 1);
        if(c64code.modifier != null)
        {
            wasm_key(c64code.modifier[0], 1);
        }
    }
}
function release_key(key)
{
    let c64code=translateSymbol(key);
    if(c64code !== undefined )
    {
        wasm_key(c64code.raw_key[0], 0);
        if(c64code.modifier != null)
        {
            wasm_key(c64code.modifier[0], 0);
        }
    }
}

async function wasm_ready_after_reset()
{
    let mega_roms=JSON.parse(wasm_rom_info()).kernal.startsWith("mega");
    //mega roms is ready after 500000 cycles, original C= Rom is ready after 2700000 cycles
    while(wasm_get_cpu_cycles()<(mega_roms?500000:2700000)) 
    {
        await sleep(50);
    }    
}

async function action_button_released(id)
{
    while(!get_running_script(id).action_button_released) 
    {
        await sleep(50);
    }    
}

//the following line is only there for codemirrors jshint autocomplete feature
//this_id and still_active will be overscoped by their equal named pendants in the action blocks 
var this_id=0; var still_active=function(){alert("NEVER CALLED!!")}