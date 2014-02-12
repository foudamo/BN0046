var mConfig = {};

Pebble.addEventListener("ready", function(e) {
  console.log("BN0046 is ready");
  loadLocalData();
  returnConfigToPebble();
});

Pebble.addEventListener("showConfiguration", function(e) {
  Pebble.openURL(mConfig.configureUrl);
});

Pebble.addEventListener("webviewclosed",
  function(e) {
    if (e.response) {
      var config = JSON.parse(e.response);
      saveLocalData(config);
      returnConfigToPebble();
    }
  }
);

function saveLocalData(config) {

  console.log("loadLocalData() " + JSON.stringify(config));

  localStorage.setItem("KEY_SHOW_DATE", parseInt(config.KEY_SHOW_DATE));
  localStorage.setItem("KEY_WEEKDAY_US_MM_DD", parseInt(config.KEY_WEEKDAY_US_MM_DD));
  localStorage.setItem("KEY_WEEKDAY_NON_US_DD_MM", parseInt(config.KEY_WEEKDAY_NON_US_DD_MM));
  localStorage.setItem("KEY_SHOW_MOON", parseInt(config.KEY_SHOW_MOON));
  localStorage.setItem("KEY_INVERSE", parseInt(config.KEY_INVERSE));

  loadLocalData();

}
function loadLocalData() {

  mConfig.KEY_SHOW_SECONDS = parseInt(localStorage.getItem("KEY_SHOW_SECONDS"));
  mConfig.KEY_SHOW_DATE = parseInt(localStorage.getItem("KEY_SHOW_DATE"));
  mConfig.KEY_WEEKDAY_US_MM_DD = parseInt(localStorage.getItem("KEY_WEEKDAY_US_MM_DD"));
  mConfig.KEY_WEEKDAY_NON_US_DD_MM = parseInt(localStorage.getItem("KEY_WEEKDAY_NON_US_DD_MM"));
  mConfig.KEY_SHOW_MOON = parseInt(localStorage.getItem("KEY_SHOW_MOON"));
  mConfig.KEY_INVERSE = parseInt(localStorage.getItem("KEY_INVERSE"));
  mConfig.configureUrl = "http://ryck.github.io/BN0046/config.html";

  if(isNaN(mConfig.KEY_SHOW_SECONDS)) {
    mConfig.KEY_SHOW_SECONDS = 1;
  }
  if(isNaN(mConfig.KEY_SHOW_DATE)) {
    mConfig.KEY_SHOW_DATE = 1;
  }
  if(isNaN(mConfig.KEY_WEEKDAY_US_MM_DD)) {
    mConfig.KEY_WEEKDAY_US_MM_DD = 0;
  }
  if(isNaN(mConfig.KEY_WEEKDAY_NON_US_DD_MM)) {
    mConfig.KEY_WEEKDAY_NON_US_DD_MM = 0;
  }
  if(isNaN(mConfig.KEY_SHOW_MOON)) {
    mConfig.KEY_SHOW_MOON = 0;
  }
  if(isNaN(mConfig.KEY_INVERSE)) {
    mConfig.KEY_INVERSE = 0;
  }


  console.log("loadLocalData() " + JSON.stringify(mConfig));
}
function returnConfigToPebble() {
  console.log("Configuration window returned: " + JSON.stringify(mConfig));
  Pebble.sendAppMessage({
    "KEY_SHOW_SECONDS":parseInt(mConfig.KEY_SHOW_SECONDS),
    "KEY_SHOW_DATE":parseInt(mConfig.KEY_SHOW_DATE),
    "KEY_WEEKDAY_US_MM_DD":parseInt(mConfig.KEY_WEEKDAY_US_MM_DD),
    "KEY_WEEKDAY_NON_US_DD_MM":parseInt(mConfig.KEY_WEEKDAY_NON_US_DD_MM),
    "KEY_SHOW_MOON":parseInt(mConfig.KEY_SHOW_MOON),
    "KEY_INVERSE":parseInt(mConfig.KEY_INVERSE)
  });
}
