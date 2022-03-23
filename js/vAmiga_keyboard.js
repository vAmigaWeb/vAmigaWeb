var shift_pressed_count=0;
function translateKey(keycode, key)
{
    console.log('keycode='+keycode + ', key='+key);
    let mapindex=key_translation_map[ keycode ];
    return [mapindex,0];
}

function isUpperCase(s){
    return s.toUpperCase() == s && s.toLowerCase() != s;
}


function translateKey2(keycode, key)
{
    console.log('keycode='+keycode + ', key='+key);
    if(keycode == 'Space' && key == '^')
    {//fix for windows system
        key=' ';
    }
    let mapindex;
    let raw_key_with_modifier = { modifier: null,  raw_key: undefined }

    mapindex=key_translation_map[ keycode ];
    raw_key_with_modifier.raw_key = [mapindex,0];

    return raw_key_with_modifier.raw_key === undefined ?
            undefined:
            raw_key_with_modifier;
}


function translateSymbol(symbol)
{
    console.log('symbol='+symbol);
    let mapindex;
    let raw_key_with_modifier = { modifier: null,  raw_key: undefined }

    //check key matches a symbol
    let sym_key = symbolic_map[symbol];
    if(sym_key === undefined && isUpperCase(symbol))
    {//get the lowercase variant and press shift
        sym_key = symbolic_map[symbol.toLowerCase()];
        if(sym_key != undefined && !Array.isArray(sym_key))
        {
            sym_key = ['ShiftLeft', sym_key];
        }
    }
    if(sym_key!== undefined)
    {
        raw_key_with_modifier = create_key_composition(sym_key);
    }
    else
    {
        mapindex=key_translation_map[ symbol ];
        if(mapindex != undefined)
        {
            raw_key_with_modifier.raw_key = [mapindex,0];
        }
    }

    return raw_key_with_modifier.raw_key === undefined ?
            undefined:
            raw_key_with_modifier;
}


function create_key_composition(entry_from_symbolic_map)
{
    var mapindex;
    var raw_key_with_modifier = { modifier: null,  raw_key: null };

    if(Array.isArray(entry_from_symbolic_map))
    {
        mapindex=key_translation_map[ entry_from_symbolic_map[0] ];
        raw_key_with_modifier.modifier = [mapindex,0]; //c64keymap[mapindex];

        mapindex=key_translation_map[ entry_from_symbolic_map[1] ];
        raw_key_with_modifier.raw_key = [mapindex,0]; //c64keymap[mapindex];
    }
    else
    {
        mapindex=key_translation_map[ entry_from_symbolic_map];
        raw_key_with_modifier.raw_key = [mapindex,0]; //c64keymap[mapindex];
    }
    return raw_key_with_modifier;
}


