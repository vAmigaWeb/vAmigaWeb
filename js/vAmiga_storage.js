//-- indexedDB API
_db = null;
_db_init_called=false;
_db_wait_counter=0;
_db_retries=0;
const _sleep = (milliseconds) => {
  return new Promise(resolve => setTimeout(resolve, milliseconds));
}
async function db(){
  let me_called_init=false;
  //wait until _db created
  while(_db==null)
  {
    if(_db_init_called==false)
    {
      _db_init_called=true;
      me_called_init=true;

      if(_db_retries>2)
      {
        let msg=`cannot open database... are you using private/incognito browsing? To enable storage use a normal browser window...`;
        _db_retries=0;
        _db_init_called=false;
        throw new Error(msg);
      }
      console.error(`opening db ... ${_db_retries+1}.try`);
      initDB();
      _db_retries+=1;
    }
    let wait_time=100;
    await _sleep(wait_time);
    if(me_called_init)
    {
      _db_wait_counter+=wait_time;
    }
    
    if(_db_wait_counter > 1500)
    {//if opening db takes too long, retry
        _db_init_called = false;
        _db_wait_counter=0;
        _me_called_init=false;
    }
  }  
  return _db;
}


function initDB() {  
  window.addEventListener('unhandledrejection', function(event) {
    alert("Error: " + event.reason.message);
  });

  
  let openReq =  indexedDB.open('vcAmigaDB', 7);

  openReq.onupgradeneeded = function (event){
      let _db = openReq.result;
      switch (event.oldVersion) {
          case 0:
              //no db
              break;
          default:
              break;
      }
      if(!_db.objectStoreNames.contains('snapshots'))
      {
         var snapshot_store=_db.createObjectStore('snapshots', {keyPath: 'id', autoIncrement: true});
         snapshot_store.createIndex("title", "title", { unique: false });
      }
      if(!_db.objectStoreNames.contains('apps'))
      {
         var apps_store=_db.createObjectStore('apps', {keyPath: 'title'}); 
      }

      //db.deleteObjectStore('custom_buttons');
      if(!_db.objectStoreNames.contains('custom_buttons'))
      {
         //alert("create two local object stores");
         var custom_buttons_store=_db.createObjectStore('custom_buttons', {keyPath: 'title'});
      }
      if(!_db.objectStoreNames.contains('roms'))
      {
         var rom_store=_db.createObjectStore('roms', {keyPath: 'id', autoIncrement: false});
         rom_store.createIndex("type", "type", { unique: false });
      }
  };
  openReq.onerror = function() { 
    console.error("Error", openReq.error);
    _db_wait_counter=1600; //flag this wait as unsuccessful 
    /*alert('error while open db: '+openReq.error);*/
  }
  openReq.onsuccess = function() {
      _db=openReq.result;
  }  
}


async function save_snapshot(the_name, the_data) {
  //beim laden in die drop zone den Titel merken
  //dann beim take snapshot, den titel automatisch mitgeben
  //im Snapshotbrowser jeden titel als eigene row darstellen
  //erste row autosnapshot, danach kommen die Titel
  //extra user snapshot ist dann unnötig
  let the_snapshot = {
    title: the_name,
    data: the_data //,
//    created: new Date()
  };

  let tx_apps = (await db()).transaction('apps', 'readwrite');
  let req_apps = tx_apps.objectStore('apps').put({title: the_name});
  req_apps.onsuccess= function(e){ 
        console.log("wrote app with id="+e.target.result)        
  };


  let tx = (await db()).transaction('snapshots', 'readwrite');
  tx.oncomplete = function() {
    console.log("Transaction is complete");
  };
  tx.onabort = function() {
    console.log("Transaction is aborted");
  };
 
  try {
    let req = tx.objectStore('snapshots').add(the_snapshot);
    req.onsuccess= function(e){ 
        console.log("wrote snapshot with id="+e.target.result)        
    };
    req.onerror = function(e){ 
        console.error("could not write snapshot: ",  req.error) 
    };
  } catch(err) {
    if (err.name == 'ConstraintError') {
      alert("Such snapshot exists already");
    } else {
      throw err;
    }
  }
}

async function get_stored_app_titles(callback_fn)
{
  let transaction = (await db()).transaction("apps"); // readonly
  let apps = transaction.objectStore("apps");

  let request = apps.getAllKeys();

  request.onsuccess = function() {
      if (request.result !== undefined) {
          callback_fn(request.result);
      } else {
          console.log("No titles found");
      }
  };    
}
function stored_app_titles() {
  return new Promise((resolve, reject) => {
    get_stored_app_titles((result) => {
        resolve(result);
    });
  });
}

