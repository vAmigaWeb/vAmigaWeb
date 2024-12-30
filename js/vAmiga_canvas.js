let TPP=1;
let HBLANK_MIN=0x12*TPP;
let HPIXELS=912*TPP;
let VPIXELS=313;
let xOff = HBLANK_MIN;//252
let yOff=26 + 6;
let clipped_width=HPIXELS-xOff-8*TPP;
let clipped_height=VPIXELS-yOff ;

let ctx=null;

function create2d_context()
{
    const canvas = document.getElementById('canvas');
    ctx = canvas.getContext('2d');
    image_data=ctx.createImageData(HPIXELS,VPIXELS);
//    pixel_buffer=new Uint8Array(0);
}

function render_canvas()
{
    let pixels = Module._wasm_pixel_buffer() + yOff*(HPIXELS<<2);
    let pixel_buffer=new Uint8Array(Module.HEAPU32.buffer, pixels, HPIXELS*clipped_height<<2);
    image_data.data.set(pixel_buffer);

    //putImageData(imageData, dx, dy, dirtyX, dirtyY, dirtyWidth, dirtyHeight)
    ctx.putImageData(image_data,
        -xOff/*TPP*/,/*-yOff*/0, 
        /*x,y*/ 
        xOff/*TPP*/,/*yOff*/0 
        /* width, height */, 
        clipped_width, clipped_height); 
}

function js_set_display(_xOff, _yOff, _clipped_width,_clipped_height) {
    xOff=_xOff-HBLANK_MIN*4;
    yOff=_yOff;
    clipped_width =_clipped_width;
    clipped_height=_clipped_height;
    if(clipped_height%2!=0) clipped_height++; //when odd make the height even 
    if(clipped_height+yOff > VPIXELS)
    {
        clipped_height=(VPIXELS-yOff) & 0xfffe;
    }

    let the_canvas = document.getElementById("canvas");
    the_canvas.width=clipped_width;
    if(typeof gl != 'undefined' && gl!=null)
    {
        the_canvas.height=clipped_height*2;

        let VPOS_CNT=VPIXELS;
        let HPOS_CNT=HPIXELS;
        updateTextureRect(xOff /HPOS_CNT, yOff / VPOS_CNT, (xOff+clipped_width) / HPOS_CNT, (yOff+clipped_height)/VPOS_CNT); 
    }
    else
    {
        the_canvas.height=clipped_height;
    }
}

function scaleVMCanvas() {
    let the_canvas = document.getElementById("canvas");
    var src_width=clipped_width; //Module._wasm_get_render_width();
    var src_height=clipped_height*2;//Module._wasm_get_render_height()*2; 
    if(use_ntsc_pixel)
    {
        src_height*=52/44; //make NTSC a bit taller
    }
    var src_ratio = src_width/src_height; //1.25
/*        if(Module._wasm_get_renderer()==0)
    {//software renderer only has half of height pixels
        src_height*=2;
        src_ratio = src_width/src_height;
    }*/
    
    src_ratio*=1.03; //make it a bit wider
    
    var inv_src_ratio = src_height/src_width;
    var wratio = window.innerWidth / window.innerHeight;

    var topPos=0;
    if(wratio < src_ratio)
    {
        var reducedHeight=window.innerWidth*inv_src_ratio;
        //all lower than 1.25
        $("#canvas").css("width", "100%")
        .css("height", Math.round(reducedHeight)+'px');
        
        if($("#virtual_keyboard").is(":hidden"))
        {   //center vertical, if virtual keyboard and navbar not present
            topPos=Math.round((window.innerHeight-reducedHeight)/2);
        }
        else
        {//virtual keyboard is present
            var keyb_height= $("#virtual_keyboard").innerHeight();          
            //positioning directly stacked onto keyboard          
            topPos=Math.round(window.innerHeight-reducedHeight-keyb_height);
        }
        if(topPos<0)
        {
            topPos=0;
        }
    }
    else
    {
        //all greater than 1.25
        if(use_wide_screen)
        {
            $("#canvas").css("width", "100%"); 
        }
        else
        {
             $("#canvas").css("width", Math.round((window.innerHeight*src_ratio)) +'px');
        }
        $("#canvas").css("height", "100%"); 
    }

    $("#canvas").css("top", topPos + 'px');   
};
