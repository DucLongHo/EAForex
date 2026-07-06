// ============================================================================
// RichTradingBot — Apps Script đầy đủ (License + Role + Remote Config Sync)
//
// Đây là bản THAY THẾ TOÀN BỘ cho file doGet hiện tại của bạn (Code.gs hay
// tương đương) — đã gộp nguyên logic license cũ (sheet "Licenses", cột
// accountId/name/expiry) + role (Master/Slave) + kho lưu config (Script
// Properties). Copy đè toàn bộ nội dung file cũ bằng file này.
//
// Sau khi paste: Deploy > Manage deployments > (deployment hiện tại) >
// Edit (bút chì) > Version: New version > Deploy.
// CHỈ Save KHÔNG đủ — phải "New version" thì URL /exec mới chạy code mới.
//
// ---- Chuẩn bị sheet "Licenses" ----
// Cột A = accountId | B = name | C = expiry | D = Master (MỚI — checkbox)
// Thêm cột D, đặt tên "Master" ở header, chọn cả cột (trừ header) rồi
// Insert > Checkbox. Tích đúng 1 dòng ứng với tài khoản muốn làm Master.
// Nếu lỡ tích nhầm >1 dòng, hệ thống coi như KHÔNG có Master nào (an toàn
// hơn là tự chọn đại 1 trong 2) cho tới khi bạn sửa lại còn đúng 1 dòng.
// ============================================================================

function doGet(e) {
  var action = e.parameter.action;
  var id     = e.parameter.id;

  // ---- Slave tải config mới nhất — cũng phải có license hợp lệ mới đọc được ----
  if (action === 'getconfig') {
    if (!id) return ContentService.createTextOutput('').setMimeType(ContentService.MimeType.TEXT);
    var accG = lookupAccount_(id);
    if (!accG.found || !accG.active)
      return ContentService.createTextOutput('').setMimeType(ContentService.MimeType.TEXT);
    var payload = PropertiesService.getScriptProperties().getProperty('CONFIG_PAYLOAD') || '';
    return ContentService.createTextOutput(payload).setMimeType(ContentService.MimeType.TEXT);
  }

  // ---- Nhánh license (giữ nguyên hành vi cũ, chỉ thêm field "role") ----
  try {
    if (!id) return respond("denied", "Missing ID");

    var acc = lookupAccount_(id);
    if (!acc.found) return respond("denied", "Not registered", "", "", "slave");

    var role = acc.isMaster ? "master" : "slave";
    if (acc.active) {
      return respond("ok", "Active", acc.name, acc.expiryStr, role);
    } else {
      return respond("denied", "Expired", acc.name, "", role);
    }
  } catch (err) {
    return respond("denied", "Server error: " + err.message);
  }
}

function doPost(e) {
  var action = e.parameter.action;

  // ---- Master ghi config mới — bắt buộc đúng là Master mới được ghi ----
  if (action === 'setconfig') {
    var id = e.parameter.id;
    if (!id)
      return ContentService.createTextOutput('{"status":"denied","reason":"missing_id"}')
          .setMimeType(ContentService.MimeType.JSON);

    var acc = lookupAccount_(id);
    if (!acc.found || !acc.active || !acc.isMaster) {
      return ContentService.createTextOutput('{"status":"denied","reason":"not_master"}')
          .setMimeType(ContentService.MimeType.JSON);
    }

    var payload = e.postData.contents; // raw body — chuỗi ";"-separated từ BuildConfigPayload()
    var props = PropertiesService.getScriptProperties();
    props.setProperty('CONFIG_PAYLOAD', payload);
    props.setProperty('CONFIG_VERSION', String(Date.now()));
    return ContentService.createTextOutput('{"status":"ok"}')
        .setMimeType(ContentService.MimeType.JSON);
  }

  return ContentService.createTextOutput('{"status":"error","reason":"unknown_action"}')
      .setMimeType(ContentService.MimeType.JSON);
}

// Quét sheet "Licenses" một lần: tìm dòng khớp accountId, đồng thời đếm toàn bộ
// số dòng đang tích cột Master (D) — chỉ khi ĐÚNG 1 dòng được tích thì dòng đó
// mới thật sự là Master. Đếm ≠ 1 (0 hoặc ≥2) → không ai là Master.
function lookupAccount_(id) {
  var result = { found: false, name: "", expiryStr: "", active: false, isMaster: false };
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("Licenses");
  if (!sheet) return result;

  var data = sheet.getDataRange().getValues();

  var masterCount = 0;
  for (var m = 1; m < data.length; m++) {
    if (data[m][3] === true) masterCount++;
  }

  var idStr = String(id).trim();
  for (var i = 1; i < data.length; i++) {
    var accountId = String(data[i][0]).trim();
    if (accountId !== idStr) continue;

    result.found = true;
    result.name  = data[i][1] || "";

    var expiry     = data[i][2];
    var today      = new Date(); today.setHours(0, 0, 0, 0);
    var expiryDate = new Date(expiry); expiryDate.setHours(23, 59, 59, 0);

    result.active    = (today <= expiryDate);
    result.expiryStr = expiry ? expiry.toString() : "";
    result.isMaster  = (masterCount === 1 && data[i][3] === true);
    break;
  }
  return result;
}

function respond(status, reason, name, expiry, role) {
  var obj = { status: status, reason: reason };
  if (name)   obj.name   = name;
  if (expiry) obj.expiry = expiry;
  obj.role = role || "slave"; // thiếu/lỗi → mặc định slave (an toàn hơn tự nhận Master)
  return ContentService
      .createTextOutput(JSON.stringify(obj))
      .setMimeType(ContentService.MimeType.JSON);
}
