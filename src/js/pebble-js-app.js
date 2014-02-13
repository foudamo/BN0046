var mConfig = {};

Pebble.addEventListener("ready", function(e) {
  console.log("BN0046 is ready");
  console.log("Pebble Account Token: " + Pebble.getAccountToken());
  loadLocalData();
  // returnConfigToPebble();
});

Pebble.addEventListener("showConfiguration", function(e) {
  Pebble.openURL('http://ryck.github.io/BN0046/config.html');
});

Pebble.addEventListener("webviewclosed",
  function(e) {
    if (e.response) {
      var config = JSON.parse(e.response);
      saveLocalData(config);
      // returnConfigToPebble();
      console.log("configuration closed");
      // webview closed
      var options = JSON.parse(e.response);
      Pebble.sendAppMessage(options,
          function(e) {
            console.log("Successfully delivered message with transactionId="
              + e.data.transactionId);
          },
          function(e) {
            console.log("Unable to deliver message with transactionId="
              + e.data.transactionId
              + " Error is: " + e.error.message);
          });
      console.log("Options = " + JSON.stringify(options));

    }
  }
);

function saveLocalData(config) {

  console.log("loadLocalData() " + JSON.stringify(config));

  localStorage.setItem("seconds", parseInt(config.seconds));
  localStorage.setItem("date", parseInt(config.date));
  // localStorage.setItem("KEY_WEEKDAY_US_MM_DD", parseInt(config.KEY_WEEKDAY_US_MM_DD));
  // localStorage.setItem("KEY_WEEKDAY_NON_US_DD_MM", parseInt(config.KEY_WEEKDAY_NON_US_DD_MM));
  // localStorage.setItem("KEY_SHOW_MOON", parseInt(config.KEY_SHOW_MOON));
  // localStorage.setItem("KEY_INVERSE", parseInt(config.KEY_INVERSE));

  loadLocalData();

}
function loadLocalData() {

  mConfig.seconds = parseInt(localStorage.getItem("seconds"));
  mConfig.date = parseInt(localStorage.getItem("date"));
  // mConfig.KEY_WEEKDAY_US_MM_DD = parseInt(localStorage.getItem("KEY_WEEKDAY_US_MM_DD"));
  // mConfig.KEY_WEEKDAY_NON_US_DD_MM = parseInt(localStorage.getItem("KEY_WEEKDAY_NON_US_DD_MM"));
  // mConfig.KEY_SHOW_MOON = parseInt(localStorage.getItem("KEY_SHOW_MOON"));
  // mConfig.KEY_INVERSE = parseInt(localStorage.getItem("KEY_INVERSE"));

  if(isNaN(mConfig.seconds)) {
    mConfig.seconds = 1;
  }
  if(isNaN(mConfig.date)) {
    mConfig.date = 1;
  }
  // if(isNaN(mConfig.KEY_WEEKDAY_US_MM_DD)) {
  //   mConfig.KEY_WEEKDAY_US_MM_DD = 0;
  // }
  // if(isNaN(mConfig.KEY_WEEKDAY_NON_US_DD_MM)) {
  //   mConfig.KEY_WEEKDAY_NON_US_DD_MM = 0;
  // }
  // if(isNaN(mConfig.KEY_SHOW_MOON)) {
  //   mConfig.KEY_SHOW_MOON = 0;
  // }
  // if(isNaN(mConfig.KEY_INVERSE)) {
  //   mConfig.KEY_INVERSE = 0;
  // }


  // console.log("loadLocalData() " + JSON.stringify(mConfig));
}
// function returnConfigToPebble() {
//   console.log("Configuration window returned: " + JSON.stringify(mConfig));
//   Pebble.sendAppMessage({
//     "date":parseInt(mConfig.date),
//     "seconds":parseInt(mConfig.seconds)
//   });
// }
