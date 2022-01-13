/**
 * official vAmigaWeb player.
 * checks whether jquery is already there, if not lazy loads it when emulator is started
 */

 var vAmigaWeb_player={
    vAmigaWeb_url: 'https://vamigaweb.github.io/',
    listens: false,
    loadScript: async function (url, callback){
        var script = document.createElement("script")
        script.type = "text/javascript";
        script.onload = callback;
        script.src = url;
        document.getElementsByTagName("head")[0].appendChild(script);
    },
    samesite_file: null,
    inject_samesite_app_into_iframe: async function (){
        let ssfile = this.samesite_file;
        this.samesite_file= null;
        let vAmigaWeb_window = document.getElementById("vAmigaWeb").contentWindow;
        
        function FromBase64(str) {
                return atob(str).split('').map(function (c) { return c.charCodeAt(0); });
        }
        let file_descriptor={
                cmd: "load"
        }
        if(ssfile.floppy_rom_base64 !== undefined)
        {
            file_descriptor.floppy_rom = Uint8Array.from(FromBase64(ssfile.floppy_rom_base64));
        }
        if(ssfile.kernal_rom_base64 !== undefined)
        {
            file_descriptor.kernal_rom = Uint8Array.from(FromBase64(ssfile.kernal_rom_base64));
        }
        if(ssfile.basic_rom_base64 !== undefined)
        {
            file_descriptor.basic_rom = Uint8Array.from(FromBase64(ssfile.basic_rom_base64));
        }
        if(ssfile.charset_rom_base64 !== undefined)
        {
            file_descriptor.charset_rom = Uint8Array.from(FromBase64(ssfile.charset_rom_base64));
        }
        if(ssfile.floppy_rom_url !== undefined)
        {
            file_descriptor.floppy_rom = new Uint8Array(await (await fetch(ssfile.floppy_rom_url)).arrayBuffer());
        }
        if(ssfile.kernal_rom_url !== undefined)
        {
            file_descriptor.kernal_rom = new Uint8Array(await (await fetch(ssfile.kernal_rom_url)).arrayBuffer());
        }
        if(ssfile.basic_rom_url !== undefined)
        {
            file_descriptor.basic_rom = new Uint8Array(await (await fetch(ssfile.basic_rom_url)).arrayBuffer());
        }
        if(ssfile.charset_rom_url !== undefined)
        {
            file_descriptor.charset_rom = new Uint8Array(await (await fetch(ssfile.charset_rom_url)).arrayBuffer());
        }

        if(ssfile.name !== undefined)
        {
            file_descriptor.file_name = ssfile.name;
        }
        if(ssfile.bin !== undefined)
        {
            file_descriptor.file = ssfile.bin;
        }
        else if(ssfile.base64 !== undefined)
        {
            file_descriptor.file = Uint8Array.from(FromBase64(ssfile.base64));
        }
        else if(ssfile.url !== undefined)
        {
            const response = await fetch(ssfile.url, {cache: "no-store"});
            file_descriptor.file = new Uint8Array(await response.arrayBuffer());
        }

        vAmigaWeb_window.postMessage(
            file_descriptor, "*"
        );

        $("#btn_open_in_extra_tab").hide();
        $("#btn_overlay").css("margin-right", "0px");
    },
    load: async function(element, params, address) {
        if(address === undefined)
        {
            address = params;
            params = '';
        }
        if(this.listens == false)
        {
            window.addEventListener('message', event => {
                if(event.data.msg == "render_run_state")
                {
                    this.render_run_state(event.data.value);
                    if(event.data.value == true && this.samesite_file != null)
                    {
                        this.inject_samesite_app_into_iframe();
                    }
                }
                else if(event.data.msg == "render_current_audio_state")
                {
                    this.render_current_audio_state(event.data.value);
                }
            });
            this.listens=true;
        }
        if(window.jQuery)
        {
            this.load_into(element,params, address);
        }
        else
        {
            this.loadScript(`${this.vAmigaWeb_url}js/jquery-3.5.0.min.js` , 
            function(){vAmigaWeb_player.load_into(element,params, address);});
        }
    },
    show_reset_icon: false,
    saved_pic_html: null,
    preview_pic_width: "100%",
    load_into: function (element, params, address) {
        var this_element=null;
        if(element == null)
        {
            if(this.saved_pic_html != null)
            {
                //that is also the case when another player is still active... because the preview pic is saved
                //then get the other running player
                this_element = $(document.querySelector('#player_container'));
            }
            else
            {
                alert("parameter element in the call vAmigaWeb_player.load(element, ...) is null.");
                return;
            }
        }
        else
        {
            var this_element = $(element);
        }

        var emu_container = this_element.parent();
        
        this.stop_emu_view();

        //save preview pic
        this.saved_pic_html = emu_container.html();
        this.preview_pic_width= emu_container.children(":first").width();

        //turn picture into iframe
        var emuview_html = `
<div id="player_container" style="display:flex;flex-direction:column;">
<iframe id="vAmigaWeb" width="100%" height="100%" src="${this.vAmigaWeb_url}${params}#${address}">
</iframe>
<div style="display: flex"><svg id="stop_icon" class="player_icon_btn" onclick="vAmigaWeb_player.stop_emu_view();return false;" xmlns="http://www.w3.org/2000/svg" width="2.0em" height="2.0em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
    <path d="M6.5 5A1.5 1.5 0 0 0 5 6.5v3A1.5 1.5 0 0 0 6.5 11h3A1.5 1.5 0 0 0 11 9.5v-3A1.5 1.5 0 0 0 9.5 5h-3z"/>
    <path d="M0 4a2 2 0 0 1 2-2h12a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V4zm15 0a1 1 0 0 0-1-1H2a1 1 0 0 0-1 1v8a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V4z"/>
</svg>`;
if(this.show_reset_icon)
{
    emuview_html += 
    `<svg id="btn_reset" class="player_icon_btn" onclick="vAmigaWeb_player.reset();return false;" xmlns="http://www.w3.org/2000/svg" width="2.0em" height="2.0em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
    ${this.reset_icon}
    </svg>`;
}
emuview_html += `<svg  id="toggle_icon" class="player_icon_btn" onclick="vAmigaWeb_player.toggle_run();return false;" xmlns="http://www.w3.org/2000/svg" width="2.0em" height="2.0em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
${this.pause_icon}
</svg>
<svg id="btn_unlock_audio" class="player_icon_btn" onclick="vAmigaWeb_player.toggle_audio();return false;" xmlns="http://www.w3.org/2000/svg" width="2.0em" height="2.0em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
${this.audio_locked_icon}
</svg>
<svg id="btn_keyboard" class="player_icon_btn" onclick="vAmigaWeb_player.toggle_keyboard();return false;" xmlns="http://www.w3.org/2000/svg" width="2.0em" height="2.0em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
${this.keyboard_icon}
</svg>`;
if(address.toLowerCase().indexOf(".zip")>0)
{
    emuview_html += 
    `<svg id="btn_zip" class="player_icon_btn" style="margin-top:4px;margin-left:auto" onclick="vAmigaWeb_player.open_zip();return false;" xmlns="http://www.w3.org/2000/svg" width="1.5em" height="1.5em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
    ${this.folder_icon}
    </svg>`;
}
emuview_html += 
`<svg id="btn_overlay" class="player_icon_btn" style="margin-top:4px;margin-left:auto" onclick="vAmigaWeb_player.toggle_overlay();return false;" xmlns="http://www.w3.org/2000/svg" width="1.5em" height="1.5em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
${this.overlay_on_icon}
</svg>
<a id="btn_open_in_extra_tab" title="open fullwindow in new browser tab" style="border:none;width:1.5em;margin-top:4px" onclick="vAmigaWeb_player.stop_emu_view();" href="${this.vAmigaWeb_url}${params}#${address}" target="blank">
    <svg class="player_icon_btn" xmlns="http://www.w3.org/2000/svg" width="1.5em" height="1.5em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
  <path fill-rule="evenodd" d="M15 2a1 1 0 0 0-1-1H2a1 1 0 0 0-1 1v12a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V2zM0 2a2 2 0 0 1 2-2h12a2 2 0 0 1 2 2v12a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V2zm5.854 8.803a.5.5 0 1 1-.708-.707L9.243 6H6.475a.5.5 0 1 1 0-1h3.975a.5.5 0 0 1 .5.5v3.975a.5.5 0 1 1-1 0V6.707l-4.096 4.096z"/>
</svg>
</a>
</div>
</div>
`;
        emu_container.html(emuview_html); 

        $('#player_container').css("width",this.preview_pic_width);

        $vAmigaWeb = $('#vAmigaWeb');
        $vAmigaWeb.height($vAmigaWeb.width() * 212/320);
        $(window).on('resize', function() { 
            setTimeout(()=>{
                if( vAmigaWeb_player.is_overlay)
                {
                    vAmigaWeb_player.scale_overlay();
                }
                $vAmigaWeb.height($vAmigaWeb.width() * 212/320);
            }, 130);
            
        });

        $(window).on('orientationchange', function() { 
            setTimeout(()=>{
                let parent = $('#player_container').parent();
		while(parent.width()==0)
		{//skip parent elements without width
		  parent=parent.parent();		  
		}
		vAmigaWeb_player.preview_pic_width = parent.width();
                $('#player_container').css("width",vAmigaWeb_player.preview_pic_width);
            }, 100);
        });

        document.addEventListener("click", this.grab_focus);
        document.getElementById("vAmigaWeb").onload = this.grab_focus;

        this.state_poller = setInterval(function(){ 
            let vAmigaWeb=document.getElementById("vAmigaWeb");            
            vAmigaWeb.contentWindow.postMessage("poll_state", "*");
        }, 900);
    },
    grab_focus: function(){            
        let vAmigaWeb=document.getElementById("vAmigaWeb");            
        if(vAmigaWeb != null)
        {
            document.activeElement.blur();
            vAmigaWeb.focus();
        }
    },
    state_poller: null,
    audio_locked_icon:`<path d="M6.717 3.55A.5.5 0 0 1 7 4v8a.5.5 0 0 1-.812.39L3.825 10.5H1.5A.5.5 0 0 1 1 10V6a.5.5 0 0 1 .5-.5h2.325l2.363-1.89a.5.5 0 0 1 .529-.06zm7.137 2.096a.5.5 0 0 1 0 .708L12.207 8l1.647 1.646a.5.5 0 0 1-.708.708L11.5 8.707l-1.646 1.647a.5.5 0 0 1-.708-.708L10.793 8 9.146 6.354a.5.5 0 1 1 .708-.708L11.5 7.293l1.646-1.647a.5.5 0 0 1 .708 0z"/>`,
    audio_unlocked_icon:`<path d="M11.536 14.01A8.473 8.473 0 0 0 14.026 8a8.473 8.473 0 0 0-2.49-6.01l-.708.707A7.476 7.476 0 0 1 13.025 8c0 2.071-.84 3.946-2.197 5.303l.708.707z"/>
<path d="M10.121 12.596A6.48 6.48 0 0 0 12.025 8a6.48 6.48 0 0 0-1.904-4.596l-.707.707A5.483 5.483 0 0 1 11.025 8a5.483 5.483 0 0 1-1.61 3.89l.706.706z"/>
<path d="M8.707 11.182A4.486 4.486 0 0 0 10.025 8a4.486 4.486 0 0 0-1.318-3.182L8 5.525A3.489 3.489 0 0 1 9.025 8 3.49 3.49 0 0 1 8 10.475l.707.707zM6.717 3.55A.5.5 0 0 1 7 4v8a.5.5 0 0 1-.812.39L3.825 10.5H1.5A.5.5 0 0 1 1 10V6a.5.5 0 0 1 .5-.5h2.325l2.363-1.89a.5.5 0 0 1 .529-.06z"/>`,
    run_icon: `<path d="M6.79 5.093A.5.5 0 0 0 6 5.5v5a.5.5 0 0 0 .79.407l3.5-2.5a.5.5 0 0 0 0-.814l-3.5-2.5z"/>
        <path d="M0 4a2 2 0 0 1 2-2h12a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V4zm15 0a1 1 0 0 0-1-1H2a1 1 0 0 0-1 1v8a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V4z"/>`,
    pause_icon:`<path d="M6.25 5C5.56 5 5 5.56 5 6.25v3.5a1.25 1.25 0 1 0 2.5 0v-3.5C7.5 5.56 6.94 5 6.25 5zm3.5 0c-.69 0-1.25.56-1.25 1.25v3.5a1.25 1.25 0 1 0 2.5 0v-3.5C11 5.56 10.44 5 9.75 5z"/>
        <path d="M0 4a2 2 0 0 1 2-2h12a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V4zm15 0a1 1 0 0 0-1-1H2a1 1 0 0 0-1 1v8a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V4z"/>
        `,
    zip_icon:`<path d="M0 2a1 1 0 0 1 1-1h14a1 1 0 0 1 1 1v2a1 1 0 0 1-1 1v7.5a2.5 2.5 0 0 1-2.5 2.5h-9A2.5 2.5 0 0 1 1 12.5V5a1 1 0 0 1-1-1V2zm2 3v7.5A1.5 1.5 0 0 0 3.5 14h9a1.5 1.5 0 0 0 1.5-1.5V5H2zm13-3H1v2h14V2zM5 7.5a.5.5 0 0 1 .5-.5h5a.5.5 0 0 1 0 1h-5a.5.5 0 0 1-.5-.5z"/>`,
    folder_icon:`<path d="M1 3.5A1.5 1.5 0 0 1 2.5 2h2.764c.958 0 1.76.56 2.311 1.184C7.985 3.648 8.48 4 9 4h4.5A1.5 1.5 0 0 1 15 5.5v7a1.5 1.5 0 0 1-1.5 1.5h-11A1.5 1.5 0 0 1 1 12.5v-9zM2.5 3a.5.5 0 0 0-.5.5V6h12v-.5a.5.5 0 0 0-.5-.5H9c-.964 0-1.71-.629-2.174-1.154C6.374 3.334 5.82 3 5.264 3H2.5zM14 7H2v5.5a.5.5 0 0 0 .5.5h11a.5.5 0 0 0 .5-.5V7z"/>`,
    overlay_on_icon:`<path d="M1.5 1a.5.5 0 0 0-.5.5v4a.5.5 0 0 1-1 0v-4A1.5 1.5 0 0 1 1.5 0h4a.5.5 0 0 1 0 1h-4zM10 .5a.5.5 0 0 1 .5-.5h4A1.5 1.5 0 0 1 16 1.5v4a.5.5 0 0 1-1 0v-4a.5.5 0 0 0-.5-.5h-4a.5.5 0 0 1-.5-.5zM.5 10a.5.5 0 0 1 .5.5v4a.5.5 0 0 0 .5.5h4a.5.5 0 0 1 0 1h-4A1.5 1.5 0 0 1 0 14.5v-4a.5.5 0 0 1 .5-.5zm15 0a.5.5 0 0 1 .5.5v4a1.5 1.5 0 0 1-1.5 1.5h-4a.5.5 0 0 1 0-1h4a.5.5 0 0 0 .5-.5v-4a.5.5 0 0 1 .5-.5z"/>`,
    overlay_off_icon:`<path d="M5.5 0a.5.5 0 0 1 .5.5v4A1.5 1.5 0 0 1 4.5 6h-4a.5.5 0 0 1 0-1h4a.5.5 0 0 0 .5-.5v-4a.5.5 0 0 1 .5-.5zm5 0a.5.5 0 0 1 .5.5v4a.5.5 0 0 0 .5.5h4a.5.5 0 0 1 0 1h-4A1.5 1.5 0 0 1 10 4.5v-4a.5.5 0 0 1 .5-.5zM0 10.5a.5.5 0 0 1 .5-.5h4A1.5 1.5 0 0 1 6 11.5v4a.5.5 0 0 1-1 0v-4a.5.5 0 0 0-.5-.5h-4a.5.5 0 0 1-.5-.5zm10 1a1.5 1.5 0 0 1 1.5-1.5h4a.5.5 0 0 1 0 1h-4a.5.5 0 0 0-.5.5v4a.5.5 0 0 1-1 0v-4z"/>`,
    keyboard_icon:`<path d="M14 5a1 1 0 0 1 1 1v5a1 1 0 0 1-1 1H2a1 1 0 0 1-1-1V6a1 1 0 0 1 1-1h12zM2 4a2 2 0 0 0-2 2v5a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V6a2 2 0 0 0-2-2H2z"/><path d="M13 10.25a.25.25 0 0 1 .25-.25h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5a.25.25 0 0 1-.25-.25v-.5zm0-2a.25.25 0 0 1 .25-.25h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5a.25.25 0 0 1-.25-.25v-.5zm-5 0A.25.25 0 0 1 8.25 8h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 8 8.75v-.5zm2 0a.25.25 0 0 1 .25-.25h1.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-1.5a.25.25 0 0 1-.25-.25v-.5zm1 2a.25.25 0 0 1 .25-.25h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5a.25.25 0 0 1-.25-.25v-.5zm-5-2A.25.25 0 0 1 6.25 8h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 6 8.75v-.5zm-2 0A.25.25 0 0 1 4.25 8h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 4 8.75v-.5zm-2 0A.25.25 0 0 1 2.25 8h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 2 8.75v-.5zm11-2a.25.25 0 0 1 .25-.25h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5a.25.25 0 0 1-.25-.25v-.5zm-2 0a.25.25 0 0 1 .25-.25h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5a.25.25 0 0 1-.25-.25v-.5zm-2 0A.25.25 0 0 1 9.25 6h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 9 6.75v-.5zm-2 0A.25.25 0 0 1 7.25 6h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 7 6.75v-.5zm-2 0A.25.25 0 0 1 5.25 6h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 5 6.75v-.5zm-3 0A.25.25 0 0 1 2.25 6h1.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-1.5A.25.25 0 0 1 2 6.75v-.5zm0 4a.25.25 0 0 1 .25-.25h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5a.25.25 0 0 1-.25-.25v-.5zm2 0a.25.25 0 0 1 .25-.25h5.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-5.5a.25.25 0 0 1-.25-.25v-.5z"/>`,
    reset_icon:` <path d="M9.71 5.093a.5.5 0 0 1 .79.407v5a.5.5 0 0 1-.79.407L7 8.972V10.5a.5.5 0 0 1-1 0v-5a.5.5 0 0 1 1 0v1.528l2.71-1.935z"/>
  <path d="M0 4a2 2 0 0 1 2-2h12a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2V4zm15 0a1 1 0 0 0-1-1H2a1 1 0 0 0-1 1v8a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1V4z"/>`,
    is_overlay: false,
    toggle_overlay: function () {
        var container = $('#player_container');
        if( container.css('position') == "fixed")
        {
            this.is_overlay=false;
            $('#btn_overlay').html(this.overlay_on_icon);
            $('#player_container').css({"position": "", "top": "", "left": "", "width": this.preview_pic_width, "z-index": ""});
        }
        else
        {
            $('#btn_overlay').html(this.overlay_off_icon);
            this.scale_overlay();
            this.is_overlay=true;
        }
        $vAmigaWeb.height($vAmigaWeb.width() * 212/320);
        
        let vAmigaWeb=document.getElementById("vAmigaWeb");
        //the blur and refocus is only needed for safari, when it goes into overlay
        document.activeElement.blur(); 
        vAmigaWeb.focus();
    },
    scale_overlay: function(){
        var ratio=320/212; //1.6;
        var w2=window.innerHeight * ratio / window.innerWidth;
        var width_percent = Math.min(1,w2)*100;
        var calc_pixel_height = window.innerWidth*width_percent/100 / ratio;
        var height_percent = calc_pixel_height/window.innerHeight *100;
        var margin_top  = (100 -  height_percent )/2;
        if(margin_top<5)
        {//give some extra room for height of player bottom bar controls 
            width_percent -= 5.2; 
        }
        var margin_left = (100-width_percent)/2;

        $('#player_container').css({"position": "fixed", "top": `${margin_top}vh`, "left": `${margin_left}vw`, "width": `${width_percent}vw`, "z-index": 1000});
    },
    toggle_run: function () {
        var vAmigaWeb = document.getElementById("vAmigaWeb").contentWindow;
        //click emulators run toggle button
        vAmigaWeb.postMessage("button_run()", "*");
    },
    last_run_state:null,
    render_run_state: function (is_running)
    {
        if(this.last_run_state == is_running)
            return;

        $("#toggle_icon").html(
            is_running ? this.pause_icon : this.run_icon
        );
        this.last_run_state = is_running;
    },
    toggle_keyboard: function()
    {			
        var vAmigaWeb = document.getElementById("vAmigaWeb").contentWindow;
        vAmigaWeb.postMessage({cmd:"script", script:"virtual_keyboard_clipping=false;action('keyboard');"}, "*");
    },
    toggle_audio: function()
    {			
        var vAmigaWeb = document.getElementById("vAmigaWeb").contentWindow;
        vAmigaWeb.postMessage("toggle_audio()", "*");
    },
    last_audio_state:null,
    render_current_audio_state: function(state)
    {
        $('#btn_unlock_audio').html( state == 'running' ? this.audio_unlocked_icon : this.audio_locked_icon );
        this.last_audio_state=state;
    },
    open_zip: function () {
        var vAmigaWeb = document.getElementById("vAmigaWeb").contentWindow;
        //click emulators open_zip button
        vAmigaWeb.postMessage("open_zip()", "*");
    },
    reset: function () {
        this.exec(()=>{wasm_reset()});
    },    
    stop_emu_view: function() {
        //close any other active emulator frame
        if (this.saved_pic_html != null) {
            //restore preview pic
            $('#player_container').parent().html(this.saved_pic_html); 
            this.saved_pic_html=null;
        }
        if(this.state_poller != null)
        {
            clearInterval(this.state_poller);
        }
        this.last_run_state=null; 
        this.last_audio_state=null;

        document.removeEventListener("click", this.grab_focus);
    },
    send_script: function(the_script) { 
        let vAmigaWeb = document.getElementById("vAmigaWeb").contentWindow;
        vAmigaWeb.postMessage({cmd:"script", script: the_script}, "*");
    },
    exec: function(the_function, the_param) { 
        let function_as_string=`(${the_function.toString()})(${the_param == undefined?'':"'"+the_param+"'"});`;
        this.send_script(function_as_string);  
    }

}
