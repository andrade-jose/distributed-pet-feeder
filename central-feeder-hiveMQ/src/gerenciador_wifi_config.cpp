#include "gerenciador_wifi_config.h"
#include "display.h"
#include <ArduinoJson.h>

// Inicialização de variáveis estáticas
AsyncWebServer *GerenciadorWiFiConfig::configServer = nullptr;
DNSServer *GerenciadorWiFiConfig::dnsServer = nullptr;
bool GerenciadorWiFiConfig::modoConfig = false;
unsigned long GerenciadorWiFiConfig::inicioModoConfig = 0;
String GerenciadorWiFiConfig::requestBody = "";

void GerenciadorWiFiConfig::inicializar()
{
    // Verificar se já tem WiFi configurado
    if (!wifiConfigurado())
    {
        DEBUG_PRINTLN("WiFi nao configurado - iniciando modo configuracao");
        Display::showMessage("WiFi nao config.", "Modo AP ativo", "AlimentadorAP", "192.168.4.1");
        delay(3000);
        iniciarModoConfig();
    }
    else
    {
        DEBUG_PRINTLN("WiFi ja configurado");
    }
}

void GerenciadorWiFiConfig::atualizar()
{
    if (modoConfig)
    {
        // Processar requisições DNS (portal captive)
        if (dnsServer)
        {
            dnsServer->processNextRequest();
        }

        // Verificar timeout
        if (millis() - inicioModoConfig > CONFIG_TIMEOUT)
        {
            DEBUG_PRINTLN("Timeout do modo config - tentando conectar com credenciais salvas");
            pararModoConfig();
        }
    }
}

bool GerenciadorWiFiConfig::wifiConfigurado()
{
    Preferences prefs;
    prefs.begin(PREFS_WIFI_NAMESPACE, true);
    String ssid = prefs.getString(PREFS_WIFI_SSID, "");
    prefs.end();

    return (ssid.length() > 0);
}

void GerenciadorWiFiConfig::iniciarModoConfig()
{
    if (modoConfig)
        return;

    DEBUG_PRINTLN("=== Iniciando Modo Configuracao WiFi ===");

    // Desconectar WiFi se estiver conectado
    WiFi.disconnect(true);
    delay(100);

    // Configurar modo AP
    WiFi.mode(WIFI_AP);

    // Configurar IP estático para o AP
    WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);

    // Iniciar Access Point
    bool apOk = WiFi.softAP(AP_SSID, AP_PASSWORD, AP_CHANNEL, 0, AP_MAX_CONNECTIONS);

    if (!apOk)
    {
        DEBUG_PRINTLN("ERRO ao criar Access Point!");
        return;
    }

    DEBUG_PRINTF("Access Point criado: %s\n", AP_SSID);
    DEBUG_PRINTF("IP: %s\n", WiFi.softAPIP().toString().c_str());

    // Iniciar servidor DNS (portal captive)
    dnsServer = new DNSServer();
    dnsServer->start(DNS_PORT, "*", AP_IP);
    DEBUG_PRINTLN("DNS Server iniciado");

    // Criar servidor web na porta 80
    configServer = new AsyncWebServer(80);
    configurarRotas();
    configServer->begin();
    DEBUG_PRINTLN("Servidor de configuracao iniciado na porta 80");

    modoConfig = true;
    inicioModoConfig = millis();

    DEBUG_PRINTLN("=== Modo Config Ativo ===");
    DEBUG_PRINTLN("Conecte-se ao WiFi 'AlimentadorAP'");
    DEBUG_PRINTLN("Acesse: http://192.168.4.1");
}

void GerenciadorWiFiConfig::pararModoConfig()
{
    if (!modoConfig)
        return;

    DEBUG_PRINTLN("Parando modo configuracao...");

    // Parar servidores
    if (configServer)
    {
        configServer->end();
        delete configServer;
        configServer = nullptr;
    }

    if (dnsServer)
    {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
    }

    // Desligar AP
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);

    modoConfig = false;

    DEBUG_PRINTLN("Modo configuracao encerrado");

    // Tentar conectar com credenciais salvas
    if (wifiConfigurado())
    {
        Preferences prefs;
        prefs.begin(PREFS_WIFI_NAMESPACE, true);
        String ssid = prefs.getString(PREFS_WIFI_SSID, "");
        String password = prefs.getString(PREFS_WIFI_PASSWORD, "");
        prefs.end();

        DEBUG_PRINTF("Tentando conectar ao WiFi: %s\n", ssid.c_str());
        tentarConectar(ssid, password);
    }
}

bool GerenciadorWiFiConfig::estaEmModoConfig()
{
    return modoConfig;
}

