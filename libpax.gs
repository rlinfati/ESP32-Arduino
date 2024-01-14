function logPAX(device, ssid, bssid, ipprv, ippub, wifi, ble) {
  const ss = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet()
  const range = ss.getRange(ss.getLastRow() + 1, 1, 1, 8)
  const value = [ [ new Date(), device, ssid, bssid, ipprv, ippub, wifi, ble ] ]
  range.setValues(value)
}

function avgPAX() {
  const ss = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet()
  const range = ss.getRange(ss.getLastRow()-5, 7, 6, 2)
  const value = range.getValues();
  const sumWifi = value.reduce((total, row) => total + row[0], 0);
  const sumBle  = value.reduce((total, row) => total + row[1], 0);
  return JSON.stringify({"wifi": sumWifi / 6, "blw": sumBle  / 6})
}

function doGet(e) {
  return ContentService.createTextOutput(avgPAX()).setMimeType(ContentService.MimeType.JSON)
}

function doPost(e) {
  const device = e.parameter["device"] || "MyDevice"
  const ssid   = e.parameter["ssid"]   || "MySSID"
  const bssid  = e.parameter["bssid"]  || "00:00:00:00:00:00"
  const ipprv  = e.parameter["ipprv"]  || "127.0.0.1"
  const ippub  = e.parameter["ippub"]  || "0.0.0.0"
  const wifi   = e.parameter["wifi"]   || "0"
  const ble    = e.parameter["ble"]    || "0"
  logPAX(device, ssid, bssid, ipprv, ippub, wifi, ble)
}
