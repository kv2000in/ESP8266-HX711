// The version of the cache.
const VERSION = "v1";

// The name of the cache
const CACHE_NAME = `Smart-Scale-App-${VERSION}`;

// The static resources that the app needs to function.
const APP_STATIC_RESOURCES = [

"./index.html",
"./icon-512.png",
"./manifest.json",
"./sw.js",
"./chart.js",
];



const putInCache = async (request, response) => {
const cache = await caches.open(CACHE_NAME);
await cache.put(request, response);
};

const cacheFirst = async ({ request, fallbackUrl }) => {
// First try to get the resource from the cache.
const responseFromCache = await caches.match(request);
if (responseFromCache) {
console.log ("Returning resources from Cache");
return responseFromCache;
}

// If the response was not found in the cache,
// try to get the resource from the network.
try {
const responseFromNetwork = await fetch(request);
// If the network request succeeded, clone the response:
// - put one copy in the cache, for the next time
// - return the original to the app
// Cloning is needed because a response can only be consumed once.
putInCache(request, responseFromNetwork.clone());
console.log ("Returning resources from Network");
return responseFromNetwork;
} catch (error) {
// If the network request failed,
// get the fallback response from the cache.
const fallbackResponse = await caches.match(fallbackUrl);
if (fallbackResponse) {
console.log ("Returning fallback response");
return fallbackResponse;
}
// When even the fallback response is not available,
// there is nothing we can do, but we must always
// return a Response object.
return new Response("Network error happened", {
status: 408,
headers: { "Content-Type": "text/plain" },
});
}
};


const cacheFirstWithRefresh = async ({request}) =>  {
const fetchResponsePromise = fetch(request).then(async (networkResponse) => {
if (networkResponse.ok) {
const cache = await caches.open(CACHE_NAME);
cache.put(request, networkResponse.clone());
}

return networkResponse;
});


return (await caches.match(request)) || (await fetchResponsePromise);
}


// On install  cache the static resources
/*Installation is attempted when the downloaded file is found to be new 
— either different to an existing service worker (byte-wise compared), 
or the first service worker encountered for this page/site.
If this is the first time a service worker has been made available, 
installation is attempted, then after a successful installation, it is activated.*/

self.addEventListener("install", (event) => {
event.waitUntil(
(async () => {


//a)delete old cache - what if there is no network

/*  caches.open(CACHE_NAME).then((cache) => {
cache.delete(APP_STATIC_RESOURCES).then((response) => {
console.log("Old cache deleted");
});
}); */   

//b)request all the resources from network and add all static resources to the cache
const cache = await caches.open(CACHE_NAME);
cache.addAll(APP_STATIC_RESOURCES);    

console.log("Cache renewed");



})(),
);
});

self.addEventListener("activate", (event) => {
console.log("handling activate event");

});


// On fetch, intercept server requests
// and respond with cached responses instead of going to network
/*self.addEventListener("fetch", (event) => {
// As a single page app, direct app to always go to cached home page.
if (event.request.mode === "navigate") {
event.respondWith(caches.match("/"));
return;
}

// For all other requests, go to the cache first, and then the network.
event.respondWith(
(async () => {
const cache = await caches.open(CACHE_NAME);
const cachedResponse = await cache.match(event.request.url);
if (cachedResponse) {
// Return the cached response if it's available.
return cachedResponse;
}
// If resource isn't in the cache, return a 404.
return new Response(null, { status: 404 });
})(),
);
});*/



/*self.addEventListener("fetch", (event) => {
event.respondWith(
cacheFirst({
request: event.request,
fallbackUrl: "./icon-512.png",
}),
);
});*/

self.addEventListener("fetch", (event) => {
event.respondWith(
cacheFirstWithRefresh({
request: event.request
}),
);
});

function openPage(whichURL) {
window.open(
whichURL, 
"_self"
);
}

