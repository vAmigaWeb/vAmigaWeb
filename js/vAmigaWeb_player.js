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
        let file_descriptor=ssfile;
        file_descriptor.cmd="load";
        if(ssfile.kickstart_rom_base64 !== undefined)
        {
            file_descriptor.kickstart_rom = Uint8Array.from(FromBase64(ssfile.kickstart_rom_base64));
        }
        if(ssfile.kickstart_rom_url !== undefined)
        {
            file_descriptor.kickstart_rom = new Uint8Array(await (await fetch(ssfile.kickstart_rom_url)).arrayBuffer());
        }
        if(ssfile.kickstart_ext_url !== undefined)
        {
            file_descriptor.kickstart_ext = new Uint8Array(await (await fetch(ssfile.kickstart_ext_url)).arrayBuffer());
        }
        if(ssfile.kickstart_rom !== undefined)
        {
            file_descriptor.kickstart_rom = ssfile.kickstart_rom;
        }
        if(ssfile.kickstart_ext !== undefined)
        {
            file_descriptor.kickstart_ext = ssfile.kickstart_ext;
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
                    this.render_warp_state(event.data.is_warping);
                    if(this.samesite_file != null)
                    {
                        this.inject_samesite_app_into_iframe();
                    }
                }
                else if(event.data.msg == "render_current_audio_state")
                {
                    this.render_current_audio_state(event.data.value);
                }
                else if(event.data.msg == "hide_zip_folder")
                {
                    $("#btn_zip").hide();
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
            this.loadScript(`${this.vAmigaWeb_url}js/jquery-3.6.0.min.js` , 
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
<div style="display:grid;grid-template-columns: repeat(5, auto) 3fr repeat(3, auto);grid-gap: 0.25em;">
<svg id="stop_icon" class="player_icon_btn" onclick="vAmigaWeb_player.stop_emu_view();return false;" xmlns="http://www.w3.org/2000/svg" width="2.0em" height="2.0em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
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
<svg id="btn_toggle_warp" class="player_icon_btn" style="" onclick="vAmigaWeb_player.toggle_warp();return false;" xmlns="http://www.w3.org/2000/svg" width="2em" height="2em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
${this.warp_icon}
</svg>

<svg id="btn_unlock_audio" class="player_icon_btn" onclick="vAmigaWeb_player.toggle_audio();return false;" xmlns="http://www.w3.org/2000/svg" width="2.0em" height="2.0em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
${this.audio_locked_icon}
</svg>
<svg id="btn_keyboard" class="player_icon_btn" onclick="vAmigaWeb_player.toggle_keyboard();return false;" xmlns="http://www.w3.org/2000/svg" width="2.0em" height="2.0em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
${this.keyboard_icon}
</svg>`;
if(address.toLowerCase().indexOf(".zip")>0 || this.samesite_file != null && this.samesite_file.name != null && this.samesite_file.name.endsWith(".zip"))
{
    emuview_html += 
    `<svg id="btn_zip" class="player_icon_btn" style="margin-top:4px;margin-left:auto" onclick="vAmigaWeb_player.open_zip();return false;" xmlns="http://www.w3.org/2000/svg" width="1.5em" height="1.5em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
    ${this.folder_icon}
    </svg>`;
}
emuview_html += 
`
<svg id="btn_activity_monitor" class="player_icon_btn" style="margin-top:4px;margin-left:auto" onclick="vAmigaWeb_player.toggle_activity_monitor();return false;" xmlns="http://www.w3.org/2000/svg" width="1.5em" height="1.5em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
${this.activity_icon}
</svg>

<svg id="btn_overlay" class="player_icon_btn" style="margin-top:4px;margin-left:auto" onclick="vAmigaWeb_player.toggle_overlay();return false;" xmlns="http://www.w3.org/2000/svg" width="1.5em" height="1.5em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16">
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
    activity_icon: `
  <path d="M4.5 12a.5.5 0 0 1-.5-.5v-2a.5.5 0 0 1 .5-.5h1a.5.5 0 0 1 .5.5v2a.5.5 0 0 1-.5.5zm3 0a.5.5 0 0 1-.5-.5v-4a.5.5 0 0 1 .5-.5h1a.5.5 0 0 1 .5.5v4a.5.5 0 0 1-.5.5zm3 0a.5.5 0 0 1-.5-.5v-6a.5.5 0 0 1 .5-.5h1a.5.5 0 0 1 .5.5v6a.5.5 0 0 1-.5.5z"/>
  <path d="M4 0a2 2 0 0 0-2 2v12a2 2 0 0 0 2 2h8a2 2 0 0 0 2-2V2a2 2 0 0 0-2-2zm0 1h8a1 1 0 0 1 1 1v12a1 1 0 0 1-1 1H4a1 1 0 0 1-1-1V2a1 1 0 0 1 1-1"/>`,
    warp_icon: `
  <path d="M8.79 5.093A.5.5 0 0 0 8 5.5v1.886L4.79 5.093A.5.5 0 0 0 4 5.5v5a.5.5 0 0 0 .79.407L8 8.614V10.5a.5.5 0 0 0 .79.407l3.5-2.5a.5.5 0 0 0 0-.814z"/>
  <path d="M0 4a2 2 0 0 1 2-2h12a2 2 0 0 1 2 2v8a2 2 0 0 1-2 2H2a2 2 0 0 1-2-2zm15 0a1 1 0 0 0-1-1H2a1 1 0 0 0-1 1v8a1 1 0 0 0 1 1h12a1 1 0 0 0 1-1z"/>`,
    warp_active_icon: `
  <path d="M0 4v8a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V4a2 2 0 0 0-2-2H2a2 2 0 0 0-2 2m4.271 1.055a.5.5 0 0 1 .52.038L8 7.386V5.5a.5.5 0 0 1 .79-.407l3.5 2.5a.5.5 0 0 1 0 .814l-3.5 2.5A.5.5 0 0 1 8 10.5V8.614l-3.21 2.293A.5.5 0 0 1 4 10.5v-5a.5.5 0 0 1 .271-.445"/>`,
    is_overlay: false,
    toggle_overlay: function () {
        var container = $('#player_container');
        if( container.css('position') == "fixed")
        {
            this.is_overlay=false;
            $('#btn_overlay').html(this.overlay_on_icon);
            container.css({"position": "", "top": "", "left": "", "width": this.preview_pic_width, "z-index": ""});
        }
        else
        {
            $('#btn_overlay').html(this.overlay_off_icon);
            this.scale_overlay();
            this.is_overlay=true;
        }
        document.body.setAttribute("player_expanded",this.is_overlay?"on":"off");
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
    last_warp_state:null,
    render_run_state: function (is_running)
    {
        if(this.last_run_state == is_running)
            return;

        $("#toggle_icon").html(
            is_running ? this.pause_icon : this.run_icon
        );
        this.last_run_state = is_running;
    },
    render_warp_state: function (is_warping)
    {
        if(this.last_warp_state == is_warping)
            return;

        $("#btn_toggle_warp").html(
            is_warping ? this.warp_active_icon: this.warp_icon 
        );
        this.last_warp_state = is_warping;
    },
    toggle_keyboard: function()
    {			
        var vAmigaWeb = document.getElementById("vAmigaWeb").contentWindow;
        vAmigaWeb.postMessage({cmd:"script", script:"action('keyboard')"}, "*");
    },
    toggle_activity_monitor: function()
    {
        var vAmigaWeb = document.getElementById("vAmigaWeb").contentWindow;
        vAmigaWeb.postMessage({cmd:"script", script:"action('activity_monitor')"}, "*");
    },
    toggle_warp: function()
    {
        var vAmigaWeb = document.getElementById("vAmigaWeb").contentWindow;
        vAmigaWeb.postMessage({cmd:"script", script:"action('toggle_warp')"}, "*");
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
        document.body.removeAttribute("player_expanded");
    },
    send_script: function(the_script) { 
        let vAmigaWeb = document.getElementById("vAmigaWeb").contentWindow;
        vAmigaWeb.postMessage({cmd:"script", script: the_script}, "*");
    },
    exec: function(the_function, the_param) { 
        let function_as_string=`(${the_function.toString()})(${the_param == undefined?'':"'"+the_param+"'"});`;
        this.send_script(function_as_string);  
    },
    registered_setups: {},
    setup: function(id,setup_config){
        this.registered_setups[id]=setup_config;

        let the_play=`<svg xmlns="http://www.w3.org/2000/svg" fill="currentColor" class="bi bi-play-circle" viewBox="0 0 16 16"><path d="M8 15A7 7 0 1 1 8 1a7 7 0 0 1 0 14m0 1A8 8 0 1 0 8 0a8 8 0 0 0 0 16"/><path d="M6.271 5.055a.5.5 0 0 1 .52.038l3.5 2.5a.5.5 0 0 1 0 .814l-3.5 2.5A.5.5 0 0 1 6 10.5v-5a.5.5 0 0 1 .271-.445"/></svg>`;
        let overlay=document.getElementById(`${id}_overlay`);

        if(setup_config.kickstart_rom_required === undefined)
        {
            overlay.innerHTML=`
            <div style="display:grid;grid-template-columns: repeat(3, 1fr); width:100%;height:100%">
            <div style="grid-column:1/span3;text-align:end;"></div>
            <div id="play_button" style="grid-column: 2/2;cursor:pointer"
                ontouchstart="touched=true"
                onclick="let touch=(typeof touched!='undefined')?touched:false;touched=false;vAmigaWeb_player.load_setup('${id}', {touch:touch})"
            >${the_play}</div>
            <div style="grid-column: 2/2"></div>
            `;
            return;
        }
        this.loadScript(`${this.vAmigaWeb_url}js/dexie.min.js` , 
        async function(){
            var db = new Dexie("kickstarts");
            db.version(1).stores({
              kickstart: "id"
            });

            await db.open();
            //check if kickstart is in local db
            let ks = await (db.kickstart.where('id').equals(setup_config.kickstart_rom_required[0]).toArray());
            
            if(ks.length==0)
            {
                let lock=`<svg xmlns="http://www.w3.org/2000/svg" style="fill:white;width:100%" viewBox="0 0 448 512" ><path d="M400 224h-24v-72C376 68.2 307.8 0 224 0S72 68.2 72 152v72H48c-26.5 0-48 21.5-48 48v192c0 26.5 21.5 48 48 48h352c26.5 0 48-21.5 48-48V272c0-26.5-21.5-48-48-48zm-104 0H152v-72c0-39.7 32.3-72 72-72s72 32.3 72 72v72z"></path></svg>`;
                overlay.innerHTML=`
                <input type="file" id="fileInput" style="display:none">
                <div id="drop_zone" style="display:grid;grid-template-columns: repeat(3, 1fr); width:100%;height:100%;cursor:pointer"
                    onclick="document.getElementById('fileInput').click()"
                >
                  <div style="grid-column:1/span 3;text-align:center">kickstart ${setup_config.kickstart_rom_required} required</div>
                  <div style="grid-column:2/2">${lock}</div>
                  <div style="grid-column:1/span3;text-align:center">select kickstart file with a click or drag file into here</div>
                </div>
                `;
                function import_kickstart(file){
                    const reader = new FileReader();            
                    reader.onload = function(event) {
                        const arrayBuffer = event.target.result;
                        const kick_rom = new Uint8Array(arrayBuffer);
                        db.kickstart.add({id: setup_config.kickstart_rom_required[0], rom: kick_rom});
                    };
            
                    reader.readAsArrayBuffer(file);
                    vAmigaWeb_player.setup(id,setup_config);
                }
                function handleDrop(event) {
                    event.preventDefault();
                    var file = event.dataTransfer.files[0];
                    import_kickstart(file);
                }
                function handleDragOver(event) {
                    event.preventDefault();
                }
                overlay.addEventListener('drop', handleDrop, false);
                overlay.addEventListener('dragover', handleDragOver, false);
                document.getElementById('fileInput').addEventListener('change', function(event) {
                    const file = event.target.files[0];
                    import_kickstart(file);
                });
                return;
            }
            
            if(ks[0].id==setup_config.kickstart_rom_required[0])
            {
                vAmigaWeb_player.delete_rom=(rom_id)=>{ 
                    db.kickstart.delete(rom_id);
                    vAmigaWeb_player.setup(id,setup_config);
                }
                overlay.innerHTML=`
                <div style="display:grid;grid-template-columns: repeat(3, 1fr); width:100%;height:100%">

                <div style="grid-column:1/span3;text-align:end;" >
                  <span title="local stored kickstart rom found">kickstart ${setup_config.kickstart_rom_required}</span>
                  <svg onclick="vAmigaWeb_player.delete_rom(${setup_config.kickstart_rom_required})" style="cursor:pointer" fill="currentColor" height="1.1em" viewBox="0 -7 16 22" width="1em" xmlns="http://www.w3.org/2000/svg"><path d="M2.5 1a1 1 0 0 0-1 1v1a1 1 0 0 0 1 1H3v9a2 2 0 0 0 2 2h6a2 2 0 0 0 2-2V4h.5a1 1 0 0 0 1-1V2a1 1 0 0 0-1-1H10a1 1 0 0 0-1-1H7a1 1 0 0 0-1 1H2.5zm3 4a.5.5 0 0 1 .5.5v7a.5.5 0 0 1-1 0v-7a.5.5 0 0 1 .5-.5zM8 5a.5.5 0 0 1 .5.5v7a.5.5 0 0 1-1 0v-7A.5.5 0 0 1 8 5zm3 .5a.5.5 0 0 0-1 0v7a.5.5 0 0 0 1 0v-7z" fill-rule="evenodd"></path></svg>                
                </div>

                <div id="play_button" style="grid-column: 2/2;cursor:pointer"
                    ontouchstart="touched=true"
                    onclick="let touch=(typeof touched!='undefined')?touched:false;touched=false;vAmigaWeb_player.load_setup('${id}', {touch:touch})"
                >${the_play}</div>
                <div style="grid-column: 2/2"></div>
                `;
                if(setup_config.samesite_file.mount_kickstart_in_dfn !== undefined ||
                    setup_config.samesite_file.patch_kickstart_into_address !== undefined)
                {
                    setup_config.samesite_file.kickemu_rom = ks[0].rom;
                }
                else
                {
                    setup_config.samesite_file.kickstart_rom = ks[0].rom;
                }
            }
        });
    },
    load_setup(setup_name, optional_params={touch:false}){
        let element = document.getElementById(setup_name);
        vAmigaWeb_player.samesite_file=this.registered_setups[setup_name].samesite_file;
        let config=this.registered_setups[setup_name].config;
        config.touch=optional_params.touch;
        vAmigaWeb_player.load(element,encodeURIComponent(JSON.stringify(config)));
    }
}
