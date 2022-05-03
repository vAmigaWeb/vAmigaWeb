const cache_name = 'vAmiga_app_cache_v2022_05_03b';

// install event
self.addEventListener('install', evt => {
  console.log('service worker installed');
});

// activate event
self.addEventListener('activate', evt => {
  console.log('service worker activated');
  evt.waitUntil(
    caches.keys().then(keys => {
      console.log('deleting cache files:'+keys);
      return Promise.all(keys
        .map(key => caches.delete(key))
      );
    })
  );
});

self.addEventListener('fetch', function(event){
  event.respondWith(async function () {
      //is this url one that should not be cached at all ? 
      if(
        event.request.url.startsWith('https://csdb.dk/webservice/') && 
        !event.request.url.endsWith('cache_me=true')
        ||
        event.request.url.startsWith('https://mega65.github.io/')
        ||
        event.request.url.toLowerCase().startsWith('https://vamigaweb.github.io/doc')
        ||
        event.request.url.toLowerCase().endsWith('vamigaweb_player.js')
	||
        event.request.url.endsWith('run.html')
	||
        event.request.url.endsWith('cache_me=false')
      )
      {
        console.log('sw: do not cache fetched resource: '+event.request.url);
        return fetch(event.request);
      }
      else
      {
        //try to get it from the cache
        var cache = await caches.open(cache_name);
        var cachedResponsePromise = await cache.match(event.request);
        if(cachedResponsePromise)
        {
          console.log('sw: get from '+cache_name+' cached resource: '+event.request.url);
          return cachedResponsePromise;
        }

        //if not in cache try to fetch
      	//with no-cache because we dont want to cache a 304 response ...
	      //learn more here
	      //https://stackoverflow.com/questions/29246444/fetch-how-do-you-make-a-non-cached-request 
        var networkResponsePromise = fetch(event.request, {cache: "no-cache"});
        event.waitUntil(
          async function () 
          {
            try {
              var networkResponse = await networkResponsePromise;
              if(networkResponse.status == 200)
              {
                console.log(`sw: status=200 into ${cache_name} putting fetched resource: ${event.request.url}`);
                await cache.put(event.request, networkResponse.clone());
              }
              else
              {
                console.error(`sw: ${cache_name} received code ${networkResponse.code} for resource: ${event.request.url}`);
              }
            }
            catch(e) { console.error(`exception during fetch ${e}`); }
          }()
        );   
        return networkResponsePromise;
      }
   }());
});
