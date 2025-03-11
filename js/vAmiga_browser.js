const vAmigaWeb_version="4.0.0"; //minimum requirement for snapshot version to be compatible
const compatible_snapshot_version_format=/^(4[.]0[.]0)$/g
var current_browser_datasource='snapshots';
var current_browser_command=null;

var already_loaded_collector = null;
var snapshot_browser_first_click=true;
var search_term='';
var latest_load_query_context=0;

workspace_path="/vamiga_workspaces"

function load_workspace(name){
    wasm_load_workspace(workspace_path+"/"+name)
    global_apptitle=name;

    let files = FS.readdir(workspace_path+"/"+name);
    let zip_file=files.filter(f=>f.toLowerCase().endsWith(".zip"));
    if(zip_file.length>0)
    {
        last_zip_archive_name = zip_file[0];
        last_zip_archive = FS.readFile(workspace_path+"/"+name+"/"+last_zip_archive_name, { encoding: 'binary' });

        $("#drop_zone").html('<svg width="1.3em" height="1.3em" viewBox="0 0 16 16" class="bi bi-archive-fill" fill="currentColor" xmlns="http://www.w3.org/2000/svg"><path fill-rule="evenodd" d="M12.643 15C13.979 15 15 13.845 15 12.5V5H1v7.5C1 13.845 2.021 15 3.357 15h9.286zM5.5 7a.5.5 0 0 0 0 1h5a.5.5 0 0 0 0-1h-5zM.8 1a.8.8 0 0 0-.8.8V3a.8.8 0 0 0 .8.8h14.4A.8.8 0 0 0 16 3V1.8a.8.8 0 0 0-.8-.8H.8z"/></svg> zip');
        $("#drop_zone").css("border", "3px solid var(--green)");
    }
}

function setup_browser_interface()
{
    var search_func= async function(){
        //window.alert('suche:'+ $('#search').val());
        search_term=$('#search').val();
        load_browser(current_browser_datasource, search_term.length>0 ? 'search':'feeds');
    }
    document.getElementById('search').onchange = search_func;
    document.getElementById('search_symbol').onclick= search_func;


    switch_collector = async function (collector_name)
    {
        for(current_col_name in collectors)
        {
            await get_data_collector(current_col_name).wait_until_finish();
        }

        if(collector_name== 'snapshots')
        {
            $('#sel_browser_snapshots').parent().removeClass('btn-secondary').removeClass('btn-primary')
            .addClass('btn-primary');
            $('#sel_browser_workspace_db').parent().removeClass('btn-secondary').removeClass('btn-primary')
            .addClass('btn-secondary');
            search_term=''; $('#search').val('').attr("placeholder", "search snapshots (local browser storage)");

        }
        else
        {
            $('#sel_browser_workspace_db').parent().removeClass('btn-secondary').removeClass('btn-primary')
            .addClass('btn-primary');
            $('#sel_browser_snapshots').parent().removeClass('btn-secondary').removeClass('btn-primary')
            .addClass('btn-secondary');

            search_term=''; $('#search').val('').attr("placeholder", "search for workspace");
        }
        browser_datasource=collector_name;

    }    


    document.getElementById('sel_browser_snapshots').onclick = async function(){
        await switch_collector('snapshots');
        load_browser('snapshots');
    }

    document.getElementById('sel_browser_workspace_db').onclick = async function(){
        await switch_collector('workspace_db');
        load_browser('workspace_db');
    }


    $('#snapshotModal').on('shown.bs.modal', function () {
        hide_all_tooltips();

        var view_detail=$("#view_detail");
        if(view_detail.is(":visible"))
        {
            view_detail.focus();
        }
    });


    $('#snapshotModal').on('hidden.bs.modal', function () {
      //  wasm_resume_auto_snapshots();
        if(is_running())
        {
            try{wasm_run();} catch(e) {}
        }
    });

    //button in navbar menu
    document.getElementById('button_snapshots').onclick = async function() 
    {
        await load_browser(current_browser_datasource);
        if(snapshot_browser_first_click)
        {//if there are no taken snapshots -> select workspace_db
            snapshot_browser_first_click=false;
            var snapshot_collector=get_data_collector("snapshots");
            //wait until snapshots are a loaded
            await snapshot_collector.wait_until_finish();
            //and now look into snapshots
            if(snapshot_collector.total_count==0)
            { 
//                document.getElementById('sel_browser_workspace_db').click();   
            }
        }   
    }
}




