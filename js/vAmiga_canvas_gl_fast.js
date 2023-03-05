let flicker_weight=1.0; // set 0.5 or 0.6 for interlace flickering
function render_canvas_gl(now)
{
    if(updateFullTexture(now)) render();
}

// Reference to the canvas element
//let canvas = null;

// The rendering context of the canvas
let gl=null;

// Indicates whether the recently drawn frames were long or short frames
let currLOF = true;
let prevLOF = true;

// Frame counter
let frameNr = 0;

// Variable used to emulate interlace flickering
let flickerCnt = 0;

// Buffers
let vBuffer=null; //WebGLBuffer;
let tBuffer=null; //WebGLBuffer;

// Textures
let lfTexture=null; //WebGLTexture;
let sfTexture=null; //WebGLTexture;
let mergeTexture=null; //WebGLTexture;

// The merge shader for rendering the merge texture
let lfWeight = null; //WebGLUniformLocation;
let sfWeight = null; //WebGLUniformLocation;
let sfSampler = null; //WebGLUniformLocation;
let lfSampler = null; //WebGLUniformLocation;


// The main shader for drawing the final texture on the canvas
let mainShaderProgram=null; //: WebGLProgram;
let sampler= null; //: WebGLUniformLocation;
let fSampler = null; //WebGLUniformLocation;

//
// Main shader
// 
const vsMain = `#version 300 es
    precision mediump float;
    in vec4 aVertexPosition;
    in vec2 aTextureCoord;
    out vec2 vTextureCoord;
    void main() {
        gl_Position = aVertexPosition;
        vTextureCoord = aTextureCoord;
    }
`;

const fsMain = `#version 300 es
    precision mediump float;
    uniform sampler2D sampler;
    in vec2 vTextureCoord;
    out vec4 o_color;
    void main() {
        o_color = texture(sampler, vTextureCoord);
    }
`;


const vsMerge = `#version 300 es
    precision mediump float;
    in vec4 aVertexPosition;
    in vec2 aTextureCoord;
    uniform vec2 diw_size;
    out vec2 vTextureCoord;
    out vec2 amiga_pos;
    void main() {
        amiga_pos = aVertexPosition.xy * diw_size.xy;
        gl_Position = aVertexPosition;
        vTextureCoord = aTextureCoord;
    }
`;

const fsMerge = `#version 300 es
precision mediump float;

uniform sampler2D u_lfSampler;
uniform sampler2D u_sfSampler;

uniform float u_lweight;
uniform float u_sweight;

in vec2 amiga_pos;
in vec2 vTextureCoord;

out vec4 color;

void main() {
  vec4 pixel;
  float w;
  float y = amiga_pos.y; 
  //float y = floor(gl_FragCoord.y);

  if (mod(y, 2.0) <1.0) {
    //w = u_sweight;
    color = texture(u_sfSampler, vTextureCoord);
    //color = vec4(1.0, 0.0, 0.0, 1.0); 

  } else {
    //w = u_lweight;
    color = texture(u_lfSampler, vTextureCoord);
    //color = vec4(1.0, 1.0, 0.0, 1.0); 
}

  //color = pixel * vec4(w, w, w, 1.0);
}
`;

