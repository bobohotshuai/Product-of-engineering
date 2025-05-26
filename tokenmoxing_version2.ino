#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "DHT.h"

#define DHTPIN 17
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "OnePlus 12";
const char* password = "ljy489475";

AsyncWebServer server(80);
String gpt_response = "等待中...";
bool isProcessing = false;

String qwen_token = "sk-9ab68ee3a0f9425197dc230391179b92";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>零食环境分析</title>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <style>
    body { 
      font-family: Arial; 
      padding: 20px; 
      background-color: #f5f5f5;
    }
    .container {
      max-width: 600px;
      margin: 0 auto;
      background: white;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }
    h2 { color: #333; text-align: center; }
    select, button {
      padding: 10px;
      margin: 5px;
      border: 1px solid #ddd;
      border-radius: 5px;
    }
    button {
      background-color: #4CAF50;
      color: white;
      cursor: pointer;
    }
    button:hover { background-color: #45a049; }
    button:disabled { 
      background-color: #cccccc; 
      cursor: not-allowed; 
    }
    #result { 
      margin-top: 20px; 
      padding: 15px;
      background-color: #f9f9f9;
      border-radius: 5px;
      min-height: 100px;
      border: 1px solid #e0e0e0;
    }
    .status { margin-top: 10px; font-size: 14px; color: #666; }
  </style>
</head>
<body>
  <div class="container">
    <h2>🍿 零食环境分析系统</h2>
    <div>
      <label for="snack">选择零食种类：</label><br>
      <select id="snack">
        <optgroup label="🥔 薯类制品">
          <option value="薯片">🥔 薯片</option>
          <option value="薯条">🍟 薯条</option>
          <option value="爆米花">🍿 爆米花</option>
          <option value="虾片">🦐 虾片</option>
          <option value="锅巴">🍘 锅巴</option>
        </optgroup>
        
        <optgroup label="🍫 甜食类">
          <option value="巧克力">🍫 巧克力</option>
          <option value="糖果">🍬 糖果</option>
          <option value="棒棒糖">🍭 棒棒糖</option>
          <option value="软糖">🟢 软糖</option>
          <option value="硬糖">💎 硬糖</option>
          <option value="棉花糖">☁️ 棉花糖</option>
          <option value="牛轧糖">🧈 牛轧糖</option>
          <option value="太妃糖">🟤 太妃糖</option>
          <option value="口香糖">🔴 口香糖</option>
        </optgroup>
        
        <optgroup label="🍪 烘焙类">
          <option value="饼干">🍪 饼干</option>
          <option value="曲奇">🟤 曲奇</option>
          <option value="威化">📦 威化</option>
          <option value="蛋卷">🌀 蛋卷</option>
          <option value="吐司干">🍞 吐司干</option>
          <option value="面包干">🥖 面包干</option>
          <option value="苏打饼干">⚪ 苏打饼干</option>
          <option value="夹心饼干">🟫 夹心饼干</option>
        </optgroup>
        
        <optgroup label="🥜 坚果类">
          <option value="坚果">🥜 坚果(混合)</option>
          <option value="花生">🥜 花生</option>
          <option value="瓜子">🌻 瓜子</option>
          <option value="开心果">🟢 开心果</option>
          <option value="腰果">🌙 腰果</option>
          <option value="核桃">🧠 核桃</option>
          <option value="杏仁">🤍 杏仁</option>
          <option value="榛子">🟤 榛子</option>
          <option value="松子">🌲 松子</option>
          <option value="碧根果">🟫 碧根果</option>
        </optgroup>
        
        <optgroup label="🍇 果干类">
          <option value="果干">🍇 果干(混合)</option>
          <option value="葡萄干">🍇 葡萄干</option>
          <option value="蜜枣">🔴 蜜枣</option>
          <option value="无花果干">🟣 无花果干</option>
          <option value="芒果干">🥭 芒果干</option>
          <option value="菠萝干">🍍 菠萝干</option>
          <option value="香蕉片">🍌 香蕉片</option>
          <option value="苹果干">🍎 苹果干</option>
          <option value="柿饼">🟠 柿饼</option>
          <option value="桂圆干">⚪ 桂圆干</option>
        </optgroup>
        
        <optgroup label="🍖 肉脯类">
          <option value="牛肉干">🥩 牛肉干</option>
          <option value="猪肉脯">🥓 猪肉脯</option>
          <option value="鸡肉干">🍗 鸡肉干</option>
          <option value="鱼片干">🐟 鱼片干</option>
          <option value="鱿鱼丝">🦑 鱿鱼丝</option>
          <option value="香肠">🌭 香肠</option>
          <option value="火腿肠">🔴 火腿肠</option>
        </optgroup>
        
        <optgroup label="🍘 谷物类">
          <option value="燕麦片">🌾 燕麦片</option>
          <option value="麦片">🌾 麦片</option>
          <option value="谷物棒">📏 谷物棒</option>
          <option value="能量棒">⚡ 能量棒</option>
          <option value="米饼">🍘 米饼</option>
          <option value="玉米片">🌽 玉米片</option>
        </optgroup>
        
        <optgroup label="🍮 果冻布丁类">
          <option value="果冻">🟢 果冻</option>
          <option value="布丁">🍮 布丁</option>
          <option value="龟苓膏">⚫ 龟苓膏</option>
          <option value="仙草冻">🟫 仙草冻</option>
          <option value="凉粉">⚪ 凉粉</option>
        </optgroup>
        
        <optgroup label="🍙 膨化食品">
          <option value="薯片">🥔 薯片</option>
          <option value="玉米棒">🌽 玉米棒</option>
          <option value="雪饼">❄️ 雪饼</option>
          <option value="仙贝">🍘 仙贝</option>
          <option value="爆米花">🍿 爆米花</option>
          <option value="膨化豆">🟡 膨化豆</option>
        </optgroup>
        
        <optgroup label="🥠 传统小食">
          <option value="月饼">🥮 月饼</option>
          <option value="绿豆糕">🟢 绿豆糕</option>
          <option value="桃酥">🍑 桃酥</option>
          <option value="麻花">🌀 麻花</option>
          <option value="花生糖">🥜 花生糖</option>
          <option value="芝麻糖">⚫ 芝麻糖</option>
          <option value="酥糖">🟡 酥糖</option>
          <option value="糖葫芦">🍡 糖葫芦</option>
        </optgroup>
        
        <optgroup label="🥨 咸味小食">
          <option value="咸菜">🥒 咸菜</option>
          <option value="泡菜">🌶️ 泡菜</option>
          <option value="海苔">🟢 海苔</option>
          <option value="紫菜">🟣 紫菜</option>
          <option value="豆腐干">🟫 豆腐干</option>
          <option value="辣条">🌶️ 辣条</option>
          <option value="咸鸭蛋">🥚 咸鸭蛋</option>
        </optgroup>
        
        <optgroup label="🧊 冷冻食品">
          <option value="冰淇淋">🍦 冰淇淋</option>
          <option value="雪糕">🍧 雪糕</option>
          <option value="冰棒">🧊 冰棒</option>
          <option value="冰激凌蛋糕">🎂 冰激凌蛋糕</option>
        </optgroup>
      </select>
    </div>
    <br>
    <button id="analyzeBtn" onclick="getData()">🔍 开始分析</button>
    <button onclick="getStatus()">📊 获取状态</button>
    
    <div id="result">等待分析...</div>
    <div class="status" id="status">系统就绪</div>
  </div>

  <script>
    let isAnalyzing = false;
    
    function getData() {
      if (isAnalyzing) {
        alert("分析正在进行中，请稍候...");
        return;
      }
      
      isAnalyzing = true;
      const btn = document.getElementById("analyzeBtn");
      btn.disabled = true;
      btn.innerText = "🔄 分析中...";
      
      document.getElementById("result").innerHTML = "🔄 正在获取传感器数据并分析...";
      document.getElementById("status").innerText = "正在分析...";
      
      const snack = document.getElementById("snack").value;
      
      fetch("/gpt?snack=" + encodeURIComponent(snack))
        .then(response => response.text())
        .then(data => {
          document.getElementById("result").innerHTML = data;
          document.getElementById("status").innerText = "分析完成 - " + new Date().toLocaleTimeString();
        })
        .catch(error => {
          document.getElementById("result").innerHTML = "❌ 请求失败: " + error;
          document.getElementById("status").innerText = "请求失败";
        })
        .finally(() => {
          isAnalyzing = false;
          btn.disabled = false;
          btn.innerText = "🔍 开始分析";
          
          // 5秒后自动获取最新状态
          setTimeout(getStatus, 5000);
        });
    }
    
    function getStatus() {
      fetch("/status")
        .then(response => response.text())
        .then(data => {
          if (data !== document.getElementById("result").innerHTML) {
            document.getElementById("result").innerHTML = data;
            document.getElementById("status").innerText = "状态已更新 - " + new Date().toLocaleTimeString();
          }
        })
        .catch(error => {
          console.log("状态获取失败:", error);
        });
    }
    
    // 每30秒自动检查状态更新
    setInterval(getStatus, 30000);
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  // 等待串口初始化
  delay(1000);
  Serial.println("系统启动中...");

  WiFi.begin(ssid, password);
  Serial.print("连接WiFi中");
  int wifi_retry = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_retry < 20) {
    delay(500);
    Serial.print(".");
    wifi_retry++;
    yield();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi已连接!");
    Serial.println("IP地址: " + WiFi.localIP().toString());
    Serial.println("信号强度: " + String(WiFi.RSSI()) + " dBm");
  } else {
    Serial.println("\n❌ WiFi连接失败，请检查网络设置");
  }

  // 主页路由
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html);
  });

  // 分析请求路由
  server.on("/gpt", HTTP_GET, [](AsyncWebServerRequest *request){
    if (isProcessing) {
      request->send(200, "text/html", "⏳ 系统正在处理中，请稍候再试...");
      return;
    }
    
    String snackType = request->hasParam("snack") ? request->getParam("snack")->value() : "零食";
    String temp = readDHTTemperature();
    String hum = readDHTHumidity();

    Serial.println("收到分析请求 - 零食类型: " + snackType);
    Serial.println("当前环境 - 温度: " + temp + "℃, 湿度: " + hum + "%");

    String prompt = "当前环境温度为 " + temp + "℃，湿度为 " + hum + "%，零食种类：" + snackType +
                     "。请简要分析该环境对零食存放的影响，给出是否适宜和简短建议。回答请简洁明了，控制在200字以内。";

    gpt_response = "🔄 正在分析环境数据中...<br>📊 温度: " + temp + "℃<br>💧 湿度: " + hum + "%<br>🍿 零食类型: " + snackType;
    isProcessing = true;

    // 创建任务处理GPT请求
    xTaskCreatePinnedToCore(
      gptTask,
      "gptTask",
      12288,  // 增加堆栈大小
      new String(prompt),
      1,
      NULL,
      1
    );

    request->send(200, "text/html", gpt_response);
  });

  // 状态查询路由
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", gpt_response);
  });

  // 传感器数据路由
  server.on("/sensor", HTTP_GET, [](AsyncWebServerRequest *request){
    String temp = readDHTTemperature();
    String hum = readDHTHumidity();
    String json = "{\"temperature\":\"" + temp + "\",\"humidity\":\"" + hum + "\",\"timestamp\":\"" + String(millis()) + "\"}";
    request->send(200, "application/json", json);
  });

  server.begin();
  Serial.println("🌐 HTTP服务器已启动");
  Serial.println("请访问: http://" + WiFi.localIP().toString());
}

