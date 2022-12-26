import 'zepto.min.js';
 
function relayOn(relay, pin) {      
  //send 1 to arduino
  $.get('/arduino/'+relay+'/'+pin+ function() {});
}

function relayOff() {      
  //send 0 to arduino
  $.get('/arduino/relayCCW/0', function() {});
  $.get('/arduino/relayCW/0', function() {});
}