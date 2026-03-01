// Ensure Module.canvas is set before main (default shell may create canvas later)
(function() {
  if (typeof Module === 'undefined') Module = {};
  Module.preRun = Module.preRun || [];
  Module.preRun.push(function() {
    var c = document.getElementById('canvas') || document.querySelector('canvas');
    if (c) Module.canvas = c;
  });
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
