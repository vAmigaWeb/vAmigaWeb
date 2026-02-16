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

function installKeyboard(layout=true) {
    let intl_layout = layout;
    keymap= 
    intl_layout?
    [ 
    [{g: 6, k:'Esc', c:'Escape',cls:'darkkey',s:3},{g:1},{g:5,k:'F1',c:'F1', cls:'darkkey',s:2}, {g:5,k:'F2',c:'F2',cls:'darkkey',s:2},{g:5,k:'F3',c:'F3',cls:'darkkey',s:2},{g:5,k:'F4',c:'F4',cls:'darkkey',s:2},{g:5,k:'F5',c:'F5',cls:'darkkey',s:2},{g:2},{g:5,k:'F6',c:'F6',cls:'darkkey',s:2},{g:5,k:'F7',c:'F7',cls:'darkkey',s:2},{g:5,k:'F8',c:'F8',cls:'darkkey',s:2},{g:5,k:'F9',c:'F9',cls:'darkkey',s:2},{g:5,k:'F10',c:'F10',cls:'darkkey',s:2},{g: 1},{g:6,k:'Del',c:'Delete',cls:'darkkey',s:3},{g:3},{k:'globe', c:'lang',cls:'darkkey',s:3},{g:6,k:'hide', c:'hide_keyboard',cls:'darkkey',s:3}],
    [{k:'`',c:'Grave'}, {k:'1', sk:'!',c:'Digit1'},{k:'2', sk:'@',c:'Digit2'},{k:'3', sk:'#',c:'Digit3'},{k:'4', sk:'$', c:'Digit4'},{k:'5',sk:'%',c:'Digit5'},{k:'6', sk:'^',c:'Digit6'},{k:'7',sk:'&',c:'Digit7'},{k:'8',sk:'*',c:'Digit8'},{k:'9', sk:'(',c:'Digit9'},{k:'0', sk:')',c:'Digit0'},{k:'-', sk:'_',c:'Minus'},{k:'=',sk:'+', c:'Equal'},{k:'\\',sk:'|', c:'Backslash'},{k:'\u{2190}',c:'Backspace',s:3,g:5}, {g:5,k:'Help',c:'Help',s:2},{g:1}, {k:'7',c:'Numpad7'},{k:'8',c:'Numpad8'},{k:'9',c:'Numpad9'} ], 
    [{k:'\u{21e5}',c:'Tab',g:6,s:2}, {k:'Q'},{k:'W'},{k:'E'},{k:'R'},{k:'T'},{k:'Y'},{k:'U'},{k:'I'},{k:'O'},{k:'P'},{k:'[',sk:'{',c:'BracketLeft'},{k:']', sk:'}',c:'BracketRight'},{k:'', c:'EnterExt', c_use:'Enter',mapto:['Enter','EnterExt2'],g:2, style:'width: calc(135%)',s:2}, {k:'\u{21b5}',c:'Enter',g:4,s:2, style:'height:calc(135%)', mapto:['EnterExt','EnterExt2']},{g:7}, {k:'4',c:'Numpad4'},{k:'5',c:'Numpad5'},{k:'6',c:'Numpad6'}], 
    [{k:'CTRL',c:'ControlLeft',cls:'ControlLeft',s:2},{k:'CAPS</br>LOCK', c:'CapsLock',cls:'capslock',s:2},{k:'A'},{k:'S'},{k:'D'},{k:'F'},{k:'G'},{k:'H'},{k:'J'},{k:'K'},{k:'L'},{k:';', sk:':', c:'Semicolon'},{k:',', sk:'\"', c:'Quote'},{k:'#', sk:'^', c:'hashtag'}, {k:'',c:'EnterExt2',g:4,s:2, mapto:['Enter','EnterExt']},{g:7},{k:'1',c:'Numpad1'},{k:'2',c:'Numpad2'},{k:'3',c:'Numpad3'}], 
    [{k:'\u{21e7}',c:'ShiftLeft',g:6,s:2},{k:'<', sk:'>', c:'laceBrace'},{k:'Z'},{k:'X'},{k:'C'},{k:'V'},{k:'B'},{k:'N'},{k:'M'},{k:',',sk:'<',c:'Comma'},{k:'.',sk:'>',c:'Period'},{k:'/',sk:'?', c:'Slash'},{k:'\u{21e7}',c:'ShiftRight',g:8,s:2},{k:'\u{2191}',c:'ArrowUp',s:2},{g:5},{k:'0',c:'Numpad0',g:8},{k:'.',c:'NumpadDecimal'}],
    [{},{k:'Alt', c:'AltLeft',s:2},{k:'A', c:'leftAmiga', cls:'amigakey',s:2},{k:' ', c:'Space',s:2, g:32},{k:'A', c:'rightAmiga', cls:'amigakey',s:2},{k:'Alt', c:'AltRight',s:2},{g:2}, {k:'\u{2190}',c:'ArrowLeft',s:2},{k:'\u{2193}', c:'ArrowDown',s:2},{k:'\u{2192}', c:'ArrowRight',s:2}, {g:1}, {k:'-',c:'NumpadSubtract'},{k:'Enter',c:'NumpadEnter',s:2,g:8}]
    ]
    :
    [ 
    [{g: 6, k:'Esc', c:'Escape',cls:'darkkey',s:3},{g:1},{g:5,k:'F1',c:'F1', cls:'darkkey',s:2}, {g:5,k:'F2',c:'F2',cls:'darkkey',s:2},{g:5,k:'F3',c:'F3',cls:'darkkey',s:2},{g:5,k:'F4',c:'F4',cls:'darkkey',s:2},{g:5,k:'F5',c:'F5',cls:'darkkey',s:2},{g:2},{g:5,k:'F6',c:'F6',cls:'darkkey',s:2},{g:5,k:'F7',c:'F7',cls:'darkkey',s:2},{g:5,k:'F8',c:'F8',cls:'darkkey',s:2},{g:5,k:'F9',c:'F9',cls:'darkkey',s:2},{g:5,k:'F10',c:'F10',cls:'darkkey',s:2},{g: 1},{g:6,k:'Del',c:'Delete',cls:'darkkey',s:3},{g:3},{k:'globe', c:'lang',cls:'darkkey',s:3},{g:6,k:'hide', c:'hide_keyboard',cls:'darkkey',s:3}],
    [{k:'`',c:'Grave'}, {k:'1', sk:'!',c:'Digit1'},{k:'2', sk:'@',c:'Digit2'},{k:'3', sk:'#',c:'Digit3'},{k:'4', sk:'$', c:'Digit4'},{k:'5',sk:'%',c:'Digit5'},{k:'6', sk:'^',c:'Digit6'},{k:'7',sk:'&',c:'Digit7'},{k:'8',sk:'*',c:'Digit8'},{k:'9', sk:'(',c:'Digit9'},{k:'0', sk:')',c:'Digit0'},{k:'-', sk:'_',c:'Minus'},{k:'=',sk:'+', c:'Equal'},{k:'\\',sk:'|', c:'Backslash'},{k:'\u{2190}',c:'Backspace',s:3,g:5}, {g:5,k:'Help',c:'Help',s:2},{g:1}, {k:'7',c:'Numpad7'},{k:'8',c:'Numpad8'},{k:'9',c:'Numpad9'} ], 
    [{k:'\u{21e5}',c:'Tab',g:6,s:2}, {k:'Q'},{k:'W'},{k:'E'},{k:'R'},{k:'T'},{k:'Y'},{k:'U'},{k:'I'},{k:'O'},{k:'P'},{k:'[',sk:'{',c:'BracketLeft'},{k:']', sk:'}',c:'BracketRight'},{k:'',c:'Enter',mapto:['EnterExt'],style:'height:calc(118%)',g:5,s:2},{g:8}, {k:'4',c:'Numpad4'},{k:'5',c:'Numpad5'},{k:'6',c:'Numpad6'}], 
    [{k:'CTRL',c:'ControlLeft',cls:'ControlLeft',s:2},{k:'CAPS</br>LOCK', c:'CapsLock',cls:'capslock',s:2},{k:'A'},{k:'S'},{k:'D'},{k:'F'},{k:'G'},{k:'H'},{k:'J'},{k:'K'},{k:'L'},{k:';', sk:':', c:'Semicolon'},{k:',', sk:'\"', c:'Quote'},{k:'&nbsp;&nbsp;&nbsp;\u{21b5}', c:'EnterExt', c_use:'Enter',mapto:['Enter'],g:7,s:2},{g:8}, {k:'1',c:'Numpad1'},{k:'2',c:'Numpad2'},{k:'3',c:'Numpad3'}], 
    [{k:'\u{21e7}',c:'ShiftLeft',g:10,s:2},{k:'Z'},{k:'X'},{k:'C'},{k:'V'},{k:'B'},{k:'N'},{k:'M'},{k:',',sk:'<',c:'Comma'},{k:'.',sk:'>',c:'Period'},{k:'/',sk:'?', c:'Slash'},{k:'\u{21e7}',c:'ShiftRight',g:8,s:2},{k:'\u{2191}',c:'ArrowUp',s:2},{g:5},{k:'0',c:'Numpad0',g:8},{k:'.',c:'NumpadDecimal'}],
    [{},{k:'Alt', c:'AltLeft',s:2},{k:'A', c:'leftAmiga', cls:'amigakey',s:2},{k:' ', c:'Space',s:2, g:32},{k:'A', c:'rightAmiga', cls:'amigakey',s:2},{k:'Alt', c:'AltRight',s:2},{g:2}, {k:'\u{2190}',c:'ArrowLeft',s:2},{k:'\u{2193}', c:'ArrowDown',s:2},{k:'\u{2192}', c:'ArrowRight',s:2}, {g:1}, {k:'-',c:'NumpadSubtract'},{k:'Enter',c:'NumpadEnter',s:2,g:8}]
    ]
    ;

    let  renderKey  = (keydef,cellpos) => {

        let pos_label='';//`<div style="position: absolute;font-size: xx-small;color: yellowgreen;">${cellpos}:${ keydef.g===undefined?4:keydef.g}</div>`;

        if(keydef.k === undefined)
        {
            var style = "";
            if(keydef.s !== undefined)
                css = keydef.s; 
            if(keydef.style !== undefined)
                style += keydef.style; 
            style+=`grid-column: auto / span ${keydef.g===undefined?4:keydef.g}`;
            the_keyBoard +=`<div class="${css}" style="${style}">${pos_label}</div>`;
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
            var css = `keycap_base ${cls}`;
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
            else if(label == "globe")
            {
                label = `
                <svg xmlns="http://www.w3.org/2000/svg" width="1.15em" height="1.15em" fill="currentColor" class="bi bi-globe" viewBox="0 0 16 16">
  <path d="M0 8a8 8 0 1 1 16 0A8 8 0 0 1 0 8m7.5-6.923c-.67.204-1.335.82-1.887 1.855A8 8 0 0 0 5.145 4H7.5zM4.09 4a9.3 9.3 0 0 1 .64-1.539 7 7 0 0 1 .597-.933A7.03 7.03 0 0 0 2.255 4zm-.582 3.5c.03-.877.138-1.718.312-2.5H1.674a7 7 0 0 0-.656 2.5zM4.847 5a12.5 12.5 0 0 0-.338 2.5H7.5V5zM8.5 5v2.5h2.99a12.5 12.5 0 0 0-.337-2.5zM4.51 8.5a12.5 12.5 0 0 0 .337 2.5H7.5V8.5zm3.99 0V11h2.653c.187-.765.306-1.608.338-2.5zM5.145 12q.208.58.468 1.068c.552 1.035 1.218 1.65 1.887 1.855V12zm.182 2.472a7 7 0 0 1-.597-.933A9.3 9.3 0 0 1 4.09 12H2.255a7 7 0 0 0 3.072 2.472M3.82 11a13.7 13.7 0 0 1-.312-2.5h-2.49c.062.89.291 1.733.656 2.5zm6.853 3.472A7 7 0 0 0 13.745 12H11.91a9.3 9.3 0 0 1-.64 1.539 7 7 0 0 1-.597.933M8.5 12v2.923c.67-.204 1.335-.82 1.887-1.855q.26-.487.468-1.068zm3.68-1h2.146c.365-.767.594-1.61.656-2.5h-2.49a13.7 13.7 0 0 1-.312 2.5m2.802-3.5a7 7 0 0 0-.656-2.5H12.18c.174.782.282 1.623.312 2.5zM11.27 2.461c.247.464.462.98.64 1.539h1.835a7 7 0 0 0-3.072-2.472c.218.284.418.598.597.933M10.855 4a8 8 0 0 0-.468-1.068C9.835 1.897 9.17 1.282 8.5 1.077V4z"/>
</svg>`;
            }

            let shift_label=null;
            if(typeof keydef.sk !== 'undefined')
            {
                shift_label=keydef.sk;
            }

            let gap = 4;
            if(keydef.g !== undefined)
            {
                gap=keydef.g;
            }     
            css+=` g-${gap}`;

            the_keyBoard +=`<div id="button_${keydef.c}" class="${css}"`;
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

            the_keyBoard+='>'+pos_label;
            if(shift_label==null)
            {
                the_keyBoard += label;
            }
            else
            {
                the_keyBoard+= `<div style="flex-direction:column"><div class="keycap_shift">${shift_label}</div><div class="keycap">${label}</div></div>`;
            }
            the_keyBoard+='</div>';
        }
    };

    let columns=0; 
    let cellpos=0;
    //calculate number of grid columns
    for(let row of keymap)
    {
        cellpos=0;
        for(let keydef of row)
        {
            cellpos+=(keydef.g === undefined) ? 4:keydef.g;
        }
        if(cellpos>columns)
        {
            columns = cellpos;
        }
    }
    document.querySelector(':root').style.setProperty('--keycap_columns', columns);

    var the_keyBoard=`
<div id="keyboard_grid"
draggable="false">
    `;
    for(let row of keymap)
    {
            cellpos=0;
            for(let keydef of row)
            {
                let gap = 4;
                if(keydef.g !== undefined)
                {
                    gap=keydef.g;
                }     
                renderKey(keydef,cellpos);
                cellpos+=gap;
         }
         if(cellpos<columns)
         {
            renderKey({g:columns-cellpos},cellpos);
         }
    }
    the_keyBoard+='</div>';
    $('#divKeyboardRows').html(the_keyBoard);

    release_modifiers=function()
    {
        let release_key = function (theModKey) {
            if(document.body.getAttribute(theModKey+'-key')=='pressed')
            {
                let key_code = translateKey(theModKey, theModKey);
                wasm_schedule_key(key_code[0], key_code[1], 0,1);
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
            let key_code = translateKey('ShiftLeft', 'ShiftLeft');
            
            if(document.body.getAttribute('CapsLock-key')!='pressed')
                wasm_schedule_key(key_code[0], key_code[1], 0,1);
            
            document.body.setAttribute('ShiftLeft-key', '');
            shift_pressed_count--;
            if(shift_pressed_count==0)
            {
                document.body.setAttribute('shift-keys', '');
            }
        }
        if(document.body.getAttribute('ShiftRight-key')=='pressed')
        {
            let key_code = translateKey('ShiftRight', 'ShiftRight');
            wasm_schedule_key(key_code[0], key_code[1], 0,1);
            document.body.setAttribute('ShiftRight-key', '');
            shift_pressed_count--;
            if(shift_pressed_count==0)
            {
                document.body.setAttribute('shift-keys', '');
            }
        }
    }

    let virtual_keyboard = document.getElementById("virtual_keyboard");
    virtual_keyboard.addEventListener("contextmenu", (event)=>{event.preventDefault();});
    virtual_keyboard.addEventListener("dragstart", (event)=>{event.preventDefault();});
    virtual_keyboard.addEventListener("drop", (event)=>{event.preventDefault();});
    virtual_keyboard.addEventListener("select", (event)=>{event.preventDefault();});

    $('#virtual_keyboard').css("user-select","none");

    //stop handled touch events on and between keys being propagated
    //touch events left/right area of virtual keyboard is still propagated
    document.getElementById("divKeyboardRows").addEventListener("touchstart",e=>e.stopPropagation());



    let  add_key_event_listener = keydef => {
        if(keydef.k === undefined)
        return;
        if(keydef.c === undefined)
        keydef.c = 'Key'+keydef.k;

        let the_key_element=document.getElementById("button_"+keydef.c);
        
        let key_down_handler=function(event) 
        {
            if(keyboard_sound_volume>0)
            {
                if(keydef.s === undefined)
                    play_sound(audio_key_standard,keyboard_sound_volume);
                else if(keydef.s == 2)
                    play_sound(audio_key_space,keyboard_sound_volume);
                else if(keydef.s == 3)
                    play_sound(audio_key_backspace,keyboard_sound_volume);
            }
            if(key_haptic_feedback)
            {
                navigator.vibrate(15);
            }

            if(keydef.c == 'hide_keyboard')
            {
                    $('#virtual_keyboard').collapse('hide');
                    setTimeout( scaleVMCanvas, 500);
            }
            else if(keydef.c == 'lang')
            {
                installKeyboard(!intl_layout);
                save_setting('keyboard_layout', !intl_layout);
            }
            else if(keydef.c == 'CapsLock')
            {
                let key_code = translateKey(keydef.c, keydef.k);
                var key_code2 = translateKey('ShiftLeft', 'ShiftLeft');
                if(keydef.locked === undefined || keydef.locked == 0)
                {
                    wasm_schedule_key(key_code[0], key_code[1], 1,0);                   
                    wasm_schedule_key(key_code2[0], key_code2[1], 1,0);                   

                    keydef.locked = 1;
                    $("#button_"+keydef.c).attr("style", "background-color: var(--green) !important;"+keydef.style);
                    document.body.setAttribute('shift-keys', 'pressed');
                    shift_pressed_count++;
                    
                }
                else
                {
                    wasm_schedule_key(key_code2[0], key_code2[1], 0, 0);                   
                    wasm_schedule_key(key_code[0], key_code[1], 0, 0);                   
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
                var key_code = translateKey(keydef.c_use ?keydef.c_use: keydef.c, keydef.k);
                if(key_code !== undefined){
                    wasm_schedule_key(key_code[0], key_code[1], 1,0);

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
                            wasm_schedule_key(key_code[0], key_code[1], 0,1);
                            shift_pressed_count--;
                            if(shift_pressed_count==0)
                            {
                                //TODO: smart und bei iOS longpress kommt er hier rein!
                                //TODO: event ausloggen
                                document.body.setAttribute('shift-keys', '');
                            }   
                        }
                    }
                    else if(keydef.c == 'ControlLeft' ||
                                keydef.c == 'leftAmiga'||keydef.c == 'rightAmiga' ||
                                keydef.c == 'AltLeft'||keydef.c == 'AltRight')
                    {
                        if(document.body.getAttribute(keydef.c+'-key')=='')
                        {
                            document.body.setAttribute(keydef.c+'-key', 'pressed');
                        }
                        else
                        {
                            document.body.setAttribute(keydef.c+'-key', '');
                            wasm_schedule_key(key_code[0], key_code[1], 0,1);
                        }                    
                    }
                    else
                    {
                        if(keydef.mapto)
                        {
                            for(let mapto of keydef.mapto)
                            {
                                document.getElementById(`button_${mapto}`).setAttribute('key-state', 'pressed');
                            }                        
                        }

                        the_key_element.setAttribute('key-state', 'pressed');
                    }
                }
            }
        }
        let key_up_handler=function() 
        {
            if( keydef.c == 'CapsLock' ||
                keydef.c == 'ShiftLeft' || keydef.c == 'ShiftRight' ||
                keydef.c == 'ControlLeft' ||
                keydef.c == 'leftAmiga'||keydef.c == 'rightAmiga' ||
                keydef.c == 'AltLeft'||keydef.c == 'AltRight'
            )
            {}
            else
            {
                let key_code = translateKey(keydef.c_use ?keydef.c_use: keydef.c, keydef.k);
                wasm_schedule_key(key_code[0], key_code[1], 0, 1);
                release_modifiers();
                the_key_element.setAttribute('key-state', '');
                
                if(keydef.mapto)
                {
                    for(let mapto of keydef.mapto)
                    {
                        document.getElementById(`button_${mapto}`).setAttribute('key-state', '');
                    }                        
                }
            }
        }
        the_key_element.addEventListener("focus", (event)=>{ event.preventDefault(); event.currentTarget.blur();})
        the_key_element.addEventListener("pointerdown", (event)=>{
            if(current_vbk_touch.startsWith("smart"))
                return;
            the_key_element.setPointerCapture(event.pointerId);
            key_down_handler(event);
        });
        the_key_element.addEventListener("pointerup", (event)=>{
            if(current_vbk_touch.startsWith("smart"))
            {
                key_down_handler(event);
                setTimeout(key_up_handler,100);
            }
        });
        the_key_element.addEventListener("lostpointercapture", (event)=>{
                key_up_handler(event);
        });


        the_key_element.addEventListener("touchstart", (event)=>{
            if(current_vbk_touch.startsWith("exact") || current_vbk_touch.startsWith("mix"))
            {
                event.preventDefault();
            }
            if(current_vbk_touch.startsWith("mix"))
            {
                let scroll_area=document.getElementById("vbk_scroll_area");
                touch_start_x=event.changedTouches[0].clientX;
                touch_start_scrollLeft=scroll_area.scrollLeft;
                touch_start_id=event.changedTouches[0].identifier;
            }
        });
        the_key_element.addEventListener("touchmove", (event)=>{
            if(current_vbk_touch.startsWith("mix"))
            {
                let scroll_area=document.getElementById("vbk_scroll_area");
                for(touch of event.changedTouches)
                {
                    if(touch.identifier == touch_start_id)
                    {
                        let scroll_x = touch_start_scrollLeft+(touch_start_x-touch.clientX);
                        scroll_area.scroll(scroll_x, 0);
                    }
                }
            } 
        });
        the_key_element.addEventListener("touchend", (event)=>{
            event.preventDefault();
        });

    }



    keymap.forEach(row => {
        row.forEach(keydef => {
            if(Array.isArray(keydef))
            {
                for(let key of keydef)
                {
                    add_key_event_listener(key);
                }
            }
            else
            {
                add_key_event_listener(keydef);
            }
        });
    });

}