var like_icon_filled = `
<svg style="color:var(--red);width:1.6em;height:1.6em"><use xlink:href="img/sprites.svg#like_filled"/></svg>
`;
var like_icon_empty = `
<svg style="color:var(--gray);width:1.6em;height:1.6em"><use xlink:href="img/sprites.svg#like_empty"/></svg>
`;

async function load_browser(datasource_name, command="feeds")
{
    hide_all_tooltips();
    var collector_in_duty=get_data_collector(datasource_name);    
    await collector_in_duty.wait_until_finish();

    current_browser_datasource=datasource_name;
    current_browser_command = command;

    internal_usersnapshots_enabled=false;

    if(is_running())
    {
        wasm_halt();
    }

  //  wasm_suspend_auto_snapshots();

    //-- build snapshot feed
    var collector=get_data_collector(datasource_name);

    if(collector.can_like("",null))
    {
        $("#div_like").html(`<button id="like_filter" class="btn btn-sm icon mt-1"
        data-toggle="tooltip" data-placement="top" title="show favourites only">${like_icon_empty}</button>`);
    
        document.getElementById('like_filter').onclick= async function(){
            if(current_browser_command == "favourites")
                load_browser(current_browser_datasource);
            else
                load_browser(current_browser_datasource, "favourites");
        }
    }
    else
    {
        $("#div_like").empty();
    }


    if(collector.needs_reload() == false && command=="feeds")
    {
        return;
    }

    //empty all feeds
    latest_load_query_context++;
    //console.log(`empty grid container ctx=#${latest_load_query_context}`);
    $('#container_snapshots').empty();

    var render_persistent_snapshot=function(app_title, item){
        var x_icon = `
        <svg style="width:1.8em;height:1.8em"><use xlink:href="img/sprites.svg#x"/></svg>
        `;
        var export_icon = `
        <svg style="width:1.6em;height:1.6em"><use xlink:href="img/sprites.svg#export"/></svg>
        `;
        var scaled_width= 15;
        var canvas_width = 384;
        var canvas_height= 272;
        var the_html=
        '<div class="col-xs-4 mr-2">'
        +`<div id="card_snap_${item.id}" class="card" style="width: ${scaled_width}rem;">`
            +`<canvas id="canvas_snap_${item.id}" width="${canvas_width}" height="${canvas_height}" class="card-img-top rounded"></canvas>`;
        if(collector.can_delete(app_title, item.id))
        {
            the_html += '<button id="delete_snap_'+item.id+'" type="button" style="position:absolute;top:0;right:0;padding:0;" class="btn btn-sm icon">'+x_icon+'</button>';
        }
        if(collector.can_export(app_title, item.id))
        {
            the_html += '<button id="export_snap_'+item.id+'" type="button" style="position:absolute;bottom:0;right:0;padding:0;" class="btn btn-sm icon">'+export_icon+'</button>';
        }

        if(collector.can_like(app_title, item))
        {
            var like_icon = collector.is_like(app_title, item) ? like_icon_filled : like_icon_empty;
            the_html += '<button id="like_snap_'+item.id+'" type="button" style="position:absolute;top:3px;left:3px;padding:0;" class="btn btn-sm icon">'+like_icon+'</button>';
        }

        var label = item.name;
        if(label !== undefined && label != null)
        {
            the_html +='<p class="card-text browser-item-text">'+ $('<span>').text(label).html()+'</p>';
        }

        the_html +=
            '</div>'
        +'</div>';
        return the_html;
    }

    var row_renderer = function(ctx, app_title, app_snaps) {
        if(latest_load_query_context != ctx)
        {
            //console.log(`abort grid container append ${app_title}`);
            return;
        }
        else
        {
            //console.log(`grid container append ${app_title}`);
        }       

        //app_title=app_title.split(' ').join('_');
        var the_grid ="";
        if(app_snaps.length>0)
        {
            the_grid+='<div class="row mt-2" style="color: var(--gray)">'+app_title+'</div>';
        }
        the_grid+='<div class="row" data-toggle="tooltip" data-placement="left" title="'+app_title+'">';

        for(var z=0; z<app_snaps.length; z++)
        {
            the_grid += render_persistent_snapshot(app_title, app_snaps[z]);
        }
        the_grid+='</div>';
    
        $('#container_snapshots').append(the_grid);
        for(var z=0; z<app_snaps.length; z++)
        {
            var canvas_id= "canvas_snap_"+app_snaps[z].id;
            var delete_id= "delete_snap_"+app_snaps[z].id;
            var export_id= "export_snap_"+app_snaps[z].id;
            var like_id= "like_snap_"+app_snaps[z].id;
            var canvas = document.getElementById(canvas_id);
            var delete_btn = document.getElementById(delete_id);
            var like_btn = document.getElementById(like_id);
            var export_btn = document.getElementById(export_id);
            
            if(delete_btn != null)
            {
                delete_btn.onclick = function() {
                    let id = this.id.match(/delete_snap_(.*)/)[1];
                    collector.delete(id)
                    $("#card_snap_"+id).remove();
                    hide_all_tooltips();
                };
            }
            if(export_btn != null)
            {
                export_btn.onclick = function() {
                    let id = this.id.match(/export_snap_(.*)/)[1];
                    collector.export(id);
                    hide_all_tooltips();
                };
            }

            if(like_btn != null)
            {
                like_btn.onclick = function() {
                    let id = this.id.match(/like_snap_(.*)/)[1];
                    var like_val = collector.set_like(app_title, id);
                    $(this).html(like_val ? like_icon_filled : like_icon_empty);
                };
            }

            canvas.onclick = function() {
                let id = this.id.match(/canvas_snap_(.*)/)[1];
                collector.show_detail(app_title, id);
            };
            try {
                collector.draw_item_into_canvas(app_title, canvas, app_snaps[z]);  
            } catch(e) { console.error(e); }
              
        }
    }

    //start the loading process
    if(command=="feeds")
    {
        $("#like_filter").html(`${like_icon_empty}`);
        await collector.load(row_renderer);
    }
    else if(command == "search")
    {
        $("#like_filter").html(`${like_icon_empty}`);
        await collector.search(row_renderer);
    }
    else if(command == "favourites")
    {
        $("#like_filter").html(`${like_icon_filled}`);
        await collector.favourites(row_renderer);
    }

    already_loaded_collector=collector; 
}




