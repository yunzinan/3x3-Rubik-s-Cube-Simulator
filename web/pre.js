// Ensure Module.canvas is set before main (default shell may create canvas later)
(function() {
  if (typeof Module === 'undefined') Module = {};
  Module.preRun = Module.preRun || [];
  Module.preRun.push(function() {
    var c = document.getElementById('canvas') || document.querySelector('canvas');
    if (c) Module.canvas = c;
  });
})();

// Play move sound from preloaded /audio.mp3 (VFS). Browsers require audio to be
// unlocked by a user gesture first; we unlock on first click/keydown.
(function() {
  if (typeof Module === 'undefined') Module = {};
  var audioUnlocked = false;
  var cachedMoveSoundUrl = null;  // blob URL of decoded /audio.mp3, set after first unlock

  function unlockAudio() {
    if (audioUnlocked) return;
    audioUnlocked = true;
    // Unlock so future Audio.play() is allowed (browser autoplay policy).
    try {
      var silent = new Audio('data:audio/wav;base64,UklGRiQAAABXQVZFZm10IBAAAAABAAEAQB8AAEAfAAABAAgAZGF0YQAAAAA=');
      silent.volume = 0;
      silent.play().then(function() {}, function() {});
    } catch (e) {}
    try {
      if (typeof AudioContext !== 'undefined' || typeof webkitAudioContext !== 'undefined') {
        var Ctx = typeof AudioContext !== 'undefined' ? AudioContext : webkitAudioContext;
        var ctx = new Ctx();
        if (ctx.state === 'suspended') ctx.resume();
      }
    } catch (e2) {}
  }

  function ensureUnlock() {
    if (audioUnlocked) return;
    document.addEventListener('keydown', unlockOnce, true);
    document.addEventListener('click', unlockOnce, true);
    document.addEventListener('touchstart', unlockOnce, true);
    function unlockOnce() {
      unlockAudio();
      document.removeEventListener('keydown', unlockOnce, true);
      document.removeEventListener('click', unlockOnce, true);
      document.removeEventListener('touchstart', unlockOnce, true);
    }
  }

  Module.playMoveSound = function() {
    try {
      if (!audioUnlocked) return;  // Skip until user has interacted with the page
      if (typeof FS === 'undefined' || !FS.readFile) return;
      var data = FS.readFile('/audio.mp3');
      var blob = new Blob([data], { type: 'audio/mpeg' });
      var url = URL.createObjectURL(blob);
      var a = new Audio(url);
      a.play().catch(function() {});
      a.onended = function() { URL.revokeObjectURL(url); };
    } catch (e) {}
  };

  // Call from C++ on first user input (key/pointer) so unlock runs in same gesture as play.
  Module.unlockAudioForMoveSound = unlockAudio;

  ensureUnlock();
})();

// Workaround: Emscripten HTML5 API sometimes calls querySelector with empty/space selector
(function() {
  var origQuerySelector = document.querySelector;
  document.querySelector = function(sel) {
    if (sel === undefined || sel === null || (typeof sel === 'string' && !sel.trim()))
      return document.getElementById('canvas') || document.body;
    return origQuerySelector.call(document, sel);
  };
  var origQuerySelectorAll = document.querySelectorAll;
  document.querySelectorAll = function(sel) {
    if (sel === undefined || sel === null || (typeof sel === 'string' && !sel.trim()))
      return document.getElementById('canvas') ? [document.getElementById('canvas')] : [];
    return origQuerySelectorAll.call(document, sel);
  };
})();
