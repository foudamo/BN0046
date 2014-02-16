var mConfig = {};
var options = {};

Pebble.addEventListener("ready", function(e) {
  // console.log("BN0046 is ready");
  // console.log("Pebble Account Token: " + Pebble.getAccountToken());
  loadLocalData();
  returnConfigToPebble();
});

Pebble.addEventListener("showConfiguration", function(e) {
  Pebble.openURL('http://ryck.github.io/BN0046/config.html');
});

Pebble.addEventListener("webviewclosed",
  function(e) {
    if (e.response) {
      var config = JSON.parse(e.response);
      saveLocalData(config);
      returnConfigToPebble();
      // console.log("configuration closed");
      // webview closed
      // options = JSON.parse(e.response);
      // console.log("Options = " + JSON.stringify(options));

    }
  }
);

function saveLocalData(config) {

  console.log("saveLocalData() " + JSON.stringify(config));

  localStorage.setItem("date", parseInt(config.show_date));
  localStorage.setItem("seconds", parseInt(config.show_seconds));
  localStorage.setItem("format", parseInt(config.date_format));
  localStorage.setItem("moon", parseInt(config.show_moon));

  loadLocalData();

}
function loadLocalData() {

  mConfig.show_date = parseInt(localStorage.getItem("date"));
  mConfig.show_seconds = parseInt(localStorage.getItem("seconds"));
  mConfig.show_moon = parseInt(localStorage.getItem("moon"));
  mConfig.date_format = parseInt(localStorage.getItem("format"));

  if(isNaN(mConfig.show_date)) {
    mConfig.show_date = 1;
  }
  if(isNaN(mConfig.show_seconds)) {
    mConfig.show_seconds = 1;
  }
  if(isNaN(mConfig.date_format)) {
    mConfig.date_format = 1;
  }
  if(isNaN(mConfig.show_moon)) {
    mConfig.show_moon = 0;
  }

  // console.log("loadLocalData() " + JSON.stringify(mConfig));
}
function returnConfigToPebble(e) {
  // console.log("Configuration window returned: " + JSON.stringify(mConfig));
  Pebble.sendAppMessage({
    "show_date":parseInt(mConfig.show_date),
    "show_seconds":parseInt(mConfig.show_seconds),
    "show_moon":parseInt(mConfig.show_moon),
    "date_format":parseInt(mConfig.date_format)
  });
}