var collectors = {
    snapshots: {
        total_count: 0,
        set_busy: function (busy_value) 
        { 
            console.log("snapshot.busy="+busy_value);
            this.busy = busy_value;

            if(busy_value)
            {
                $('#alert_wait').show();
            }
            else
            {
                $('#alert_wait').hide();
            }
        },
        busy: false,
        wait_until_finish: async function() {
            var i=0;
            console.log(`snapshot.check on busy in waituntil busy=`+this.busy);
            while(this.busy)
            {
                console.log(`snapshots collector is busy since ${i*10}ms`);
                await sleep(100);
                i++;
            }
        },
        needs_reload: () => true,
        search: async function (row_renderer){
            console.log("snapshot search");  
            await this.wait_until_finish(); 
            this.load(row_renderer);
        },
        load: async function (row_renderer){
            console.log("snapshot load");
            await this.wait_until_finish(); 
            this.set_busy(true);
            try
            {
                //first load autosave snapshots
                var auto_save_count=auto_snaps.length;
                var auto_save_items=[];
                for(var z=0; z<auto_save_count; z++)
                {
                    var new_item = new Object();
                    new_item.id='a'+z;
                    new_item.internal_id=z;
                    auto_save_items.push(new_item);
                }
                
                row_renderer(latest_load_query_context,'auto_save',auto_save_items);
                this.total_count=auto_save_count;

                //now the user snapshots
                var store_renderer = async function(app_titles)
                {
                    for(var t=0; t<app_titles.length;t++)
                    {
                        var app_title=app_titles[t];
                        if(search_term == '' || app_title.toLowerCase().indexOf(search_term.toLowerCase()) >= 0)
                        {
                            try {
                                let app_snaps = await get_snapshots_for_app_title(app_title);
                                get_data_collector('snapshots').total_count+=app_snaps.length;
                                row_renderer(latest_load_query_context, app_title, app_snaps);
                            } catch (error) {
                                console.error(error);
                                alert(error.message);
                                return;
                            }
                        }
                    }
                    get_data_collector('snapshots').set_busy(false);
                }
                await get_stored_app_titles(store_renderer);
            }
            catch(e)
            {
                console.error(`cannot read app titles...${e.message}`);
                get_data_collector('snapshots').set_busy(false);
            }
        },
        draw_item_into_canvas: function (app_title, teaser_canvas, item){
            if(app_title == 'auto_save')
            {
                var id = item.internal_id; 
                this.copy_autosnapshot_to_canvas(auto_snaps[id], teaser_canvas);
            }
            else
            {
                var src_data = item.data;
                var version = src_data[6] +'.'+src_data[7]+'.'+src_data[8];
                if(src_data[9]>0)
                {
                    version += `_beta${src_data[9]}`;
                }
                width=src_data[13]*256+ src_data[12];
                height=src_data[17]*256+ src_data[16];
                if(width==0)
                {//width is 0 if there is structure padding for 8 byte aligment instead of 4
                    width=src_data[13+4]*256+ src_data[12+4];
                    height=src_data[17+4]*256+ src_data[16+4];
                }
                if(version[0]>=3)
                {
                    width=src_data[13+4]*256+ src_data[12+4];
                    height=src_data[17+4]*256+ src_data[16+4];
                }
                var ctx = teaser_canvas.getContext("2d");
                teaser_canvas.width = width;
                teaser_canvas.height = height;
                if(ctx!=null)
                {
                    imgData=ctx.createImageData(width,height);
                
                    var data = imgData.data;
                    let snapshot_data = new Uint8Array(src_data, 40/* offset .. this number was a guess... */, data.length);

                    data.set(snapshot_data.subarray(0, data.length), 0);
                    ctx.putImageData(imgData,0,0); 
                
                    if(!version.match(compatible_snapshot_version_format))
                    {
                        ctx.translate(50, 0); // translate to rectangle center 
                        ctx.rotate((Math.PI / 180) * 27); // rotate
                        ctx.font = '32px serif';
                        ctx.fillStyle = '#DD0000';
                        ctx.fillText('compatible core '+version+' ', 10, 45);
                    }
                    else if(version!=vAmigaWeb_version)
                    {
                        ctx.translate(0, 230); // translate to rectangle center 
                        //ctx.rotate((Math.PI / 180) * 27); // rotate
                        ctx.font = '20px serif';
                        ctx.fillStyle = '#aa0';
                        ctx.fillText('saved with V'+version+' maybe incompatible', 10, 45);
                    }
                }
            }
            return; 
        },
        show_detail: function (app_title, id){
            if(app_title == 'auto_save')
            {
                var _id=id.substring(1);
                var snapshot_data =auto_snaps[_id];
                if(is_running())
                {
                    wasm_halt();
                }
                wasm_loadfile(
                            global_apptitle /*snapshot.title*/+".vAmiga",
                            snapshot_data);
                $('#snapshotModal').modal('hide');
                if(!is_running())
                {
                    $("#button_run").click();
                }            
            }
            else
            {
                get_snapshot_per_id(id,
                    function (snapshot) {
                        var version = snapshot.data[6] +'.'+snapshot.data[7]+'.'+snapshot.data[8];
                        if(snapshot.data[9]>0)
                        {
                            version += `_beta${snapshot.data[9]}`;
                        }

                        if(!version.match(compatible_snapshot_version_format))
                        {
                            alert(`This snapshot has been taken with vAmiga version ${version} and can not be loaded with the current version ${vAmigaWeb_version}. You can switch back to version ${version} in the settings, or alternatively, use workspaces, which remain compatible.`);
                            return;
                        }
                        wasm_loadfile(
                            snapshot.title+".vAmiga",
                            snapshot.data);
                        $('#snapshotModal').modal('hide');
                        global_apptitle=snapshot.title;
                        get_custom_buttons(global_apptitle, 
                            function(the_buttons) {
                                custom_keys = the_buttons;
                                install_custom_keys();
                            }
                        );
                        if(!is_running())
                        {
                            $("#button_run").click();
                        }            
                    }
                );
            }
            return; 
        },
        can_delete: function(app_title, the_id){
            return app_title == 'auto_save' ? false: true;
        },
        delete: function(id) {delete_snapshot_per_id(id)},
        can_export: function(app_title, the_id){
            return app_title == 'auto_save' ? false: true;
        },
        export: function(id) {
            get_snapshot_per_id(id,
                function (snapshot) {
                    let blob_data = new Blob([snapshot.data], {type: 'application/octet-binary'});
                    const url = window.URL.createObjectURL(blob_data);
                    const a = document.createElement('a');
                    a.style.display = 'none';
                    a.href = url;
            
                    let app_name = snapshot.title;
                    let extension_pos = app_name.indexOf(".");
                    if(extension_pos >=0)
                    {
                        app_name = app_name.substring(0,extension_pos);
                    }
                    a.download = app_name+'_snap'+snapshot.id+'.vAmiga';
                    document.body.appendChild(a);
                    a.click();
                    window.URL.revokeObjectURL(url);
                }
            );
        },
        can_like: function(app_title, item){
            return false;
        },
        //helper method...
        copy_snapshot_to_canvas: function(snapshot_ptr, canvas, width, height){ 
            var ctx = canvas.getContext("2d");
            canvas.width = width;
            canvas.height = height;
            imgData=ctx.createImageData(width,height);

            var data = imgData.data;

            let snapshot_data = new Uint8Array(Module.HEAPU8.buffer, snapshot_ptr, data.length);

            data.set(snapshot_data.subarray(0, data.length), 0);
            ctx.putImageData(imgData,0,0); 
        },
        copy_autosnapshot_to_canvas: function(snapshot_data, canvas){ 
            var ctx = canvas.getContext("2d");
            canvas.width = snapshot_data[13+4]*256+ snapshot_data[12+4];
            canvas.height = snapshot_data[17+4]*256+ snapshot_data[16+4];
            imgData=ctx.createImageData(canvas.width,canvas.height);

            var data = imgData.data;

            data.set(snapshot_data.subarray(0, data.length), 0);
            ctx.putImageData(imgData,0,0); 
        }

    },

    workspace_db: {
        busy: false,
        set_busy: function (busy_value) 
        { 
            console.log("workspace_db.busy="+busy_value);
            this.busy = busy_value;

            if(busy_value)
            {
                $('#alert_wait').show();
            }
            else
            {
                $('#alert_wait').hide();
            }
        },
        wait_until_finish: async function() {
            var i=0;
            console.log(`workspace_db.check on busy in waituntil busy=`+this.busy);
            while(this.busy)
            {
                console.log(`workspace_db collector is busy since ${i*10}ms`);
                await sleep(100);
                i++;
            }
        },
        all_ids: [],
        all_items: [],
        loaded_feeds: null,
        loaded_search: null,
        loaded_favourites: null,
        last_load_was_a_search: false,
        needs_reload: function ()
        { 
            return true;
        },
        favourites: async function (row_renderer){
            this.load(row_renderer, 
                path=>this.like_values.includes(path)
            )
        },
        search: async function (row_renderer){
            this.load(row_renderer, path=>path.toLowerCase().includes(search_term.toLowerCase()))
        },
        load: async function (row_renderer, filter_func=async ()=>true){
            await this.wait_until_finish();
            this.set_busy(true);
            last_load_was_a_search=false;
            try
            {
                this.all_ids= [];
                this.all_items= [];
                this.loaded_feeds = [];

                await mount_workspaces();
 
                let dirs = FS.readdir(workspace_path);
                console.log(dirs)
                dirs.sort((a,b)=>a.localeCompare(b));
                dirs=dirs.filter(filter_func);
                
                function loadImageFromFS(path) {
                    try
                    {
                        let data = FS.readFile(path, { encoding: 'binary' });

                        let blob = new Blob([data], { type: 'image/png' });
                        let url = URL.createObjectURL(blob);
                        return url;    
                    }
                    catch(e) {
                        return null;
                    }
                }


                let id_counter=0;
                let last_ws_item=null;
                items= []
                for(ws_item of dirs)
                {
                    if(!ws_item.startsWith("."))
                    {
                        if(items.length>0 && last_ws_item[0] != ws_item[0])
                        {
                            this.row_name = last_ws_item[0].toUpperCase();
                            row_renderer(latest_load_query_context, this.row_name,items);
                            items = []
                        }

                        let urlPreview= loadImageFromFS(workspace_path+"/"+ws_item+"/preview.png")

                        let files = FS.readdir(workspace_path+"/"+ws_item);
                        let time_stamp="";
                        try{
                            let stat=FS.stat(workspace_path+"/"+ws_item+"/config.retrosh");
                            time_stamp = stat.mtime.toLocaleString();
                        }
                        catch {}
                        
                        let new_item = { 
                            id:id_counter++,
                            workspace_name: ws_item, 
                            name: ws_item,
                            date: time_stamp,
                            files: files.filter(f=>!f.startsWith(".")),
                            screen_shot: urlPreview
                        }
                        this.all_items[new_item.id] = new_item;
                        items.push(new_item)

                        last_ws_item = ws_item;
                    }
                }
                if(items.length>0)
                {
                    this.row_name = last_ws_item[0].toUpperCase();
                    row_renderer(latest_load_query_context, this.row_name,items);
                    items = []
                }
            }
            finally
            {
                this.set_busy(false);
            }
        },
        draw_item_into_canvas: function (app_title, teaser_canvas, item){
            var ctx = teaser_canvas.getContext('2d');
            var img = new Image;
            img.onload = function(){
                if(ctx!=null && img != null)
                {
                    ctx.drawImage(img,0,0); // Or at whatever offset you like
                }
            };
            if(item.screen_shot != null)
            {
                img.src=item.screen_shot;
            }
            return; 
        },
        show_detail:function (app_title, id){
            var item = this.all_items[id];
            //fetching details
            if(item == null || item.details_already_fetched !== undefined )
            {
                this.render_detail(app_title, id);
            }
            else
            {
                //show already loaded content
                this.render_detail(app_title, id);
            }
        },
        render_detail: function (app_title, id){
            hide_all_tooltips();
            $("#view_detail").show().scrollTop(0).focus();

            $("#detail_back").click(function(){
                $("#view_detail").hide();
                $('#snapshotModal').focus();
            });

            var item = this.all_items[id];


            var like_icon = this.is_like(app_title, item) ? like_icon_filled : like_icon_empty;
            
var content = `
<div class="container-xl">
    <button id="like_detail_${item.id}" type="button" style="position:absolute;top:10%;right:10%;padding:0;z-index:3000" class="btn btn-sm icon">${like_icon}</button>
</div>

<div style="margin-left:10%;margin-right:10%">
    <span style="font-size:xx-large">${item.name}</span> <br>
    <span style="font-size:large">last saved ${item.date}</span>
</div>
<br>
`;


content += `<button type="button" id="detail_run${0}" class="btn btn-primary my-2">
load workspace
<svg width="1.8em" height="1.8em" viewBox="0 0 16 16" class="bi bi-play-fill" fill="currentColor" xmlns="http://www.w3.org/2000/svg"><path d="M11.596 8.697l-6.363 3.692c-.54.313-1.233-.066-1.233-.697V4.308c0-.63.692-1.01 1.233-.696l6.363 3.692a.802.802 0 0 1 0 1.393z"/></svg>
</button>
<br>
`;

            content+=`<div style="margin-left:20%;margin-top:15px;">
            <div>workspace files:</div>`;
            var link_id=0;
            for(var file of item.files.filter(f => f != "config.retrosh" && f != "preview.png"))
            {              
                content += `<div style="margin-left:15px">${file}</div>`;
            }
            content += '</div>'; //col
            content += '</div>'; //row
            content += '</div>'; //container

            $("#detail_content").html(content);


            document.getElementById(`like_detail_${item.id}`).onclick = function() {
                let id = this.id.match(/like_detail_(.*)/)[1];
                var like_val = get_data_collector("workspace_db").set_like(app_title, id);
                $(this).html(like_val ? like_icon_filled : like_icon_empty);
                $(`#like_snap_${id}`).html(like_val ? like_icon_filled : like_icon_empty);
            };

            link_id=0;
            for(var link of item.files)
            {
                $(`#detail_run${link_id}`).click(function (){
                    load_workspace(item.workspace_name);

                    $('#snapshotModal').modal('hide');
                });
                link_id++;
            }

            var esc_on_detail=function( event ) {
                event.stopPropagation();
                if(event.key === "Escape")
                {
                    $("#view_detail").hide();
                    $('#snapshotModal').focus();
                }
                return false;
            }

            $( "#view_detail" ).keyup(esc_on_detail)
            .keydown(function( event ) {
                event.stopPropagation();
                return false;
            });

        },
        can_delete: function(app_title, the_id){
            return true;
        },
        delete: function(id) {
            let item = this.all_items[id];
            deleteAllFiles(workspace_path+"/"+item.name);
            FS.rmdir(workspace_path+"/"+item.name);

             FS.syncfs(false,(error)=>
                {  
                   // $("#modal_take_snapshot").modal('hide');
                }
            )
        },
        can_export: function(app_title, the_id){
            return true;
        },
        export: function(id) {
            function zipFilesInTmpFolder(name) {
                const zip = new JSZip();
                const workspace_path=`/vamiga_workspaces/${name}`
                const files = FS.readdir(workspace_path);
    
                // Alle Dateien im Verzeichnis durchlaufen und zum Zip hinzufügen
                files.forEach(function(file) {
                    if (file !== '.' && file !== '..') {
                        const filePath = `${workspace_path}/${file}`;
                        const fileData = FS.readFile(filePath);  // Datei als Byte-Array lesen
                        zip.file(`${name}.vamiga/${file}`, fileData);  // Datei in das Zip-Archiv einfügen
                    }
                });
    
                // Zip-Datei generieren und zum Download anbieten
                zip.generateAsync({ type: "blob" })
                    .then(function(content) {
                        // Blob URL erzeugen und Download-Link erstellen
                        const url = URL.createObjectURL(content);
                        const link = document.createElement('a');
                        link.href = url;
                        link.download = `workspace_${name}.zip`;  // Name des Zip-Archivs
                        link.click();  // Download starten
                        URL.revokeObjectURL(url);  // Blob URL wieder freigeben
                    });
            }
            let item = this.all_items[id];

            zipFilesInTmpFolder(item.name);
        },
        can_like: function(app_title, item){
            if(this.like_values == null)
            {
               this.like_values = JSON.parse(load_setting('workspace_likes', '[]'));
            }
            return true;
        },
        is_like: function(app_title, item){
            return this.like_values.includes(item.name)
        },
        like_values: null,  
        set_like: function(app_title, id){

            let name = this.all_items[id].name;

            if(this.like_values.includes(name))
            {
                this.like_values = this.like_values.filter(n=>n != name); 
            }
            else
            {
                this.like_values.push(name);
            }
            //hier die positiven werte abspeichern z.b. in 
            save_setting('workspace_likes', JSON.stringify(this.like_values));
            return this.like_values.includes(name);
        }
    }

};


