function logPAX(device, ssid, bssid, ipprv, ippub, wifi, ble) {
  var ss = SpreadsheetApp.getActiveSpreadsheet().getSheetByName(device);
  if (ss == null) {
    ss = SpreadsheetApp.getActiveSpreadsheet().insertSheet(device)
  }
  ss.appendRow( [new Date(), device, ssid, bssid, ipprv, ippub, wifi, ble] )
}

function avgPAX() {
  const nrows = 6
  const sss = SpreadsheetApp.getActiveSpreadsheet().getSheets()
  const jsonObjects = sss.map(ss => {
    const value = ss.getRange(ss.getLastRow()-nrows+1, 7, nrows, 2).getValues()
    const sumWifi = value.reduce((total, row) => total + row[0], 0)
    const sumBle  = value.reduce((total, row) => total + row[1], 0)
    return {[ss.getSheetName()]: {'wifi': sumWifi / nrows, 'blw': sumBle / nrows}}
  })
  return JSON.stringify(Object.assign({}, ...jsonObjects))
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
