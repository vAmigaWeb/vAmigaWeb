function translateKey(keycode, key)
{
    console.log('keycode='+keycode + ', key='+key);
    var mapindex;
    var sym_key = symbolic_map[key.toLowerCase()];
    var c64code;
    if(sym_key!== undefined) // && !Array.isArray(sym_key))
    {//if there is a symbolic mapping ... use it instead of the positional mapping
        var raw_key_with_modifier=create_key_composition(sym_key);
        c64code=raw_key_with_modifier.raw_key;
    } 
    else
    {//when there is no symbolic mapping fall back to positional mapping
        mapindex=key_translation_map[ keycode ];
        c64code=[mapindex,0];
    }
    return c64code;
}

function isUpperCase(s){
    return s.toUpperCase() == s && s.toLowerCase() != s;
}

function translateKey2(keycode, key, use_positional_mapping=false)
{
    console.log('keycode='+keycode + ', key='+key);
    if(keycode == 'Space' && key == '^')
    {//fix for windows system
        key=' ';
    }

    let mapindex;
    let raw_key_with_modifier = { modifier: null,  raw_key: undefined }

    if(use_positional_mapping)
    {
        mapindex=key_translation_map[ keycode ];
        raw_key_with_modifier.raw_key = [mapindex,0] //c64keymap[mapindex];
    }
    else
    {
        let sym_key = symbolic_map[key];
        if(sym_key === undefined && isUpperCase(key))
        {//get the lowercase variant and press shift
            sym_key = symbolic_map[key.toLowerCase()];
            if(!Array.isArray(sym_key))
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
            mapindex=key_translation_map[ keycode ];
            raw_key_with_modifier.raw_key = [mapindex,0];  //c64keymap[mapindex];
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
    F2: ['ShiftLeft','F1'],
    F3: 'F3',
    F4: ['ShiftLeft','F3'],
    F5: 'F5',
    F6: ['ShiftLeft','F5'],
    F7: 'F7',
    F8: ['ShiftLeft','F7'],
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
    'ArrowLeft': ['ShiftLeft','ArrowRight'],
    'ArrowUp': ['ShiftLeft','ArrowDown'],
    '\n': 'Enter',
    'Dead': 'upArrow', '^': 'upArrow' //^
}

key_translation_map =  
        {//https://github.com/dirkwhoffmann/vAmiga/blob/164c04d75f0ae739dd9f2ff2c28520db05e7c047/GUI/Peripherals/AmigaKey.swift

    // First row
    Backspace:0x41,
    Enter:0x44,
    ArrowLeft:0x4F,
    ArrowRight:0x4E,
    F7:0x56,
    F8:0x57,
    F1:0x50,
    F2:0x51,
    F3:0x52,
    F4:0x53,
    F5:0x54,
    F6:0x55,
    ArrowUp:0x4C,
    ArrowDown:0x4D,
    
    // Second row
    Digit3: 3,
    KeyW:0x11,
    KeyA:0x20,
    Digit4:4,
    KeyZ:0x31,
    KeyS:0x21,
    KeyE:0x12,
    ShiftRight:0x61,
    ShiftLeft:0x60,
    
    // Third row
    Digit5:5,
    KeyR     :0x13,
    KeyD     :0x22,
    Digit6:6,
    KeyC     :0x33,
    KeyF     :0x23,
    KeyT     :0x14,
    KeyX     :0x32,
    
    // Fourth row
    Digit7:7,
    KeyY     :0x15,
    KeyG     :0x24,
    Digit8:8,
    KeyB     :0x35,
    KeyH     :0x25,
    KeyU     :0x16,
    KeyV     :0x34,
    
    // Fifth row
    Digit9:9,
    KeyI     :0x17,
    KeyJ     :0x26,
    Digit0:10,
    KeyM     :0x37,
    KeyK     :0x27,
    KeyO     :0x18,
    KeyN     :0x36,
    
    // Sixth row
    Minus:11,  //plus
    KeyP     :0x19,
    KeyL     :0x28,
    Equal :12, //minus
    Period:59, 
    Semicolon :44, //colon
    BracketLeft :28, //@
    Comma :58,
    
    // Seventh row
    pound :13,
    BracketRight:29, //asterisk
    Quote:45,  //semicolon
    home  :14,
    rightShift:61,
    Backslash :46, //equal
    upArrow :30,
    Slash :60,

    // Eights row
    Digit1:1,
    Delete :0x46,   //left arrow
    ControlLeft   :17,
    Digit2:2,
    Space :0x40,
    commodore :49,  //commodore
    commodore :49,  //commodore
    KeyQ     :18,
    runStop   :33,
    
    // Restore key
    restore   :31

}        






function installKeyboard() {
    keymap= [ 
    [{k:'hide', c:'hide_keyboard'},{style:'width:30px'},{k:'!',c:'exclama',sym:'!'},{k:'"',c:'quot',sym:'"'},{k:'#',c:'hash',sym:'#'},{k:'$',c:'dollar',sym:'$'},{style:'width:180px'},{k:'F1',c:'F1'}, {k:'F2',c:'F2'},{k:'F3',c:'F3'},{k:'F4',c:'F4'},{k:'F5',c:'F5'},{k:'F6',c:'F6'},{k:'F7',c:'F7'},{k:'F8',c:'F8'}],
    [{k:'<-',c:'Delete'}, {k:'1',c:'Digit1'},{k:'2',c:'Digit2'},{k:'3',c:'Digit3'},{k:'4',c:'Digit4'},{k:'5',c:'Digit5'},{k:'6',c:'Digit6'},{k:'7',c:'Digit7'},{k:'8',c:'Digit8'},{k:'9',c:'Digit9'},{k:'0',c:'Digit0'},{k:'+', c:'Minus'},{k:'-', c:'Equal'},{k:'€', c:'pound'},{k:'CLR/Home', c:'home'},{k:'Inst/DEL',c:'Backspace'} ], 
    [{k:'CTRL',c:'ControlLeft'}, {k:'Q'},{k:'W'},{k:'E'},{k:'R'},{k:'T'},{k:'Y'},{k:'U'},{k:'I'},{k:'O'},{k:'P'},{k:'@',c:'BracketLeft'},{k:'*', c:'BracketRight'},{k:'up',c:'upArrow'},{k:'RESTORE', c:'restore'}], 
    [{k:'RunStop',c:'runStop'},{k:'ShftLock', c:'shiftlock'},{k:'A'},{k:'S'},{k:'D'},{k:'F'},{k:'G'},{k:'H'},{k:'J'},{k:'K'},{k:'L'},{k:':', c:'Semicolon'},{k:';', c:'Quote'},{k:'=', c:'Backslash'},{k:'RETURN',c:'Enter'}], 
    [{k:'C=', c:'commodore'},{k:'SHIFT',c:'ShiftLeft'},{k:'Z'},{k:'X'},{k:'C'},{k:'V'},{k:'B'},{k:'N'},{k:'M'},{k:',',c:'Comma'},{k:'.',c:'Period'},{k:'/', c:'Slash'},{k:'SHIFT',c:'rightShift',style:"padding:0"}, {k:'UP',c:'UP',sym:'ArrowUp'}],
    [{style:'width:138px'},{k:'SPACE', c:'Space', style:'width:450px'},{style:'width:60px'}, {k:'LEFT',c:'left',sym:'ArrowLeft'},{k:'DOWN', c:'ArrowDown'},{k:'RIGHT', c:'ArrowRight'}]
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
                if(keydef.c === undefined)
                    keydef.c = 'Key'+keydef.k;
                var css = "btn btn-secondary ml-1 mt-1";
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

                the_keyBoard +='<button type="button" id="button_'+keydef.c+'" class="'+css+'"';
                if(style !=null)
                    the_keyBoard += ' style="'+style+'"';
                the_keyBoard += '>'+label+'</button>'
            }
        });
        the_keyBoard+='</div>';
    });
    $('#divKeyboardRows').html(the_keyBoard);

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
               else if(keydef.c == 'shiftlock')
               {
                   var c64code = translateKey('ShiftLeft', 'ShiftLeft');
                   if(keydef.locked === undefined || keydef.locked == 0)
                   {
                     wasm_schedule_key(c64code[0], c64code[1], 1,0);                   
                     keydef.locked = 1;
                     $("#button_"+keydef.c).attr("style", "background-color: var(--green) !important");
                   }
                   else
                   {
                     wasm_schedule_key(c64code[0], c64code[1], 0, 0);                   
                     keydef.locked = 0;
                     $("#button_"+keydef.c).attr("style", "");
                   
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

                    if(keydef.c == 'ShiftLeft' ||keydef.c == 'rightShift')
                    {
                        $("#button_"+keydef.c).attr("style", "background-color: var(--green) !important");
                    
                        setTimeout(() => {
                            wasm_schedule_key(c64code[0], c64code[1], 0,1);
                            $("#button_"+keydef.c).attr("style", "");
                        }, 1000*4);
                    }
                    else if(keydef.c == 'commodore')
                    {
                        $("#button_"+keydef.c).attr("style", "background-color: var(--blue) !important");
                    
                        setTimeout(() => {
                            wasm_schedule_key(c64code[0], c64code[1], 0,1);
                            $("#button_"+keydef.c).attr("style", "");
                        }, 1000*4);
                    
                    }
                    else if(keydef.c == 'ControlLeft')
                    {
                        $("#button_"+keydef.c).attr("style", "background-color: var(--blue) !important");
                    
                        setTimeout(() => {
                            wasm_schedule_key(c64code[0], c64code[1], 0,1);
                            $("#button_"+keydef.c).attr("style", "");
                        }, 1000*4);
                    
                    }
                    else if(keydef.c == 'runStop')
                    {
                        $("#button_"+keydef.c).attr("style", "background-color: var(--red) !important");
                    
                        setTimeout(() => {
                            wasm_schedule_key(c64code[0], c64code[1], 0, 1);
                            $("#button_"+keydef.c).attr("style", "");
                        }, 1000*4);
                    
                    }
                    else
                    {  //release the key automatically after a short time ...
                        //setTimeout(() => {
                        wasm_schedule_key(c64code[0], c64code[1], 0, 1);
                        //}, 100);
                    }
                }
               }
            });
        });
    });


}