symbolic_map = {
    a: 'KeyA',
    b: 'KeyB',
    c: 'KeyC',
    d: 'KeyD',
    e: 'KeyE',
    f: 'KeyF',
    g: 'KeyG',
    h: 'KeyH',
    i: 'KeyI',
    j: 'KeyJ',
    k: 'KeyK',
    l: 'KeyL',
    m: 'KeyM',
    n: 'KeyN',
    o: 'KeyO',
    p: 'KeyP',
    q: 'KeyQ',
    r: 'KeyR',
    s: 'KeyS',
    t: 'KeyT',
    u: 'KeyU',
    v: 'KeyV',
    w: 'KeyW',
    x: 'KeyX',
    z: 'KeyZ',
    y: 'KeyY',

    F1: 'F1',
    F2: 'F2',
    F3: 'F3',
    F4: 'F4',
    F5: 'F5',
    F6: 'F6',
    F7: 'F7',
    F8: 'F8',
    ',': 'Comma',
    '*': 'BracketRight', 
    "1": 'Digit1',
    "2": 'Digit2',
    "3": 'Digit3',
    "4": 'Digit4',
    "5": 'Digit5',
    "6": 'Digit6',
    "7": 'Digit7',
    "8": 'Digit8',
    "9": 'Digit9',
    "0": 'Digit0',
    ' ': 'Space',
    ':': 'Semicolon',
    '.': 'Period',
    ';': 'Quote',
    '=': 'Backslash', 
    '!': ['ShiftLeft','Digit1'],
    '"': ['ShiftLeft','Digit2'],
    '#': ['ShiftLeft','Digit3'],
    '§': ['ShiftLeft','Digit3'],
    '$': ['ShiftLeft','Digit4'],
    '%': ['ShiftLeft','Digit5'],
    '&': ['ShiftLeft','Digit6'],
    "'": ['ShiftLeft','Digit7'],
    '(': ['ShiftLeft','Digit8'],
    ')': ['ShiftLeft','Digit9'],
    '+': 'Minus',
    '/': 'Slash',
    '?':  ['ShiftLeft','Slash'],
    '-': 'Equal',
    '@': 'BracketLeft',
    '[': ['ShiftLeft','Semicolon'],
    ']': ['ShiftLeft','Quote'],
    '<': ['ShiftLeft','Comma'],
    '>': ['ShiftLeft','Period'],
    'shiftrunstop': ['ShiftLeft','runStop'],   //load from tape shortcut
//    'ArrowLeft': ['ShiftLeft','ArrowRight'],
//    'ArrowUp': ['ShiftLeft','ArrowDown'],
    '\n': 'Enter',
    'Dead': 'upArrow', '^': 'upArrow' //^
}

key_translation_map =  
        {//https://github.com/dirkwhoffmann/vAmiga/blob/164c04d75f0ae739dd9f2ff2c28520db05e7c047/GUI/Peripherals/AmigaKey.swift

//--- ANSI
    Grave:0,
    Digit1:1,
    Digit2:2, 
    Digit3:3,
    Digit4:4,
    Digit5:5,
    Digit6:6,
    Digit7:7,
    Digit8:8,
    Digit9:9,
    Digit0:10,
    Minus: 0x0B, 
    Equal: 0x0C, 
    Backslash: 0x0D, 

    Numpad0:  0x0F,

    KeyQ     :0x10,
    KeyW     :0x11,
    KeyE     :0x12,
    KeyR     :0x13,
    KeyT     :0x14,
    KeyY     :0x15,
    KeyU     :0x16,
    KeyI     :0x17,
    KeyO     :0x18,
    KeyP     :0x19,
    BracketLeft: 0x1a,
    BracketRight: 0x1b,

    Numpad1:  0x1d,
    Numpad2:  0x1e,
    Numpad3:  0x1f,

    KeyA     :0x20,
    KeyS     :0x21,
    KeyD     :0x22,
    KeyF     :0x23,
    KeyG     :0x24,
    KeyH     :0x25,
    KeyJ     :0x26,
    KeyK     :0x27,
    KeyL     :0x28,
    Semicolon:0x29,
    Quote    :0x2a,

    Numpad4:  0x2d,
    Numpad5:  0x2e,
    Numpad6:  0x2f,

    KeyZ     :0x31,
    KeyX     :0x32,
    KeyC     :0x33,
    KeyV     :0x34,
    KeyB     :0x35,
    KeyN     :0x36,
    KeyM     :0x37,
    Comma    :0x38,
    Period   :0x39,
    Slash    :0x3a,

    NumpadDecimal: 0x3c,
    Numpad7:  0x3d,
    Numpad8:  0x3e,
    Numpad9:  0x3f,

//--- Extra Keys on international Amigas (ISO style)
    hashtag: 0x2b, //[.generic: "", .german: "^ #", .italian: "§ \u{00F9}"], 
    laceBrace: 0x30, // [.generic: "", .german: "> <", .italian: "> <"],

// Amiga keycodes 0x40 - 0x5F (Codes common to all keyboards)
    Space:      0x40,
    Backspace:  0x41,
    Tab:        0x42,
    NumpadEnter:0x43,
    Enter:      0x44,
    Escape:     0x45,
    Delete:     0x46,
    NumpadSubtract: 0x4A,
    ArrowUp:    0x4C,
    ArrowDown:  0x4D,
    ArrowRight: 0x4E,
    ArrowLeft:  0x4F,
    F1:0x50,
    F2:0x51,
    F3:0x52,
    F4:0x53,
    F5:0x54,
    F6:0x55,
    F7:0x56,
    F8:0x57,
    F9:0x58,
    F10:0x59,

    keypadLBracket: 0x5A, //?? no javascript code
    keypadRBracket: 0x5B, //?? no javascript code
    NumpadDivide:   0x5C,
    NumpadMultiply: 0x5D,
    NumpadAdd:      0x5E,
    Help:           0x5F,

// 0x60 - 0x67 (Key codes for qualifier keys)
    ShiftLeft:  0x60,
    ShiftRight: 0x61,
    CapsLock:   0x62,
    ControlLeft:0x63,
    AltLeft:    0x64,
    AltRight:   0x65,
    leftAmiga:  0x66, //?? no javascript code
    rightAmiga: 0x67, //?? no javascript code   
}        

