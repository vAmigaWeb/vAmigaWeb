let HBLANK_MIN=0x12;
let HPIXELS=912;
let PAL_EXTRA_VPIXELS=140;
let VPIXELS=313;
let xOff = HBLANK_MIN;//252
let yOff=26 + 6;
let clipped_width=HPIXELS-xOff-8;
let clipped_height=VPIXELS-yOff ;

let ctx=null;

function createContext()
{
    const canvas = document.getElementById('canvas');
    let ctx2d = canvas.getContext('2d');
    image_data=ctx2d.createImageData(HPIXELS,VPIXELS);
    return ctx2d;
}

function render_canvas()
{
    let pixels = Module._wasm_pixel_buffer();
    pixel_buffer=new Uint8Array(Module.HEAPU32.buffer, pixels, HPIXELS*VPIXELS*4);

    if(ctx == null)
    {
        ctx = createContext();
    }
    image_data.data.set(pixel_buffer, 0);

/* SDL2 Rendering
    Uint8 *texture = (Uint8 *)(stable_ptr);
    SDL_Rect SrcR;

    SrcR.x = (xOff-HBLANK_MIN*4) *TPP;
    SrcR.y = yOff;
    SrcR.w = clipped_width * TPP;
    SrcR.h = clipped_height;

    SDL_UpdateTexture(screen_texture, &SrcR, texture+ (4*HPIXELS*TPP*SrcR.y) + SrcR.x*4, 4*HPIXELS*TPP);

    SDL_RenderCopy(renderer, screen_texture, &SrcR, NULL);
*/
    //putImageData(imageData, dx, dy, dirtyX, dirtyY, dirtyWidth, dirtyHeight)
    ctx.putImageData(image_data,-xOff+HBLANK_MIN*4,-yOff, /*x,y*/ xOff-HBLANK_MIN*4,yOff /* width, height */, clipped_width, clipped_height); 
}

function js_set_display(_xOff, _yOff, _clipped_width,_clipped_height) {
    xOff=_xOff;
    yOff=_yOff;
    clipped_width =_clipped_width;
    clipped_height=_clipped_height;
    let the_canvas = document.getElementById("canvas");
    the_canvas.width=clipped_width;
    the_canvas.height=clipped_height;
}

function scaleVMCanvas() {
    let the_canvas = document.getElementById("canvas");
    var src_width=Module._wasm_get_render_width();
    var src_height=Module._wasm_get_render_height()*2; 
    if(use_ntsc_pixel)
    {
        src_height*=52/44;
    }
    var src_ratio = src_width/src_height; //1.25
/*        if(Module._wasm_get_renderer()==0)
    {//software renderer only has half of height pixels
        src_height*=2;
        src_ratio = src_width/src_height;
    }*/

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
