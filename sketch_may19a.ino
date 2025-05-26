#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <DHT.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <mbedtls/md.h>
#include <mbedtls/base64.h>
#include <MD5Builder.h>

// WiFi 配置
const char* ssid = "OnePlus 12";
const char* password = "ljy489475";

// 阿里云 Table Store 配置
const char* otsEndpoint = "https://lingshicunchu.cn-shanghai.ots.aliyuncs.com";
const char* otsAccessKeyId = "LTAI5tPbg6wNsjHzpQb3WQJs";
const char* otsAccessKeySecret = "7dVXIAi08ZDYClc5PZKYtQlNyEUaTs";
const char* otsTableName = "lingshi";

#define DHTPIN 17
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800);
AsyncWebServer server(80);

// HTML页面
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>零食管理系统</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }
        .container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { text-align: center; color: #333; margin-bottom: 30px; }
        .sensor-data { display: flex; justify-content: space-around; margin: 20px 0; }
        .sensor-card { background: #e3f2fd; padding: 15px; border-radius: 8px; text-align: center; flex: 1; margin: 0 10px; }
        .add-section { background: #f3e5f5; padding: 20px; border-radius: 8px; margin: 20px 0; }
        .snack-list { background: #e8f5e8; padding: 20px; border-radius: 8px; margin: 20px 0; }
        input[type="text"] { width: 70%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; }
        button { background: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin: 5px; }
        button:hover { background: #45a049; }
        .delete-btn { background: #f44336; }
        .delete-btn:hover { background: #da190b; }
        .snack-item { background: white; padding: 10px; margin: 5px 0; border-radius: 4px; display: flex; justify-content: space-between; align-items: center; }
        .message { padding: 10px; margin: 10px 0; border-radius: 4px; }
        .success { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .error { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .loading { text-align: center; color: #666; }
        .status-info { background: #d1ecf1; padding: 10px; border-radius: 4px; margin: 10px 0; font-size: 14px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>ESP32 零食储存管理系统</h1>
            <div class="status-info" id="status">系统启动中...</div>
        </div>
        
        <div class="sensor-data">
            <div class="sensor-card">
                <h3>温度</h3>
                <p id="temperature">-- °C</p>
            </div>
            <div class="sensor-card">
                <h3>湿度</h3>
                <p id="humidity">-- %</p>
            </div>
        </div>
        
        <div class="add-section">
            <h3>添加零食</h3>
            <input type="text" id="snackName" placeholder="请输入零食名称" />
            <button onclick="addSnack()">添加零食</button>
            <div id="message"></div>
        </div>
        
        <div class="snack-list">
            <h3>零食列表</h3>
            <button onclick="loadSnacks()">刷新列表</button>
            <div id="snackList"><div class="loading">加载中...</div></div>
        </div>
    </div>

    <script>
        function showMessage(msg, type) {
            const messageDiv = document.getElementById('message');
            messageDiv.innerHTML = '<div class="message ' + type + '">' + msg + '</div>';
            setTimeout(() => messageDiv.innerHTML = '', 3000);
        }
        
        function updateStatus(msg) {
            document.getElementById('status').innerHTML = msg;
        }
        
        function updateSensors() {
            fetch('/temperature')
                .then(response => response.text())
                .then(data => document.getElementById('temperature').innerHTML = data + ' °C')
                .catch(error => document.getElementById('temperature').innerHTML = '读取失败');
            
            fetch('/humidity')
                .then(response => response.text())
                .then(data => document.getElementById('humidity').innerHTML = data + ' %')
                .catch(error => document.getElementById('humidity').innerHTML = '读取失败');
        }
        
        function addSnack() {
            const snackName = document.getElementById('snackName').value.trim();
            if (!snackName) {
                showMessage('请输入零食名称！', 'error');
                return;
            }
            
            updateStatus('正在添加零食...');
            const formData = new FormData();
            formData.append('snack_name', snackName);
            
            fetch('/add_snack', {
                method: 'POST',
                body: formData
            })
            .then(response => {
                if (!response.ok) {
                    return response.text().then(text => { throw new Error(text || 'HTTP error ' + response.status); });
                }
                return response.text();
            })
            .then(data => {
                showMessage('零食添加成功！', 'success');
                document.getElementById('snackName').value = '';
                loadSnacks();
                updateStatus('系统运行正常');
            })
            .catch(error => {
                showMessage('添加失败：' + error.message, 'error');
                updateStatus('添加失败');
            });
        }
        
        function loadSnacks() {
            document.getElementById('snackList').innerHTML = '<div class="loading">加载中...</div>';
            updateStatus('正在加载零食列表...');
            
            fetch('/snack_list')
                .then(response => {
                    if (!response.ok) throw new Error('HTTP ' + response.status);
                    return response.json();
                })
                .then(data => {
                    const listDiv = document.getElementById('snackList');
                    if (!Array.isArray(data) || data.length === 0) {
                        listDiv.innerHTML = '<p>暂无零食记录</p>';
                        updateStatus('系统运行正常 - 暂无数据');
                        return;
                    }
                    
                    let html = '';
                    data.forEach(snack => {
                        html += `<div class="snack-item">
                                    <div>
                                        <strong>${snack.snack_name || '未知零食'}</strong><br>
                                        <small>存储时间: ${snack.storage_time || '未知时间'}</small>
                                    </div>
                                    <button class="delete-btn" onclick="deleteSnack('${snack.id}')">删除</button>
                                </div>`;
                    });
                    listDiv.innerHTML = html;
                    updateStatus(`系统运行正常 - 已加载 ${data.length} 条记录`);
                })
                .catch(error => {
                    document.getElementById('snackList').innerHTML = '<p>加载失败：' + error + '</p>';
                    updateStatus('数据加载失败');
                });
        }
        
        function deleteSnack(id) {
            if (!confirm('确定要删除这个零食记录吗？')) return;
            
            updateStatus('正在删除零食...');
            fetch('/delete_snack?id=' + id, {
                method: 'DELETE'
            })
            .then(response => {
                if (!response.ok) {
                    return response.text().then(text => { throw new Error(text || 'HTTP error ' + response.status); });
                }
                return response.text();
            })
            .then(data => {
                showMessage('删除成功！', 'success');
                loadSnacks();
            })
            .catch(error => {
                showMessage('删除失败：' + error.message, 'error');
                updateStatus('删除操作失败');
            });
        }
        
        window.onload = function() {
            updateStatus('正在初始化系统...');
            setTimeout(() => {
                updateSensors();
                loadSnacks();
                setInterval(updateSensors, 30000);
            }, 1000);
        };
        
        document.addEventListener('DOMContentLoaded', function() {
            const snackInput = document.getElementById('snackName');
            if (snackInput) {
                snackInput.addEventListener('keypress', function(e) {
                    if (e.key === 'Enter') addSnack();
                });
            }
        });
    </script>
</body>
</html>
)rawliteral";

// Base64编码函数
String base64Encode(const uint8_t* input, size_t length) {
    size_t output_len = 0;
    size_t required_buf_size = (length + 2) / 3 * 4 + 1;
    unsigned char* output_buf = (unsigned char*)malloc(required_buf_size);
    if (!output_buf) return "";

    int ret = mbedtls_base64_encode(output_buf, required_buf_size, &output_len, input, length);
    String result = "";
    if (ret == 0) {
        result = String((char*)output_buf);
    }
    free(output_buf);
    return result;
}

// 生成随机ID
String generateId() {
    String id = "";
    for (int i = 0; i < 8; i++) {
        id += String(random(0, 10));
    }
    return id;
}

// 获取当前本地时间
String getCurrentTime() { 
    timeClient.update();
    time_t epochTime = timeClient.getEpochTime();
    struct tm *ptm = localtime(&epochTime); 
    char timeString[20];
    strftime(timeString, 20, "%Y-%m-%d %H:%M:%S", ptm);
    return String(timeString);
}

// 修复：获取GMT格式日期字符串 - 使用正确的工作日缩写
String getGMTDateString() {
    timeClient.update();
    time_t now = timeClient.getEpochTime();
    struct tm *tm_info = gmtime(&now);
    
    // 手动构建日期字符串，确保格式正确
    const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    char dateHeader[80];
    snprintf(dateHeader, 80, "%s, %02d %s %04d %02d:%02d:%02d GMT",
             weekdays[tm_info->tm_wday],
             tm_info->tm_mday,
             months[tm_info->tm_mon],
             tm_info->tm_year + 1900,
             tm_info->tm_hour,
             tm_info->tm_min,
             tm_info->tm_sec);
    
    return String(dateHeader);
}

// 计算HMAC-SHA1签名
String calculateHMACSHA1(const String& data, const String& key) {
    const mbedtls_md_info_t* md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    if (md_info == NULL) return "";
    
    unsigned char hmac_result[20]; 
    int ret = mbedtls_md_hmac(md_info, (const unsigned char*)key.c_str(), key.length(), 
                             (const unsigned char*)data.c_str(), data.length(), hmac_result);
    
    if (ret != 0) return "";
    return base64Encode(hmac_result, 20);
}

// 简化：WiFi连接函数
bool connectToWiFi() {
    Serial.println("连接WiFi...");
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) { 
        delay(1000);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi连接成功! IP: " + WiFi.localIP().toString());
        return true;
    } else {
        Serial.println("WiFi连接失败");
        return false;
    }
}

// 简化：通用OTS请求函数
String sendOTSRequest(const String& action, const String& requestBody) {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi未连接");
        return "";
    }
    
    HTTPClient http;
    http.setTimeout(15000); // 增加超时时间
    
    String url = String(otsEndpoint) + "/" + action;
    
    // 计算Content-MD5
    MD5Builder md5;
    md5.begin();
    md5.add(requestBody);
    md5.calculate();
    uint8_t md5Result[16];
    md5.getBytes(md5Result);
    String contentMD5 = base64Encode(md5Result, 16);
    
    String date = getGMTDateString();
    String contentType = "application/json";
    
    // 构建签名字符串
    String canonicalizedOTSHeaders = "x-ots-accesskeyid:" + String(otsAccessKeyId) + "\n" +
                                    "x-ots-apiversion:2015-12-31\n" +
                                    "x-ots-date:" + date + "\n";
    
    String stringToSign = "POST\n" + contentMD5 + "\n" + contentType + "\n" + date + "\n" +
                         canonicalizedOTSHeaders + "/" + action;
    
    String signature = calculateHMACSHA1(stringToSign, otsAccessKeySecret);
    String authHeader = "OTS " + String(otsAccessKeyId) + ":" + signature;
    
    // 发送请求
    http.begin(url);
    http.addHeader("Content-Type", contentType);
    http.addHeader("x-ots-apiversion", "2015-12-31");
    http.addHeader("x-ots-accesskeyid", otsAccessKeyId);
    http.addHeader("Date", date);
    http.addHeader("Content-MD5", contentMD5);
    http.addHeader("x-ots-date", date);
    http.addHeader("Authorization", authHeader);
    
    int httpResponseCode = http.POST(requestBody);
    String responsePayload = http.getString();
    
    Serial.println(action + " HTTP响应: " + String(httpResponseCode));
    if (httpResponseCode != 200) {
        Serial.println("错误响应: " + responsePayload);
    }
    
    http.end();
    return (httpResponseCode == 200) ? responsePayload : "";
}

// 简化：插入零食数据
bool insertSnackData(String snackName, String storageTime) {
    DynamicJsonDocument doc(1024);
    doc["TableName"] = otsTableName;
    doc["Condition"] = "IGNORE";
    
    JsonArray primaryKey = doc.createNestedArray("PrimaryKey");
    JsonObject idKey = primaryKey.createNestedObject();
    idKey["ColumnName"] = "id";
    idKey["Value"] = generateId();
    
    JsonArray attributeColumns = doc.createNestedArray("AttributeColumns");
    JsonObject nameColumn = attributeColumns.createNestedObject();
    nameColumn["ColumnName"] = "snack_name";
    nameColumn["Value"] = snackName;
    
    JsonObject timeColumn = attributeColumns.createNestedObject();
    timeColumn["ColumnName"] = "storage_time";
    timeColumn["Value"] = storageTime;
    
    String requestBody;
    serializeJson(doc, requestBody);
    
    String response = sendOTSRequest("PutRow", requestBody);
    return !response.isEmpty();
}

// 简化：查询零食数据
String querySnackData() {
    DynamicJsonDocument doc(1024);
    doc["TableName"] = otsTableName;
    
    JsonObject inclusiveStartPrimaryKey = doc.createNestedObject("InclusiveStartPrimaryKey");
    JsonArray startPK = inclusiveStartPrimaryKey.createNestedArray("PrimaryKey");
    JsonObject startIdKey = startPK.createNestedObject();
    startIdKey["ColumnName"] = "id";
    startIdKey["Value"] = "00000000";
    
    JsonObject exclusiveEndPrimaryKey = doc.createNestedObject("ExclusiveEndPrimaryKey");
    JsonArray endPK = exclusiveEndPrimaryKey.createNestedArray("PrimaryKey");
    JsonObject endIdKey = endPK.createNestedObject();
    endIdKey["ColumnName"] = "id";
    endIdKey["Value"] = "99999999";
    
    doc["Direction"] = "FORWARD";
    doc["Limit"] = 50;
    
    String requestBody;
    serializeJson(doc, requestBody);
    Serial.println("--- GetRange Request Body (ESP32) ---"); // 新增打印
    Serial.println(requestBody);                            // 新增打印
    Serial.println("------------------------------------"); // 新增打印
    
    String response = sendOTSRequest("GetRange", requestBody);
    if (response.isEmpty()) return "[]";
    
    // 解析响应
    DynamicJsonDocument responseDoc(4096);
    DeserializationError error = deserializeJson(responseDoc, response);
    if (error) return "[]";
    
    DynamicJsonDocument resultDoc(4096);
    JsonArray resultArray = resultDoc.to<JsonArray>();
    
    if (responseDoc.containsKey("Rows")) {
        JsonArray rows = responseDoc["Rows"];
        for (JsonObject row : rows) {
            JsonObject snackItem = resultArray.createNestedObject();
            if (row.containsKey("PrimaryKey")) {
                JsonArray pk = row["PrimaryKey"];
                for (JsonObject pkItem : pk) {
                    if (pkItem["ColumnName"].as<String>() == "id") {
                        snackItem["id"] = pkItem["Value"].as<String>();
                        break;
                    }
                }
            }
            if (row.containsKey("AttributeColumns")) {
                JsonArray attrs = row["AttributeColumns"];
                for (JsonObject attr : attrs) {
                    String colName = attr["ColumnName"].as<String>();
                    if (colName == "snack_name") {
                        snackItem["snack_name"] = attr["Value"].as<String>();
                    } else if (colName == "storage_time") {
                        snackItem["storage_time"] = attr["Value"].as<String>();
                    }
                }
            }
        }
    }
    
    String resultJson;
    serializeJson(resultDoc, resultJson);
    return resultJson;
}

// 简化：删除零食数据
bool deleteSnackData(String id) {
    DynamicJsonDocument doc(512);
    doc["TableName"] = otsTableName;
    doc["Condition"] = "IGNORE";
    
    JsonArray primaryKey = doc.createNestedArray("PrimaryKey");
    JsonObject idKey = primaryKey.createNestedObject();
    idKey["ColumnName"] = "id";
    idKey["Value"] = id;
    
    String requestBody;
    serializeJson(doc, requestBody);
    
    String response = sendOTSRequest("DeleteRow", requestBody);
    return !response.isEmpty();
}

// 读取传感器数据
String readDHTTemperature() {
    float t = dht.readTemperature();
    return isnan(t) ? "--" : String(t, 1);
}

String readDHTHumidity() {
    float h = dht.readHumidity();
    return isnan(h) ? "--" : String(h, 1);
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("ESP32零食管理系统启动...");

    dht.begin();
    Serial.println("DHT11传感器初始化完成");

    if (!connectToWiFi()) {
        Serial.println("WiFi连接失败，系统功能受限");
    }

    timeClient.begin();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("同步NTP时间...");
        for (int i = 0; i < 10 && !timeClient.update(); i++) {
            timeClient.forceUpdate();
            delay(500);
            Serial.print(".");
        }
        Serial.println(timeClient.getEpochTime() > 1609459200 ? "\nNTP同步成功" : "\nNTP同步失败");
    }

    randomSeed(analogRead(0));

    // 设置路由
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });

    server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", readDHTTemperature());
    });

    server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", readDHTHumidity());
    });

    server.on("/add_snack", HTTP_POST, [](AsyncWebServerRequest *request){
        if (request->hasParam("snack_name", true)) {
            String snackName = request->getParam("snack_name", true)->value();
            String storageTime = getCurrentTime();
            
            if (insertSnackData(snackName, storageTime)) {
                request->send(200, "text/plain; charset=utf-8", "添加成功");
            } else {
                request->send(500, "text/plain; charset=utf-8", "添加失败");
            }
        } else {
            request->send(400, "text/plain; charset=utf-8", "缺少参数");
        }
    });

    server.on("/snack_list", HTTP_GET, [](AsyncWebServerRequest *request){
        String data = querySnackData();
        request->send(200, "application/json; charset=utf-8", data);
    });

    server.on("/delete_snack", HTTP_DELETE, [](AsyncWebServerRequest *request){
        if (request->hasParam("id")) {
            String id = request->getParam("id")->value();
            if (deleteSnackData(id)) {
                request->send(200, "text/plain; charset=utf-8", "删除成功");
            } else {
                request->send(500, "text/plain; charset=utf-8", "删除失败");
            }
        } else {
            request->send(400, "text/plain; charset=utf-8", "缺少参数");
        }
    });

    server.begin();
    Serial.println("Web服务器启动完成");
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("访问地址: http://" + WiFi.localIP().toString());
    }
}

void loop() {
    // 检查WiFi连接
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi断开，尝试重连...");
        connectToWiFi();
    }
    
    // 更新时间
    if (WiFi.status() == WL_CONNECTED) {
        timeClient.update();
    }
    
    delay(1000);
}