function get_data_collector(collector_name)
{
    return collectors[collector_name];
}



function property(xml_root_item, property_name) {
    var val=null;
    var xml_item=property_path(xml_root_item, property_name);
    try{
        val=xml_item[0].textContent;
    } catch {}
    return val;
}
function property_list(xml_root_item, property_name, matches=null) {
    var list=[];
    var xml_item=property_path(xml_root_item, property_name);
    try{
        for(var element of xml_item)
        {
            if(matches==null || element.textContent.match(matches)!=null)
            {
                list.push(element.textContent);
            }
        }
    } catch {}
    return list;
}
function property_path(xml_root_item, property_path) {
    var xml_element=xml_root_item;
    var path = property_path.split('/');
    try{
        for(var element_name of path)
        {
            if(xml_element instanceof HTMLCollection)
            {
              if(xml_element.length==0)
              {
                  return null;
              }
              xml_element = xml_element[0];

            }
            xml_element=xml_element.getElementsByTagName(element_name);
        }
    } catch(e) {
        console.error(e);
    }
    return xml_element;
}


function SlowRingBuffer(maxLength) {
  this.maxLength = maxLength;
}
SlowRingBuffer.prototype = Object.create(Array.prototype);
SlowRingBuffer.prototype.push = function(element) {
  Array.prototype.push.call(this, element);
  while (this.length > this.maxLength) {
    this.shift();
  }
}