function initWebGL() {
    console.log('initWebGL()');

    // General WebGL options
    const options = {
        alpha: false,
        antialias: false,
        depth: false,
        preserveDrawingBuffer: false,
        stencil: false
    };

    // Only proceed if WebGL2 is supported
    if (canvas.getContext('webgl2', options) ==null) {
        throw new Exception("no webgl2");
    }

    // Store the context for further use
    gl = canvas.getContext('webgl2', options); //WebGL2RenderingContext;
    gl.disable(gl.BLEND);
    gl.disable(gl.DEPTH_TEST);
    gl.disable(gl.SCISSOR_TEST);
    gl.disable(gl.STENCIL_TEST);

    // Start with a clean buffer
    gl.clearColor(0.0, 0.0, 0.0, 1.0);
    gl.clear(gl.COLOR_BUFFER_BIT);

    // Create the main shader
    mainShaderProgram = compileProgram(vsMain, fsMain);
    sampler = gl.getUniformLocation(mainShaderProgram, 'sampler');
    gl.uniform1i(sampler, 0);


    // Create the merge shader
    mergeShaderProgram = compileProgram(vsMerge, fsMerge);

    lfWeight = gl.getUniformLocation(mergeShaderProgram, 'u_lweight');
    sfWeight = gl.getUniformLocation(mergeShaderProgram, 'u_sweight');

    lfSampler = gl.getUniformLocation(mergeShaderProgram, 'u_lfSampler');
    gl.uniform1i(lfSampler, 1);    

    sfSampler = gl.getUniformLocation(mergeShaderProgram, 'u_sfSampler');
    gl.uniform1i(sfSampler, 0);    

    diw_size = gl.getUniformLocation(mergeShaderProgram, 'diw_size');



    // Setup the vertex coordinate buffer
    const vCoords = new Float32Array([-1.0, 1.0, 1.0, 1.0, -1.0, -1.0, 1.0, -1.0]);
    vBuffer = createBuffer(vCoords);
    setAttribute(mainShaderProgram, 'aVertexPosition');
    setAttribute(mergeShaderProgram, 'aVertexPosition');

    // Setup the texture coordinate buffer
    const tCoords = new Float32Array([0.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 0.0]);
    tBuffer = createBuffer(tCoords);
    setAttribute(mainShaderProgram, 'aTextureCoord');
    setAttribute(mergeShaderProgram, 'aTextureCoord');

    // Create textures
    lfTexture = createTexture(HPIXELS, VPIXELS);
    sfTexture = createTexture(HPIXELS, VPIXELS);

}

function updateTextureRect(x1, y1, x2, y2) {
    // console.log("updateTextureRect(" + x1 + ", " + y1 + " ," + x2 + ", " + y2 + ")");
    const array = new Float32Array([x1, y1, x2, y1, x1, y2, x2, y2]);
    //console.log(array);

    gl.bindBuffer(gl.ARRAY_BUFFER, tBuffer);
    gl.bufferSubData(gl.ARRAY_BUFFER, 0, array);

    gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);

    gl.useProgram(mergeShaderProgram);
    gl.uniform2f(diw_size, clipped_width, -clipped_height);
}


function compileProgram(vSource, fSource) {
    const vert = compileShader(gl.VERTEX_SHADER, vSource);
    const frag = compileShader(gl.FRAGMENT_SHADER, fSource);
    const prog = gl.createProgram();

    gl.attachShader(prog, vert);
    gl.attachShader(prog, frag);
    gl.linkProgram(prog);

    if (!gl.getProgramParameter(prog, gl.LINK_STATUS)) {
        throw new Error(`Shader link error: ${gl.getProgramInfoLog(prog)}`);
    }

    gl.useProgram(prog);
    return prog;
}

function compileShader(type, source) {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);

    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        throw new Error(`Shader compile error: ${gl.getShaderInfoLog(shader)}`);
    }
    return shader;
}

function createTexture(width, height) {
    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, width, height, 0, gl.RGBA, gl.UNSIGNED_BYTE, null);

    return texture;
}

function createBuffer(values /*Float32Array*/) {
    const buffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
    gl.bufferData(gl.ARRAY_BUFFER, values, gl.STATIC_DRAW);
    return buffer;
}

function setAttribute(program, attribute) {
    const a = gl.getAttribLocation(program, attribute);
    gl.enableVertexAttribArray(a);
    gl.vertexAttribPointer(a, 2, gl.FLOAT, false, 0, 0);
}

