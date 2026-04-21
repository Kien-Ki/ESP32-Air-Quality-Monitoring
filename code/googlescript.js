function doGet(e) {

  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();

  var time = new Date();

  var temp = e.parameter.temp;
  var hum  = e.parameter.hum;
  var gas  = e.parameter.gas;

  sheet.appendRow([time, temp, hum, gas]);

  return ContentService.createTextOutput("OK");
}