void loop() {
  // 检查WiFi连接状态
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck > 30000) { // 每30秒检查一次
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️ WiFi连接丢失，尝试重连...");
      WiFi.reconnect();
    }
    lastCheck = millis();
  }
  
  delay(100);
  yield();
}

String readDHTTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) {
    Serial.println("⚠️ 温度传感器读取失败");
    return "读取失败";
  }
  return String(t, 1);
}

String readDHTHumidity() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("⚠️ 湿度传感器读取失败");
    return "读取失败";
  }
  return String(h, 1);
}

void gptTask(void* parameter) {
  String prompt = *((String*)parameter);
  delete (String*)parameter;
  
  Serial.println("🚀 开始GPT分析任务");
  
  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(15000); // 15秒超时
  
  HTTPClient http;
  http.setTimeout(15000);
  http.setConnectTimeout(10000);
  
  if (!http.begin(client, "https://dashscope.aliyuncs.com/api/v1/services/aigc/text-generation/generation")) {
    gpt_response = "❌ HTTP连接初始化失败";
    isProcessing = false;
    Serial.println("❌ HTTP连接初始化失败");
    vTaskDelete(NULL);
    return;
  }

  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + String(qwen_token));
  http.addHeader("User-Agent", "ESP32-SnackAnalyzer/1.0");

  // 构建请求体
  StaticJsonDocument<1024> requestDoc;
  requestDoc["model"] = "qwen-plus";
  requestDoc["input"]["messages"][0]["role"] = "user";
  requestDoc["input"]["messages"][0]["content"] = prompt;
  requestDoc["parameters"]["result_format"] = "message";
  requestDoc["parameters"]["max_tokens"] = 500;
  requestDoc["parameters"]["temperature"] = 0.7;

  String body;
  serializeJson(requestDoc, body);
  
  Serial.println("📤 发送请求到阿里云API...");
  Serial.println("请求大小: " + String(body.length()) + " bytes");
  
  int httpCode = http.POST(body);
  Serial.println("📥 HTTP响应码: " + String(httpCode));

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String response = http.getString();
      Serial.println("✅ 收到响应，长度: " + String(response.length()) + " bytes");
      
      StaticJsonDocument<4096> doc;
      DeserializationError error = deserializeJson(doc, response);

      if (error) {
        gpt_response = "❌ JSON解析失败: " + String(error.c_str());
        Serial.println("❌ JSON解析失败: " + String(error.c_str()));
      } else {
        if (doc.containsKey("output") && 
            doc["output"].containsKey("choices") && 
            doc["output"]["choices"].size() > 0 && 
            doc["output"]["choices"][0].containsKey("message") &&
            doc["output"]["choices"][0]["message"].containsKey("content")) {
          
          String content = doc["output"]["choices"][0]["message"]["content"].as<String>();
          
          // 格式化响应
          content.replace("\\n", "<br>");
          content.replace("\n", "<br>");
          content.trim();
          
          gpt_response = "🤖 <strong>AI分析结果:</strong><br><br>" + content;
          Serial.println("✅ GPT分析完成");
        } else {
          gpt_response = "❌ API响应格式异常";
          Serial.println("❌ API响应格式异常");
          Serial.println("响应内容: " + response.substring(0, 200) + "...");
        }
      }
    } else {
      gpt_response = "❌ HTTP错误码: " + String(httpCode);
      String errorResponse = http.getString();
      Serial.println("❌ HTTP错误响应: " + errorResponse.substring(0, 200));
    }
  } else {
    String errorMsg;
    switch(httpCode) {
      case HTTPC_ERROR_CONNECTION_REFUSED:
        errorMsg = "连接被拒绝";
        break;
      case HTTPC_ERROR_SEND_HEADER_FAILED:
        errorMsg = "发送请求头失败";
        break;
      case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
        errorMsg = "发送请求体失败";
        break;
      case HTTPC_ERROR_NOT_CONNECTED:
        errorMsg = "未连接到服务器";
        break;
      case HTTPC_ERROR_CONNECTION_LOST:
        errorMsg = "连接丢失";
        break;
      case HTTPC_ERROR_READ_TIMEOUT:
        errorMsg = "读取超时";
        break;
      default:
        errorMsg = "未知网络错误";
    }
    
    gpt_response = "❌ 网络请求失败<br>错误码: " + String(httpCode) + "<br>错误信息: " + errorMsg + "<br><br>🔧 请检查:<br>• 网络连接是否正常<br>• API密钥是否有效<br>• 服务器是否可访问";
    Serial.println("❌ 请求失败 - 错误码: " + String(httpCode) + " (" + errorMsg + ")");
  }

  http.end();
  isProcessing = false;
  Serial.println("🏁 GPT任务完成");
  vTaskDelete(NULL);
}