function updateSubTexture() {
    let frame_info=Module._wasm_frame_info();
    currLOF=frame_info & 1;
    frame_info = frame_info>>>1; 
    prevLOF=frame_info & 1;
    frame_frameNr = frame_info>>>1; 
    
    // Check for duplicate frames or frame drops
    if (frame_frameNr != frameNr + 1) {
        // console.log('Frame sync mismatch: ' + frameNr + ' -> ' + frame.frameNr);

        // Return immediately if we already have this texture
        if (frame_frameNr === frameNr) return false;
    }

    frameNr = frame_frameNr;

//  let frame_data = Module._wasm_pixel_buffer();
//  let tex=new Uint8Array(Module.HEAPU8.buffer, frame_data, w*h<<2);

    let frame_data = Module._wasm_pixel_buffer()+ yOff*(HPIXELS<<2);
    let tex=new Uint8Array(Module.HEAPU8.buffer, frame_data, clipped_height*(HPIXELS<<2));

    // Update the GPU texture
    if (currLOF) {
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, lfTexture);
    } else {
        gl.activeTexture(gl.TEXTURE1);
        gl.bindTexture(gl.TEXTURE_2D, sfTexture);
    }

    gl.pixelStorei(gl.UNPACK_ROW_LENGTH, HPIXELS);
    gl.pixelStorei(gl.UNPACK_SKIP_PIXELS, xOff);

    gl.texSubImage2D(gl.TEXTURE_2D, 0, xOff, yOff, clipped_width, clipped_height, gl.RGBA, gl.UNSIGNED_BYTE, tex);
    return true;
}
function updateFullTexture() {
    let frame_info=Module._wasm_frame_info();
    currLOF=frame_info & 1;
    frame_info = frame_info>>>1; 
    prevLOF=frame_info & 1;
    frame_frameNr = frame_info>>>1; 
    
    // Check for duplicate frames or frame drops
    if (frame_frameNr != frameNr + 1) {
        // console.log('Frame sync mismatch: ' + frameNr + ' -> ' + frame.frameNr);

        // Return immediately if we already have this texture
        if (frame_frameNr === frameNr) return false;
    }

    frameNr = frame_frameNr;

//  let frame_data = Module._wasm_pixel_buffer();
//  let tex=new Uint8Array(Module.HEAPU8.buffer, frame_data, w*h<<2);

    let frame_data = Module._wasm_pixel_buffer();
    let tex=new Uint8Array(Module.HEAPU8.buffer, frame_data, VPIXELS*HPIXELS<<2);

    // Update the GPU texture
    if (currLOF) {
        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, lfTexture);
    } else {
        gl.activeTexture(gl.TEXTURE1);
        gl.bindTexture(gl.TEXTURE_2D, sfTexture);
    }

    gl.texSubImage2D(gl.TEXTURE_2D, 0, 0, 0, HPIXELS, VPIXELS, gl.RGBA, gl.UNSIGNED_BYTE, tex);
    return true;
}

function render() {
//    gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);

  //  gl.activeTexture(currLOF? gl.TEXTURE0: gl.TEXTURE1);
  //  gl.bindTexture(gl.TEXTURE_2D, currLOF? lfTexture:sfTexture);

    if (currLOF === prevLOF) {
        gl.useProgram(mainShaderProgram);
        if (currLOF) {            
            // Case 1: Non-interlace mode, two long frames in a row
            gl.uniform1i(sampler, 0);
        } else {
            // Case 2: Non-interlace mode, two short frames in a row
            gl.uniform1i(sampler, 1);
        }
    } else {
        // Case 3: Interlace mode, long frame followed by a short frame
        gl.useProgram(mergeShaderProgram);
//        gl.uniform1i(lfSampler, 1);
//        gl.uniform1i(sfSampler, 0);
        
//        gl.uniform2f(diw_size, clipped_width, -clipped_height);

//        gl.activeTexture(gl.TEXTURE0);
//        gl.bindTexture(gl.TEXTURE_2D, lfTexture);
//        gl.activeTexture(gl.TEXTURE1);
//        gl.bindTexture(gl.TEXTURE_2D, sfTexture);
        
/*
        const weight = flicker_weight;//0.5; // TODO: USE OPTION PARAMETER

        if (weight) {
            gl.useProgram(mergeShaderProgram);
            gl.uniform1f(lfWeight, flickerCnt % 4 >= 2 ? 1.0 : weight);
            gl.uniform1f(sfWeight, flickerCnt % 4 >= 2 ? weight : 1.0);
            flickerCnt += 1;
        }
*/
    }

    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
}