var auto_snaps= new SlowRingBuffer(5);
var auto_snap_interval=null;
function set_take_auto_snapshots(on) {
    if(on == false && auto_snap_interval != null)
    {
        clearInterval(auto_snap_interval);
        auto_snap_interval=null;
    }
    else if(on && auto_snap_interval == null)
    {
        auto_snap_interval=setInterval(() => {
            if(is_running() && emulator_currently_runs==true)
            {
//                wasm_halt();
                var snapshot_json= wasm_take_user_snapshot();
//                wasm_run();
                var snap_obj = JSON.parse(snapshot_json);
                var snapshot_buffer = new Uint8Array(Module.HEAPU8.buffer, snap_obj.address, snap_obj.size);
        
                //snapshot_buffer is only a typed array view therefore slice, which creates a new array with byteposition 0 ...
                auto_snaps.push(snapshot_buffer.slice(0,snap_obj.size));
                wasm_delete_user_snapshot();

                console.log(`auto_snaps count = ${auto_snaps.length}`);
            }
        }, 5000);
    }
}

function deleteAllFiles(path) {
    let files = FS.readdir(path);
    for (let file of files) {
        if (file === '.' || file === '..') continue;
        let fullPath = path + '/' + file;
        let stats = FS.stat(fullPath);
        if (!stats.isDir) {
            FS.unlink(fullPath);
        }
    }
}