function get_snapshots_for_app_title(app_title)
{
    return new Promise(async (resolve, reject) => {
      let transaction = (await db()).transaction("snapshots"); 
      let snapshots = transaction.objectStore("snapshots");
      let titleIndex = snapshots.index("title");
      let request = titleIndex.getAll(app_title);

      request.onsuccess = function() {
          resolve(request.result);
      };
      request.onerror = function(e){ 
        console.error("could not read snapshots: ",  request.error) 
        reject(request.error);
      };
    });
}


async function get_snapshot_per_id(the_id, callback_fn)
{
    let transaction = (await db()).transaction("snapshots"); 
    let snapshots = transaction.objectStore("snapshots");
 
    let request = snapshots.get(parseInt(the_id));

    request.onsuccess = function() {
        callback_fn(request.result);
    };
}

function delete_snapshot_per_id(the_id)
{
  get_snapshot_per_id(the_id, 
  async function(the_snapshot) {
    let transaction = (await db()).transaction("snapshots", 'readwrite'); 
    let snapshots = transaction.objectStore("snapshots");
    snapshots.delete(parseInt(the_id));
    //check if this was the last snapshot of the game title 
    get_snapshots_for_app_title(0, the_snapshot.title, 
      async function (ctx, app_title, snapshot_list) {
        if(snapshot_list.length == 0)
        {//when it was the last one, then also delete the app title
          let tx_apps = (await db()).transaction("apps", 'readwrite'); 
          let apps = tx_apps.objectStore("apps");
          apps.delete(app_title);
        }
      }
    );
  });
}


//--- local storage API ---


function local_storage_get(name) {
  try{
    return localStorage.getItem(name);
  } 
  catch(e) {
    console.error(e.message);
    return null;
  }  
}
function local_storage_set(name, value) {
  try{
    localStorage.setItem(name,value);
  } 
  catch(e) {
    console.error(e.message);
  }  
}
function local_storage_remove(name) {
  try{
    localStorage.removeItem(name);
  } 
  catch(e) {
    console.error(e.message);
  }  
}




function load_setting(name, default_value) {
    var value = local_storage_get(name);
    if(value === null)
    {
        return default_value;
    } 
    else
    {
        if(value=='true')
          return true;
        else if(value=='false')
          return false;
        else
          return value;
    }
}

function save_setting(name, value) {
    if (value!= null) {
      local_storage_set(name, value);
    } else {
      local_storage_remove(name);
    }
}






//-------------- custom buttons

function save_new_empty_group(the_name) {
  save_custom_buttons_scope(the_name, []);
}


function save_custom_buttons(the_name, the_data) {
  var app_specific_data=[];
  var global_data=[];

  for(button_def_id in the_data)
  {
    var button_def=the_data[button_def_id];
    if(button_def.transient !== undefined && button_def.transient)
    {
      //don't save transient buttons
    }
    else if(button_def.app_scope)
    {
      app_specific_data.push(button_def);
    }
    else
    {
      global_data.push(button_def); 
    }
  }
  save_custom_buttons_scope(the_name, app_specific_data);
  save_custom_buttons_scope('__global_scope__', global_data);
}



async function save_custom_buttons_scope(the_name, the_data) {
  let the_custom_buttons = {
    title: the_name,
    data: the_data 
  };

  let tx_apps = (await db()).transaction('apps', 'readwrite');
  let req_apps = tx_apps.objectStore('apps').put({title: the_name});
  req_apps.onsuccess= function(e){ 
        console.log("wrote app with id="+e.target.result)        
  };


  let tx = (await db()).transaction('custom_buttons', 'readwrite');
  tx.oncomplete = function() {
    console.log("Transaction is complete");
  };
  tx.onabort = function() {
    console.log("Transaction is aborted");
  };
 
  try {
    let req = tx.objectStore('custom_buttons').put(the_custom_buttons);
    req.onsuccess= function(e){ 
        console.log("wrote custom_buttons with id="+e.target.result)        
    };
    req.onerror = function(e){ 
        console.error("could not write custom_buttons: ",  req.error) 
    };
  } catch(err) {
      throw err;
  }
}



var buttons_from_mixed_scopes = null;
function get_custom_buttons(the_app_title, callback_fn)
{
  get_custom_buttons_app_scope(the_app_title, 
    function(the_buttons) {
      buttons_from_mixed_scopes = the_buttons;

      //add globals
      get_custom_buttons_app_scope('__global_scope__', 
          function(the_global_buttons) {

              var last_id_in_appscope= buttons_from_mixed_scopes.data.length-1;
              for(gb_id in the_global_buttons.data)
              {
                var gb = the_global_buttons.data[gb_id];
                last_id_in_appscope++;
                gb.id=last_id_in_appscope;
                buttons_from_mixed_scopes.data.push(gb);                                
              }
              callback_fn(buttons_from_mixed_scopes.data);
          }
        );
      }
    );
}