function reset_keyboard()
{
    let reset_mod_key=function(the_mod_key){
        document.body.setAttribute(the_mod_key+'-key', '');
    }
    reset_mod_key('ControlLeft');
    reset_mod_key('ShiftLeft');
    reset_mod_key('ShiftRight');
    reset_mod_key('shift');
    reset_mod_key('CapsLock');
    reset_mod_key('leftAmiga');
    reset_mod_key('rightAmiga');
    reset_mod_key('AltLeft');
    reset_mod_key('AltRight');

    shift_pressed_count=0;
}

function installKeyboard() {
    reset_keyboard();

    keymap= [ 
    [{k:'Esc', c:'Escape',cls:'darkkey'},{style:'width:20px'},{k:'F1',c:'F1', cls:'darkkey'}, {k:'F2',c:'F2',cls:'darkkey'},{k:'F3',c:'F3',cls:'darkkey'},{k:'F4',c:'F4',cls:'darkkey'},{k:'F5',c:'F5',cls:'darkkey'},{style:'width:20px'},{k:'F6',c:'F6',cls:'darkkey'},{k:'F7',c:'F7',cls:'darkkey'},{k:'F8',c:'F8',cls:'darkkey'},{k:'F9',c:'F9',cls:'darkkey'},{k:'F10',c:'F10',cls:'darkkey'},{style:'width:15px'},{k:'Del',c:'Delete',cls:'darkkey'},{style:'width:15px'},{k:'hide', c:'hide_keyboard',cls:'darkkey'}],
    [{k:'`',c:'Grave'}, {k:'1', sk:'!',c:'Digit1'},{k:'2', sk:'@',c:'Digit2'},{k:'3', sk:'#',c:'Digit3'},{k:'4', sk:'$', c:'Digit4'},{k:'5',sk:'%',c:'Digit5'},{k:'6', sk:'^',c:'Digit6'},{k:'7',sk:'&',c:'Digit7'},{k:'8',sk:'*',c:'Digit8'},{k:'9', sk:'(',c:'Digit9'},{k:'0', sk:')',c:'Digit0'},{k:'-', sk:'_',c:'Minus'},{k:'=',sk:'+', c:'Equal'},{k:'\\',sk:'|', c:'Backslash'},{k:'\u{2190}',c:'Backspace'}, {k:'Help',c:'Help'}, {k:'7',c:'Numpad7'},{k:'8',c:'Numpad8'},{k:'9',c:'Numpad9'} ], 
    [{k:'\u{21e5}',c:'Tab',style:"width:80px"}, {k:'Q'},{k:'W'},{k:'E'},{k:'R'},{k:'T'},{k:'Y'},{k:'U'},{k:'I'},{k:'O'},{k:'P'},{k:'[',sk:'{',c:'BracketLeft'},{k:']', sk:'}',c:'BracketRight'}, {k:'\u{21b5}',c:'Enter',style:"width:80px"},{style:'width:10px'}, {k:'4',c:'Numpad4'},{k:'5',c:'Numpad5'},{k:'6',c:'Numpad6'}], 
    [{k:'CTRL',c:'ControlLeft',style:'font-size:xx-small'},{k:'CAPS</br>LOCK', c:'CapsLock',cls:'capslock'},{k:'A'},{k:'S'},{k:'D'},{k:'F'},{k:'G'},{k:'H'},{k:'J'},{k:'K'},{k:'L'},{k:';', sk:':', c:'Semicolon'},{k:',', sk:'\"', c:'Quote'},{k:'#', sk:'^', c:'hashtag'},{style:'width:70px'}, {k:'1',c:'Numpad1'},{k:'2',c:'Numpad2'},{k:'3',c:'Numpad3'}], 
    [{k:'\u{21e7}',c:'ShiftLeft',style:"padding-right:51px"},{k:'<', sk:'>', c:'laceBrace'},{k:'Z'},{k:'X'},{k:'C'},{k:'V'},{k:'B'},{k:'N'},{k:'M'},{k:',',sk:'<',c:'Comma'},{k:'.',sk:'>',c:'Period'},{k:'/',sk:'?', c:'Slash'},{k:'\u{21e7}',c:'ShiftRight',style:'padding-right:10px'},{style:'width:10px'}, {k:'\u{2191}',c:'UP',sym:'ArrowUp'}, {style:'width:30px'}, {k:'0',c:'Numpad0', style:'padding-right:57px'},{k:'.',c:'NumpadDecimal'}],
    [{style:'width:138px'},{k:'Alt', c:'AltLeft'},{k:'A', c:'leftAmiga', cls:'amigakey'},{k:'SPACE', c:'Space', style:'width:450px'},{k:'A', c:'rightAmiga', cls:'amigakey'},{k:'Alt', c:'AltRight'},{style:'width:10px'}, {k:'\u{2190}',c:'left',sym:'ArrowLeft'},{k:'\u{2193}', c:'ArrowDown'},{k:'\u{2192}', c:'ArrowRight'}, {style:'width:15px'}, {k:'-',c:'NumpadSubtract'},{k:'Enter',c:'NumpadEnter'}]
    ];

    var the_keyBoard='';
    keymap.forEach(row => {
        the_keyBoard+='<div class="justify-content-center" style="display:flex">';
        row.forEach(keydef => {
            if(keydef.k === undefined)
            {
                var style = "";
                if(keydef.s !== undefined)
                    css = keydef.s; 
                if(keydef.style !== undefined)
                    style = keydef.style; 
                
                the_keyBoard +='<div class="'+css+'" style="'+style+'"></div>';
            }
            else
            {
                let cls = '';
                if(keydef.cls !== undefined)
                {
                    cls=keydef.cls;
                }
                if(keydef.c === undefined)
                    keydef.c = 'Key'+keydef.k;
                var css = `btn btn-secondary ml-1 mt-1 ${cls}`;
                var style = null; 
                if(keydef.css !== undefined)
                    css = keydef.css; 
                if(keydef.style !== undefined)
                    style = keydef.style; 

                let label = keydef.k;
                if(label == "hide")
                {
                    label = `<svg xmlns="http://www.w3.org/2000/svg" width="2.0em" height="2.0em" fill="currentColor" class="bi bi-pause-btn" viewBox="0 0 16 16"><path d="M14 5a1 1 0 0 1 1 1v5a1 1 0 0 1-1 1H2a1 1 0 0 1-1-1V6a1 1 0 0 1 1-1h12zM2 4a2 2 0 0 0-2 2v5a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V6a2 2 0 0 0-2-2H2z"/><path d="M13 10.25a.25.25 0 0 1 .25-.25h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5a.25.25 0 0 1-.25-.25v-.5zm0-2a.25.25 0 0 1 .25-.25h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5a.25.25 0 0 1-.25-.25v-.5zm-5 0A.25.25 0 0 1 8.25 8h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 8 8.75v-.5zm2 0a.25.25 0 0 1 .25-.25h1.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-1.5a.25.25 0 0 1-.25-.25v-.5zm1 2a.25.25 0 0 1 .25-.25h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5a.25.25 0 0 1-.25-.25v-.5zm-5-2A.25.25 0 0 1 6.25 8h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 6 8.75v-.5zm-2 0A.25.25 0 0 1 4.25 8h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 4 8.75v-.5zm-2 0A.25.25 0 0 1 2.25 8h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 2 8.75v-.5zm11-2a.25.25 0 0 1 .25-.25h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5a.25.25 0 0 1-.25-.25v-.5zm-2 0a.25.25 0 0 1 .25-.25h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5a.25.25 0 0 1-.25-.25v-.5zm-2 0A.25.25 0 0 1 9.25 6h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 9 6.75v-.5zm-2 0A.25.25 0 0 1 7.25 6h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 7 6.75v-.5zm-2 0A.25.25 0 0 1 5.25 6h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5A.25.25 0 0 1 5 6.75v-.5zm-3 0A.25.25 0 0 1 2.25 6h1.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-1.5A.25.25 0 0 1 2 6.75v-.5zm0 4a.25.25 0 0 1 .25-.25h.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-.5a.25.25 0 0 1-.25-.25v-.5zm2 0a.25.25 0 0 1 .25-.25h5.5a.25.25 0 0 1 .25.25v.5a.25.25 0 0 1-.25.25h-5.5a.25.25 0 0 1-.25-.25v-.5z"/></svg>`;
                }
                let shift_label=null;
                if(typeof keydef.sk !== 'undefined')
                {
                    shift_label=keydef.sk;
                }

                the_keyBoard +='<button type="button" id="button_'+keydef.c+'" class="'+css+'"';
                if(shift_label!= null)
                {
                    let style_composite_key=";padding-top:0;padding-bottom:0;";
                    if(style==null)
                        style=style_composite_key;
                    else
                        style+=style_composite_key;
                }
                if(style !=null)
                    the_keyBoard += ' style="'+style+'"';

                the_keyBoard+='>';
                if(shift_label==null)
                {
                    the_keyBoard += label;
                }
                else
                {
                    the_keyBoard+= `<div style="flex-direction:column"><div class="keycap_shift">${shift_label}</div><div class="keycap">${label}</div><div>`;
                }
                the_keyBoard+='</button>';
            }
        });
        the_keyBoard+='</div>';
    });
    $('#divKeyboardRows').html(the_keyBoard);

    release_modifiers=function()
    {
        let release_key = function (theModKey) {
            if(document.body.getAttribute(theModKey+'-key')=='pressed')
            {
                let c64code = translateKey(theModKey, theModKey);
                wasm_schedule_key(c64code[0], c64code[1], 0,1);
                $("#button_"+theModKey).attr("style", "");
                document.body.setAttribute(theModKey+'-key','');
            }
        }

        release_key('ControlLeft');
        release_key('leftAmiga');
        release_key('rightAmiga');
        release_key('AltLeft');
        release_key('AltRight');

        if(document.body.getAttribute('ShiftLeft-key')=='pressed')
        {
            let c64code = translateKey('ShiftLeft', 'ShiftLeft');
            
            if(document.body.getAttribute('CapsLock-key')!='pressed')
                wasm_schedule_key(c64code[0], c64code[1], 0,1);
            
            document.body.setAttribute('ShiftLeft-key', '');
            shift_pressed_count--;
            if(shift_pressed_count==0)
            {
                document.body.setAttribute('shift-keys', '');
            }
        }
        if(document.body.getAttribute('ShiftRight-key')=='pressed')
        {
            let c64code = translateKey('ShiftRight', 'ShiftRight');
            wasm_schedule_key(c64code[0], c64code[1], 0,1);
            document.body.setAttribute('ShiftRight-key', '');
            shift_pressed_count--;
            if(shift_pressed_count==0)
            {
                document.body.setAttribute('shift-keys', '');
            }
        }
    }

    keymap.forEach(row => {
        row.forEach(keydef => {
            if(keydef.k === undefined)
                return;
            if(keydef.c === undefined)
              keydef.c = 'Key'+keydef.k;

            $("#button_"+keydef.c).click(function() 
            {
               if(keydef.c == 'hide_keyboard')
               {
                    $('#virtual_keyboard').collapse('hide');
                    setTimeout( scaleVMCanvas, 500);
               }
               else if(keydef.c == 'CapsLock')
               {
                   let c64code = translateKey(keydef.c, keydef.k);
                   var c64code2 = translateKey('ShiftLeft', 'ShiftLeft');
                   if(keydef.locked === undefined || keydef.locked == 0)
                   {
                     wasm_schedule_key(c64code[0], c64code[1], 1,0);                   
                     wasm_schedule_key(c64code2[0], c64code2[1], 1,0);                   

                     keydef.locked = 1;
                     $("#button_"+keydef.c).attr("style", "background-color: var(--green) !important;"+keydef.style);
                     document.body.setAttribute('shift-keys', 'pressed');
                     shift_pressed_count++;
                     
                   }
                   else
                   {
                     wasm_schedule_key(c64code2[0], c64code2[1], 0, 0);                   
                     wasm_schedule_key(c64code[0], c64code[1], 0, 0);                   
                     keydef.locked = 0;
                     $("#button_"+keydef.c).attr("style", "");
                     shift_pressed_count--;
                     if(shift_pressed_count==0)
                     {
                        document.body.removeAttribute('shift-keys');
                     }              
                   }
               }
               else if(keydef.sym !== undefined)
               {
                   emit_string([keydef.sym],0);
               } 
               else
               {
                var c64code = translateKey(keydef.c, keydef.k);
                if(c64code !== undefined){
                    wasm_schedule_key(c64code[0], c64code[1], 1,0);

                    if(keydef.c == 'ShiftLeft' ||keydef.c == 'ShiftRight')
                    {
                        if(document.body.getAttribute(keydef.c+'-key')=='')
                        {
                            document.body.setAttribute('shift-keys', 'pressed');
                            document.body.setAttribute(keydef.c+'-key', 'pressed');
                            shift_pressed_count++;
                        }
                        else
                        {
                            document.body.setAttribute(keydef.c+'-key', '');
                            wasm_schedule_key(c64code[0], c64code[1], 0,1);
                            shift_pressed_count--;
                            if(shift_pressed_count==0)
                            {
                               document.body.setAttribute('shift-keys', '');
                            }   
                        }
                    }
/*                    else if(keydef.c == 'leftAmiga'||keydef.c == 'rightAmiga')
                    {
                        $("#button_"+keydef.c).attr("style", "background-color: var(--green) !important;"+keydef.style);
                    
                        setTimeout(() => {
                            wasm_schedule_key(c64code[0], c64code[1], 0,1);
                            $("#button_"+keydef.c).attr("style", "");
                        }, 1000*4);
                    
                    }
*/                    else if(keydef.c == 'ControlLeft' ||
                                keydef.c == 'leftAmiga'||keydef.c == 'rightAmiga' ||
                                keydef.c == 'AltLeft'||keydef.c == 'AltRight')
                    {
                        if(document.body.getAttribute(keydef.c+'-key')=='')
                        {
//                            $("#button_"+keydef.c).attr("style", "background-color: var(--blue) !important");
                            document.body.setAttribute(keydef.c+'-key', 'pressed');
                        }
                        else
                        {
                            document.body.setAttribute(keydef.c+'-key', '');
                            wasm_schedule_key(c64code[0], c64code[1], 0,1);
//                            $("#button_"+keydef.c).attr("style", "");
                        }                    
                    }
                    else
                    {  
                        release_modifiers();
                        //release the key automatically after a short time ...
                        setTimeout(() => {
                            wasm_schedule_key(c64code[0], c64code[1], 0, 1);
                        }, 100);
                    }
                }
               }
            });
        });
    });

}