void GerenciadorWiFiConfig::configurarRotas()
{
    // Rota principal (portal captive)
    configServer->on("/", HTTP_GET, handleRoot);
    configServer->on("/index.html", HTTP_GET, handleRoot);
    configServer->on("/generate_204", HTTP_GET, handleRoot);        // Android
    configServer->on("/hotspot-detect.html", HTTP_GET, handleRoot); // iOS

    // API de scan
    configServer->on("/scan", HTTP_GET, handleScan);

    // API de status
    configServer->on("/status", HTTP_GET, handleStatus);

    // API de salvar (com body capture)
    configServer->on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, handleSaveBody);

    // Capturar todas as outras requisições (portal captive)
    configServer->onNotFound(handleRoot);
}
// ===== FUNÇÕES HANDLERS E AUXILIARES =====
void GerenciadorWiFiConfig::handleRoot(AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>WiFi Config</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{
  font-family:Arial,sans-serif;
  background:#667eea;
  min-height:100vh;
  display:flex;
  align-items:center;
  justify-content:center;
  padding:20px
}
.container{
  background:white;
  border-radius:20px;
  box-shadow:0 20px 60px rgba(0,0,0,0.3);
  max-width:450px;
  width:100%;
  padding:40px
}
h1{
  color:#333;
  text-align:center;
  margin-bottom:10px;
  font-size:24px
}
.subtitle{
  text-align:center;
  color:#666;
  margin-bottom:30px;
  font-size:14px
}
.form-group{margin-bottom:25px}
label{
  display:block;
  margin-bottom:8px;
  color:#555;
  font-weight:600;
  font-size:14px
}
select,input{
  width:100%;
  padding:12px 15px;
  border:2px solid #e0e0e0;
  border-radius:10px;
  font-size:14px
}
select:focus,input:focus{
  outline:none;
  border-color:#667eea
}
.btn{
  width:100%;
  padding:15px;
  background:#667eea;
  color:white;
  border:none;
  border-radius:10px;
  font-size:16px;
  font-weight:600;
  cursor:pointer;
  margin-top:10px
}
.btn:hover{background:#5568d3}
.btn-secondary{background:#f093fb}
.status{
  padding:15px;
  border-radius:10px;
  margin-top:20px;
  display:none;
  text-align:center;
  font-weight:600
}
.status.success{background:#d4edda;color:#155724;display:block}
.status.error{background:#f8d7da;color:#721c24;display:block}
.status.loading{background:#fff3cd;color:#856404;display:block}
.network-list{
  max-height:200px;
  overflow-y:auto;
  border:2px solid #e0e0e0;
  border-radius:10px;
  padding:10px;
  margin-top:10px
}
.network-item{
  padding:10px;
  cursor:pointer;
  border-radius:8px;
  margin-bottom:5px;
  display:flex;
  justify-content:space-between;
  align-items:center
}
.network-item:hover{background:#f5f5f5}
.network-item.selected{background:#667eea;color:white}
.signal-strength{font-size:12px;color:#888}
.icon{
  font-size:48px;
  text-align:center;
  margin-bottom:20px
}
</style>
</head>
<body>
  <div class="container">
    <div class="icon">📶</div>
    <h1>Configurar WiFi</h1>
    <p class="subtitle">Alimentador Automático</p>

    <div class="form-group">
      <label>Redes:</label>
      <button class="btn btn-secondary" onclick="scanNetworks()">🔍 Buscar</button>
      <div id="networks" class="network-list" style="display:none"></div>
    </div>

    <div class="form-group">
      <label>SSID:</label>
      <input type="text" id="ssid" placeholder="Nome da rede">
    </div>

    <div class="form-group">
      <label>Senha:</label>
      <input type="password" id="password" placeholder="Senha WiFi">
    </div>

    <button class="btn" onclick="saveConfig()">💾 Salvar</button>
    <div id="status" class="status"></div>
  </div>

<script>
function scanNetworks(){
  const status = document.getElementById('status');
  status.className='status loading';
  status.innerText='🔍 Buscando...';
  fetch('/scan')
    .then(r=>r.json())
    .then(d=>{
      let h='';
      if(d.networks && d.networks.length>0){
        d.networks.forEach(n=>{
          let s = n.rssi > -50 ? '📶📶📶' : (n.rssi > -70 ? '📶📶' : '📶');
          h += "<div class='network-item' onclick='selectNetwork(\""+n.ssid+"\")'><span>"+n.ssid+"</span><span class='signal-strength'>"+s+" "+n.rssi+"dBm</span></div>";
        });
        document.getElementById('networks').innerHTML = h;
        document.getElementById('networks').style.display='block';
        status.style.display='none';
      } else {
        status.className='status error';
        status.innerText='❌ Nenhuma rede encontrada';
      }
    })
    .catch(()=>{
      status.className='status error';
      status.innerText='❌ Erro ao buscar redes';
    });
}

function selectNetwork(s){
  document.getElementById('ssid').value = s;
  document.querySelectorAll('.network-item').forEach(i=>{
    i.classList.remove('selected');
    if(i.innerText.includes(s)) i.classList.add('selected');
  });
}

function saveConfig(){
  const ssid = document.getElementById('ssid').value;
  const password = document.getElementById('password').value;
  const status = document.getElementById('status');

  if(!ssid){
    status.className='status error';
    status.innerText='❌ Digite o SSID';
    return;
  }

  status.className='status loading';
  status.innerText='⏳ Salvando...';

  fetch('/save',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({ssid:ssid,password:password})
  })
  .then(r=>r.json())
  .then(d=>{
    if(d.status==='ok'){
      status.className='status success';
      status.innerText='✅ '+d.message;
      setTimeout(()=>{
        status.innerText='✅ Conectado! Reiniciando...';
      },2000);
    }else{
      status.className='status error';
      status.innerText='❌ '+d.message;
    }
  })
  .catch(()=>{
    status.className='status error';
    status.innerText='❌ Erro ao salvar configuração';
  });
}

window.onload=function(){
  setTimeout(scanNetworks,500);
}
</script>
</body>
</html>
)rawliteral";

    request->send(200, "text/html", html);
}

String GerenciadorWiFiConfig::scanWiFiNetworks() {
    DEBUG_PRINTLN("Escaneando redes WiFi...");
    int n = WiFi.scanNetworks();

    DynamicJsonDocument doc(2048);
    JsonArray networks = doc.createNestedArray("networks");

    for (int i = 0; i < n && i < 20; i++) {
        JsonObject net = networks.createNestedObject();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
        net["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "encrypted";
    }

    WiFi.scanDelete();

    String response;
    serializeJson(doc, response);
    return response;
}

void GerenciadorWiFiConfig::handleScan(AsyncWebServerRequest *request)
{
    request->send(200, "application/json", scanWiFiNetworks());
}

void GerenciadorWiFiConfig::handleStatus(AsyncWebServerRequest *request)
{
    DynamicJsonDocument doc(512);
    doc["modo_config"] = modoConfig;
    doc["ap_ssid"] = AP_SSID;
    doc["ap_ip"] = WiFi.softAPIP().toString();
    doc["tempo_restante"] = (CONFIG_TIMEOUT - (millis() - inicioModoConfig)) / 1000;

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void GerenciadorWiFiConfig::handleSaveBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
    if (index == 0)
    {
        requestBody = "";
    }

    for (size_t i = 0; i < len; i++)
    {
        requestBody += (char)data[i];
    }

    if (index + len == total)
    {
        DEBUG_PRINTF("Body recebido: %s\n", requestBody.c_str());

        DynamicJsonDocument doc(512);
        DynamicJsonDocument respDoc(512);

        DeserializationError erro = deserializeJson(doc, requestBody);

        if (!erro)
        {
            String ssid = doc["ssid"] | "";
            String password = doc["password"] | "";

            DEBUG_PRINTF("SSID: %s\n", ssid.c_str());

            if (salvarCredenciais(ssid, password))
            {
                respDoc["status"] = "ok";
                respDoc["message"] = "WiFi configurado! Conectando...";

                String response;
                serializeJson(respDoc, response);
                request->send(200, "application/json", response);

                delay(1000);
                tentarConectar(ssid, password);
            }
            else
            {
                respDoc["status"] = "error";
                respDoc["message"] = "Erro ao salvar credenciais";

                String response;
                serializeJson(respDoc, response);
                request->send(500, "application/json", response);
            }
        }
        else
        {
            respDoc["status"] = "error";
            respDoc["message"] = "JSON inválido";

            String response;
            serializeJson(respDoc, response);
            request->send(400, "application/json", response);
        }

        requestBody = "";
    }
}

bool GerenciadorWiFiConfig::salvarCredenciais(const String &ssid, const String &password)
{
    Preferences prefs;
    prefs.begin(PREFS_WIFI_NAMESPACE, false);

    prefs.putString(PREFS_WIFI_SSID, ssid);
    prefs.putString(PREFS_WIFI_PASSWORD, password);

    prefs.end();

    DEBUG_PRINTLN("Credenciais salvas em Preferences");
    return true;
}

void GerenciadorWiFiConfig::tentarConectar(const String &ssid, const String &password)
{
    DEBUG_PRINTF("Tentando conectar: %s\n", ssid.c_str());

    pararModoConfig();

    WiFi.begin(ssid.c_str(), password.c_str());

    int tentativas = 0;
    while (WiFi.status() != WL_CONNECTED && tentativas < 20)
    {
        delay(500);
        DEBUG_PRINT(".");
        tentativas++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        DEBUG_PRINTLN("");
        DEBUG_PRINTLN("WiFi conectado!");
        DEBUG_PRINTF("IP: %s\n", WiFi.localIP().toString().c_str());
        Display::showMessage("WiFi Conectado!", ssid.c_str(), "IP:", WiFi.localIP().toString().c_str());
    }
    else
    {
        DEBUG_PRINTLN("");
        DEBUG_PRINTLN("Falha ao conectar!");
        Display::showMessage("Falha WiFi", "Verifique senha", "Reinicie para", "tentar novamente");
    }
}