async function get_custom_buttons_app_scope(the_app_title, callback_fn)
{
//  if(_db === undefined)
//    return;
  try
  {
    let transaction = (await db()).transaction("custom_buttons"); 
    let custom_buttons = transaction.objectStore("custom_buttons");

    let request = custom_buttons.get(the_app_title);

    request.onsuccess = function() {
        if(request.result !== undefined)
        {
          for(button_id in request.result.data)
          {
            var action_button = request.result.data[button_id];
            if(action_button.app_scope === undefined)
            {
              action_button.app_scope= true;
            }
            if(action_button.lang === undefined)
            {
              //migration of js: prefix to lang property, can be removed in a later version ... 
              if(action_button.script.startsWith("js:"))
              {
                action_button.script = action_button.script.substring(3);
                action_button.lang = "javascript";
              }
              else
              {
                action_button.lang = "actionscript";
              }
            }
          }

          callback_fn(request.result);
        }
        else
        {
          let empty_custom_buttons = {
              title: the_app_title,
              data: [] 
            };

          callback_fn(empty_custom_buttons);
        }
    };
  }
  catch(e)
  {
    console.error(e.message);
    let empty_custom_buttons = {
              title: the_app_title,
              data: [] 
            };
    callback_fn(empty_custom_buttons);
  }
}


async function get_stored_groups(callback_fn)
{
  let transaction = (await db()).transaction("custom_buttons"); // readonly
  let custom_buttons = transaction.objectStore("custom_buttons");

  let request = custom_buttons.getAllKeys();

  request.onsuccess = function() {
      if (request.result !== undefined) {
          callback_fn(request.result);
      } else {
          console.log("No titles found");
      }
  };    
}
function stored_groups() {
  return new Promise((resolve, reject) => {
    get_stored_groups((result) => {
        resolve(result);
    });
  });
}

async function delete_button_group(key)
{
  let tx_apps = (await db()).transaction("custom_buttons", 'readwrite'); 
  let custom_buttons = tx_apps.objectStore("custom_buttons");
  custom_buttons.delete(key);
}

//--- roms
async function save_rom(the_name, extension_or_rom, the_data) {
  let the_rom = {
    id: the_name,
    type: extension_or_rom,
    data: the_data //,
//    created: new Date()
  };

  let tx = (await db()).transaction('roms', 'readwrite');
  tx.oncomplete = function() {
    console.log("Transaction is complete");
  };
  tx.onabort = function() {
    console.log("Transaction is aborted");
  };
 
  try {
    let req = tx.objectStore('roms').put(the_rom);
    req.onsuccess= function(e){ 
        console.log("wrote rom with id="+e.target.result)        
    };
    req.onerror = function(e){ 
        console.error("could not write rom: ",  req.error) 
    };
  } catch(err) {
    if (err.name == 'ConstraintError') {
      alert("Such rom exists already");
    } else {
      throw err;
    }
  }
}

async function load_rom(the_id)
{
  return new Promise(async (resolve, reject) => {
    try
    {
      let transaction = (await db()).transaction("roms"); 
      let roms = transaction.objectStore("roms");
  
      let request = roms.get(the_id);

      request.onsuccess = function() {
          resolve(request.result);
      };
      request.onerror = function(e){ 
        console.error("could not read roms: ",  request.error) 
        reject(request.error);
      };
    }
    catch(e)
    {
      console.error("load_rom: "+e.message);
      resolve(null);
    }
  });
}

async function delete_rom(the_id)
{
  return new Promise(async (resolve, reject) => {

    let transaction = (await db()).transaction("roms",'readwrite'); 
    let roms = transaction.objectStore("roms");
 
    let request = roms.delete(the_id);

    request.onsuccess = function() {
        resolve(request.result);
    };
    request.onerror = function(e){ 
      console.error("could not read roms: ",  request.error) 
      reject(request.error);
    };
  });
}


function list_rom_type_entries(rom_type)
{
    return new Promise(async (resolve, reject) => {
      try{
        let transaction = (await db()).transaction("roms"); 
        let roms = transaction.objectStore("roms");
        let typeIndex = roms.index("type");
        let request = typeIndex.getAll(rom_type);
        request.onsuccess = function() {
            resolve(request.result);
        };
        request.onerror = function(e){ 
          console.error("could not read snapshots: ",  request.error) 
          reject(request.error);
        };
      }
      catch(e)
      {
        console.error(e.message);
      }
    });
}
