const CACHE_VERSION = 'v11';
const CACHE_NAME = `${registration.scope}!${CACHE_VERSION}`;

// キャッシュするファイルをセットする
const urlsToCache = [
  '.',
  '.htaccess',
  'camera-viewer.html',
  'command-standalone.html',
  'index.html',
  'data/server.py',
  'data/style.css',
  'data/Right-Arrow.png',
  'data/CodeFont.otf',
  'data/CodeFontLight.otf',
  'data/MainFont.ttf',
  'data/TitleFont.ttf',
  'data/VarsionFont.ttf',
  'data/bluejelly.js',
  'data/CameraLostConnecton.png',
  'image/Command.png',
  'image/CommandStart.png',
  'image/CommandStop.png',
  'image/Controle.png',
  'image/DisplayRotation.png',
  'image/Enter.png',
  'image/Error.png',
  'image/Fullscreen-Off.png',
  'image/Fullscreen-On.png',
  'image/Gyro-Off.png',
  'image/Gyro-On.png',
  'image/icon16.png',
  'image/icon32.png',
  'image/icon48.png',
  'image/icon64.png',
  'image/icon150.png',
  'image/Light-Off.png',
  'image/Light-On.png',
  'image/Loading.png',
  'image/Question.png',
  'image/Setting.png',
  'image/StartButton.png',
  'image/TitleLogo.png',
  'pdf/Mobile-Guide.pdf',
  'pdf/Mobile-Setting.pdf',
  'pdf/PC-Guide.pdf',
  'pdf/PC-Setting.pdf'
];

self.addEventListener('install', (event) => {
  event.waitUntil(
    // キャッシュを開く
    caches.open(CACHE_NAME)
    .then((cache) => {
      // 指定されたファイルをキャッシュに追加する
      return cache.addAll(urlsToCache.map(url => new Request(url, {credentials: 'same-origin'})));
    })
  );
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((cacheNames) => {
      return cacheNames.filter((cacheName) => {
        // このスコープに所属していて且つCACHE_NAMEではないキャッシュを探す
        return cacheName.startsWith(`${registration.scope}!`) &&
               cacheName !== CACHE_NAME;
      });
    }).then((cachesToDelete) => {
      return Promise.all(cachesToDelete.map((cacheName) => {
        // いらないキャッシュを削除する
        return caches.delete(cacheName);
      }));
    })
  );
});

self.addEventListener('fetch', (event) => {
  event.respondWith(
    caches.match(event.request)
    .then((response) => {
      // キャッシュ内に該当レスポンスがあれば、それを返す
      if (response) {
        return response;
      }

      // 重要：リクエストを clone する。リクエストは Stream なので
      // 一度しか処理できない。ここではキャッシュ用、fetch 用と2回
      // 必要なので、リクエストは clone しないといけない
      let fetchRequest = event.request.clone();

      return fetch(fetchRequest)
        .then((response) => {
          if (!response || response.status !== 200 || response.type !== 'basic') {
            // キャッシュする必要のないタイプのレスポンスならそのまま返す
            return response;
          }

          // 重要：レスポンスを clone する。レスポンスは Stream で
          // ブラウザ用とキャッシュ用の2回必要。なので clone して
          // 2つの Stream があるようにする
          let responseToCache = response.clone();

          caches.open(CACHE_NAME)
            .then((cache) => {
              cache.put(event.request, responseToCache);
            });

          return response;
        });
    })
  );
});
