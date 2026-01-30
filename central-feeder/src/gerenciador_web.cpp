#include "gerenciador_web.h"
#include "gerenciador_mqtt.h"
#include "gerenciador_wifi.h"
#include "gerenciador_tempo.h"
#include "gerenciador_telas.h"

// Acessar a instância singleton do MQTT
#define gerenciadorMqtt GerenciadorMQTT::instance

// Inicialização das variáveis estáticas
AsyncWebServer GerenciadorWeb::server(80);
bool GerenciadorWeb::iniciado = false;
String GerenciadorWeb::requestBody = "";

void GerenciadorWeb::inicializar() {
    if (iniciado) return;
    
    DEBUG_PRINTLN("Inicializando Gerenciador Web...");
    
    // Verificar se WiFi está conectado antes de iniciar servidor
    if (WiFi.status() != WL_CONNECTED) {
        DEBUG_PRINTLN("WiFi não conectado - aguardando conexão...");
        return; // Não inicializar ainda
    }
    
    // Configurar rotas
    configurarRotas();
    
    // Iniciar servidor
    server.begin();
    iniciado = true;
    
    DEBUG_PRINTLN("Servidor Web iniciado na porta 80");
    DEBUG_PRINTF("Acesse: http://%s\n", WiFi.localIP().toString().c_str());
}

void GerenciadorWeb::atualizar() {
    // Se ainda não foi iniciado, tentar inicializar quando WiFi conectar
    if (!iniciado && WiFi.status() == WL_CONNECTED) {
        DEBUG_PRINTLN("WiFi conectado - iniciando servidor web...");
        configurarRotas();
        server.begin();
        iniciado = true;
        DEBUG_PRINTLN("Servidor Web iniciado na porta 80");
        DEBUG_PRINTF("Acesse: http://%s\n", WiFi.localIP().toString().c_str());
    }
    
    // O AsyncWebServer roda em background, não precisa de atualização manual
}

bool GerenciadorWeb::estaIniciado() {
    return iniciado;
}

void GerenciadorWeb::configurarRotas() {
    // Página principal
    server.on("/", HTTP_GET, handleRoot);

    // APIs REST
    server.on("/api/status", HTTP_GET, handleAPI_Status);
    server.on("/api/feed", HTTP_POST, handleAPI_Feed);
    server.on("/api/status_remota", HTTP_POST, handleAPI_StatusRemota);
    server.on("/api/obter_config", HTTP_GET, handleAPI_ObterConfig);
    server.on("/api/config", HTTP_GET, handleAPI_Config);
    server.on("/api/listar_remotas", HTTP_GET, handleAPI_ListarRemotas);

    // Handler para configurar com body capture
    server.on("/api/configurar", HTTP_POST, [](AsyncWebServerRequest *request){}, NULL,
             [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
                 handleAPI_ConfigurarBody(request, data, len, index, total);
             });

    // Página 404
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Página não encontrada");
    });
}

void GerenciadorWeb::handleRoot(AsyncWebServerRequest *request) {
    String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta name='viewport' content='width=device-width, initial-scale=1.0'>
    <title>Alimentador Automático</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f0f0f0; }
        .container { max-width: 400px; margin: 0 auto; background: white; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); overflow: hidden; }
        .header { background: #4CAF50; color: white; padding: 15px; text-align: center; }
        .screen { padding: 20px; min-height: 300px; }
        .hidden { display: none; }
        .menu-item { padding: 15px; border-bottom: 1px solid #eee; cursor: pointer; transition: background 0.2s; }
        .menu-item:hover { background: #f5f5f5; }
        .menu-item:last-child { border-bottom: none; }
        .status-card { background: #e8f5e8; padding: 15px; border-radius: 8px; margin: 10px 0; border-left: 4px solid #4CAF50; }
        .button { background: #4CAF50; color: white; padding: 12px 20px; border: none; border-radius: 6px; cursor: pointer; margin: 5px; font-size: 14px; }
        .button:hover { background: #45a049; }
        .button.secondary { background: #757575; }
        .button.danger { background: #f44336; }
        .input-group { margin: 15px 0; }
        .input-group label { display: block; margin-bottom: 5px; font-weight: bold; }
        .input-group input, .input-group select { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }
        .refeicao-config { background: #f9f9f9; padding: 15px; border-radius: 8px; margin: 10px 0; }
        .nav-buttons { text-align: center; padding: 15px; background: #f5f5f5; }
    </style>
</head>
<body>
    <div class='container'>
        <div class='header'>
            <h2>🐾 Alimentador Automático</h2>
            <div id='currentTime'>--:--</div>
        </div>
        
        <!-- Tela Principal -->
        <div id='screen-home' class='screen'>
            <div class='status-card'>
                <h3>Status do Sistema</h3>
                <div id='status'>Carregando...</div>
            </div>
            
            <div class='menu-item' onclick='showScreen("remotas")'>
                🤖 Configurar Remotas
            </div>
            <div class='menu-item' onclick='showScreen("status")'>
                📊 Status Detalhado
            </div>
        </div>
        
        <!-- Tela Lista de Remotas -->
        <div id='screen-remotas' class='screen hidden'>
            <h3>Selecionar Remota</h3>
            <div id='lista-remotas-container'>
                <div style='text-align: center; padding: 20px; color: #999;'>
                    Carregando remotas...
                </div>
            </div>
        </div>
        
        <!-- Tela Remota Genérica -->
        <div id='screen-remota' class='screen hidden'>
            <h3 id='remota-titulo'>🤖 Remota</h3>

            <div id='remota-refeicoes-container'>
                <!-- Refeições serão carregadas dinamicamente -->
            </div>

            <button class='button' id='btn-alimentar-remota' style='width: 100%; margin-top: 15px;'>
                🍽️ Alimentar Agora
            </button>
        </div>
        
        <!-- Tela Configurar Refeição Genérica -->
        <div id='screen-config-refeicao' class='screen hidden'>
            <h3 id='config-refeicao-titulo'>⏰ Refeição</h3>

            <div class='input-group'>
                <label>Horário:</label>
                <input type='time' id='hora-refeicao' value='08:00'>
            </div>

            <div class='input-group'>
                <label>Quantidade (gramas):</label>
                <select id='quantidade-refeicao'>
                    <option value='5'>5g</option>
                    <option value='10'>10g</option>
                    <option value='15'>15g</option>
                    <option value='20'>20g</option>
                    <option value='25'>25g</option>
                    <option value='30'>30g</option>
                    <option value='35'>35g</option>
                    <option value='40' selected>40g</option>
                    <option value='45'>45g</option>
                    <option value='50'>50g</option>
                    <option value='60'>60g</option>
                    <option value='70'>70g</option>
                    <option value='80'>80g</option>
                    <option value='90'>90g</option>
                    <option value='100'>100g</option>
                </select>
            </div>

            <button class='button' id='btn-salvar-refeicao' style='width: 100%;'>
                💾 Salvar Configuração
            </button>
        </div>
        
        <!-- Tela Status Detalhado -->
        <div id='screen-status' class='screen hidden'>
            <h3>📊 Status Detalhado</h3>
            <div id='statusDetalhado'>Carregando...</div>
        </div>
        
        <!-- Navegação -->
        <div class='nav-buttons'>
            <button class='button secondary' onclick='goBack()'>◀️ Voltar</button>
            <button class='button secondary' onclick='showScreen("home")'>🏠 Início</button>
        </div>
    </div>

    <script>
        let currentScreen = 'home';
        let screenHistory = [];
        let remotaAtual = null;
        let refeicaoAtual = null;

        function showScreen(screenId) {
            console.log('showScreen chamado:', screenId);

            // Salvar tela atual no histórico
            if (currentScreen !== screenId) {
                screenHistory.push(currentScreen);
            }

            // Esconder todas as telas
            document.querySelectorAll('.screen').forEach(screen => {
                screen.classList.add('hidden');
            });

            // Tratar IDs de remotas (remota1, remota2, etc)
            if (screenId.startsWith('remota') && !isNaN(screenId.replace('remota', ''))) {
                remotaAtual = parseInt(screenId.replace('remota', ''));
                console.log('Abrindo remota:', remotaAtual);
                mostrarTelaRemota(remotaAtual);
                screenId = 'remota'; // Usar tela genérica
            }

            // Mostrar tela selecionada
            let screenElement = document.getElementById('screen-' + screenId);
            if (screenElement) {
                screenElement.classList.remove('hidden');
                console.log('Tela exibida:', screenId);
            } else {
                console.error('Tela não encontrada:', 'screen-' + screenId);
            }

            currentScreen = screenId;

            // Se está mostrando a tela de remotas, carregar lista
            if (screenId === 'remotas') {
                console.log('Carregando lista de remotas...');
                carregarRemotas();
            }
        }

        function mostrarTelaRemota(remotaId) {
            console.log('Mostrando tela da remota:', remotaId);

            // Atualizar título
            document.getElementById('remota-titulo').innerText = '🤖 Remota ' + remotaId;

            // Carregar refeições da remota
            fetch('/api/obter_config')
                .then(response => response.json())
                .then(data => {
                    let container = document.getElementById('remota-refeicoes-container');
                    container.innerHTML = '';

                    let remotaKey = 'remota' + remotaId;
                    if (data[remotaKey]) {
                        for (let i = 1; i <= 3; i++) {
                            let refeicaoKey = 'refeicao' + i;
                            if (data[remotaKey][refeicaoKey]) {
                                let ref = data[remotaKey][refeicaoKey];
                                let menuItem = document.createElement('div');
                                menuItem.className = 'menu-item';
                                menuItem.onclick = function() {
                                    abrirConfigRefeicao(remotaId, i, ref.hora, ref.quantidade);
                                };
                                menuItem.innerHTML = '⏰ Refeição ' + i + ' - <span>' +
                                    ref.hora + ' - ' + ref.quantidade + 'g</span>';
                                container.appendChild(menuItem);
                            }
                        }
                    }
                })
                .catch(err => {
                    console.error('Erro ao carregar refeições:', err);
                });

            // Configurar botão de alimentar
            document.getElementById('btn-alimentar-remota').onclick = function() {
                alimentarAgora(remotaId);
            };
        }

        function abrirConfigRefeicao(remotaId, refeicaoId, hora, quantidade) {
            console.log('Abrindo config refeição:', remotaId, refeicaoId);

            remotaAtual = remotaId;
            refeicaoAtual = refeicaoId;

            // Atualizar título
            document.getElementById('config-refeicao-titulo').innerText =
                '⏰ Refeição ' + refeicaoId + ' - Remota ' + remotaId;

            // Preencher valores
            document.getElementById('hora-refeicao').value = hora;
            document.getElementById('quantidade-refeicao').value = quantidade;

            // Configurar botão salvar
            document.getElementById('btn-salvar-refeicao').onclick = function() {
                salvarRefeicaoGenerica();
            };

            showScreen('config-refeicao');
        }

        function salvarRefeicaoGenerica() {
            if (!remotaAtual || !refeicaoAtual) {
                alert('Erro: dados da refeição não encontrados');
                return;
            }

            let hora = document.getElementById('hora-refeicao').value;
            let quantidade = document.getElementById('quantidade-refeicao').value;

            salvarRefeicao(remotaAtual, refeicaoAtual, hora, quantidade);
        }
        
        function goBack() {
            if (screenHistory.length > 0) {
                let previousScreen = screenHistory.pop();
                showScreen(previousScreen);
                screenHistory.pop(); // Remove duplicata
            } else {
                showScreen('home');
            }
        }
        
        function atualizarStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('status').innerHTML =
                        'WiFi: ' + data.wifi + '<br>' +
                        'MQTT: ' + data.mqtt + '<br>' +
                        'Broker: ' + (data.broker_rodando ? '✅ Rodando' : '❌ Parado') + '<br>' +
                        'Clientes: ' + data.clientes_conectados + '/6';

                    document.getElementById('statusDetalhado').innerHTML =
                        '<strong>Sistema:</strong><br>' +
                        'WiFi: ' + data.wifi + '<br>' +
                        'RTC: ' + (data.rtc_conectado ? 'Conectado' : 'Erro') + '<br>' +
                        'Tempo: ' + data.tempo + '<br>' +
                        'Uptime: ' + Math.floor(data.uptime/60) + ' min<br>' +
                        'Memória: ' + Math.floor(data.memoria_livre/1024) + ' KB<br><br>' +
                        '<strong>Broker MQTT Local:</strong><br>' +
                        'Status: ' + (data.broker_rodando ? '✅ Rodando' : '❌ Parado') + '<br>' +
                        'IP: ' + data.broker_ip + '<br>' +
                        'Porta: ' + data.broker_porta + '<br>' +
                        'Cliente Central: ' + data.mqtt_status + '<br>' +
                        'Remotas conectadas: ' + data.clientes_conectados + '/6';
                })
                .catch(err => {
                    document.getElementById('status').innerHTML = 'Erro ao carregar status';
                });
        }
        
        function alimentarAgora(remotaId) {
            if(confirm('Confirma alimentação manual da Remota ' + remotaId + '?')) {
                fetch('/api/feed?remota=' + remotaId, { method: 'POST' })
                    .then(response => response.json())
                    .then(data => {
                        alert(data.message || 'Comando enviado!');
                    })
                    .catch(err => {
                        alert('Erro ao enviar comando');
                    });
            }
        }
        
        function salvarRefeicao(remotaId, refeicaoId) {
            let hora = document.getElementById('hora-remota' + remotaId + '-ref' + refeicaoId).value;
            let quantidade = document.getElementById('quantidade-remota' + remotaId + '-ref' + refeicaoId).value;
            
            let dados = {
                remota: remotaId,
                refeicao: refeicaoId,
                hora: hora,
                quantidade: parseInt(quantidade)
            };
            
            fetch('/api/configurar', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(dados)
            })
            .then(response => response.json())
            .then(data => {
                alert(data.message || 'Configuração salva!');
                // Atualizar display da refeição
                document.getElementById('remota' + remotaId + '-ref' + refeicaoId + '-info').innerText = 
                    hora + ' - ' + quantidade + 'g';
                goBack();
            })
            .catch(err => {
                alert('Erro ao salvar configuração');
            });
        }
        
        function carregarConfiguracoes() {
            fetch('/api/obter_config')
                .then(response => response.json())
                .then(data => {
                    // Atualizar displays das refeições da Remota 1
                    if(data.remota1) {
                        // Refeição 1
                        if(data.remota1.refeicao1) {
                            document.getElementById('remota1-ref1-info').innerText = 
                                data.remota1.refeicao1.hora + ' - ' + data.remota1.refeicao1.quantidade + 'g';
                            document.getElementById('hora-remota1-ref1').value = data.remota1.refeicao1.hora;
                            document.getElementById('quantidade-remota1-ref1').value = data.remota1.refeicao1.quantidade;
                        }
                        
                        // Refeição 2
                        if(data.remota1.refeicao2) {
                            document.getElementById('remota1-ref2-info').innerText = 
                                data.remota1.refeicao2.hora + ' - ' + data.remota1.refeicao2.quantidade + 'g';
                            document.getElementById('hora-remota1-ref2').value = data.remota1.refeicao2.hora;
                            document.getElementById('quantidade-remota1-ref2').value = data.remota1.refeicao2.quantidade;
                        }
                        
                        // Refeição 3
                        if(data.remota1.refeicao3) {
                            document.getElementById('remota1-ref3-info').innerText = 
                                data.remota1.refeicao3.hora + ' - ' + data.remota1.refeicao3.quantidade + 'g';
                            document.getElementById('hora-remota1-ref3').value = data.remota1.refeicao3.hora;
                            document.getElementById('quantidade-remota1-ref3').value = data.remota1.refeicao3.quantidade;
                        }
                    }
                })
                .catch(err => {
                    console.log('Erro ao carregar configurações:', err);
                });
        }
        
        function atualizarHora() {
            let agora = new Date();
            document.getElementById('currentTime').innerText =
                agora.getHours().toString().padStart(2, '0') + ':' +
                agora.getMinutes().toString().padStart(2, '0');
        }

        function carregarRemotas() {
            console.log('Carregando remotas...');
            fetch('/api/listar_remotas')
                .then(response => {
                    console.log('Resposta recebida:', response.status);
                    return response.json();
                })
                .then(data => {
                    console.log('Dados remotas:', data);
                    let container = document.getElementById('lista-remotas-container');

                    if (data.remotas && data.remotas.length > 0) {
                        console.log('Total de remotas conectadas:', data.remotas.length);
                        container.innerHTML = '';

                        data.remotas.forEach(remota => {
                            console.log('Adicionando remota:', remota.id, remota.nome);
                            let menuItem = document.createElement('div');
                            menuItem.className = 'menu-item';
                            menuItem.onclick = function() {
                                console.log('Navegando para remota', remota.id);
                                showScreen('remota' + remota.id);
                            };
                            menuItem.innerHTML = '🤖 ' + remota.nome + ' <span style="color: #4CAF50;">●</span>';
                            container.appendChild(menuItem);
                        });
                    } else {
                        console.log('Nenhuma remota conectada');
                        container.innerHTML = '<div style="text-align: center; padding: 20px; color: #999;">Nenhuma remota conectada</div>';
                    }
                })
                .catch(err => {
                    console.error('Erro ao carregar remotas:', err);
                    document.getElementById('lista-remotas-container').innerHTML =
                        '<div style="text-align: center; padding: 20px; color: #f44336;">Erro ao carregar remotas<br>' + err.message + '</div>';
                });
        }

        // Inicialização
        setInterval(atualizarStatus, 10000);
        setInterval(atualizarHora, 1000);
        setInterval(carregarConfiguracoes, 15000); // Recarregar configurações a cada 15s
        setInterval(carregarRemotas, 5000); // Recarregar remotas a cada 5s
        atualizarStatus();
        atualizarHora();
        carregarConfiguracoes(); // Carregar configurações iniciais
        carregarRemotas(); // Carregar remotas iniciais
    </script>
</body>
</html>
    )";
    
    request->send(200, "text/html", html);
}

void GerenciadorWeb::handleAPI_Status(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);

    doc["wifi"] = WiFi.isConnected() ? "Conectado" : "Desconectado";
    doc["mqtt"] = gerenciadorMqtt->estaConectado() ? "Conectado" : "Desconectado";
    doc["tempo"] = GerenciadorTempo::obterDataTempoFormatado();
    doc["uptime"] = millis() / 1000;
    doc["memoria_livre"] = ESP.getFreeHeap();
    doc["mqtt_status"] = gerenciadorMqtt->obterStatusConexao();
    doc["rtc_conectado"] = GerenciadorTempo::rtcConectado();
    doc["tempo_valido"] = GerenciadorTempo::tempoValido();

    // Informações do broker MQTT local
    doc["broker_rodando"] = gerenciadorMqtt->brokerEstaRodando();
    doc["broker_ip"] = gerenciadorMqtt->getIPBroker();
    doc["broker_porta"] = MQTT_BROKER_PORT;
    doc["clientes_conectados"] = gerenciadorMqtt->getClientesConectados();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void GerenciadorWeb::handleAPI_ConfigurarBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    // Capturar dados do body em chunks
    if(index == 0) {
        requestBody = ""; // Reset no início
    }
    
    // Adicionar chunk atual ao body
    for(size_t i = 0; i < len; i++) {
        requestBody += (char)data[i];
    }
    
    // Quando todos os chunks foram recebidos, processar
    if(index + len == total) {
        DEBUG_PRINTF("Body completo recebido (%d bytes): %s\n", total, requestBody.c_str());
        
        DynamicJsonDocument doc(512);
        DynamicJsonDocument configDoc(512);
        
        DeserializationError erro = deserializeJson(configDoc, requestBody);
        
        if (!erro) {
            int remotaId = configDoc["remota"] | 1;
            int refeicaoId = configDoc["refeicao"] | 1;
            String hora = configDoc["hora"] | "08:00";
            int quantidade = configDoc["quantidade"] | 40;
            
            DEBUG_PRINTF("Dados parseados: Remota=%d, Refeicao=%d, Hora=%s, Quantidade=%d\n", 
                        remotaId, refeicaoId, hora.c_str(), quantidade);
            
            // Extrair hora e minuto
            int h = hora.substring(0, 2).toInt();
            int m = hora.substring(3, 5).toInt();
            
            // Salvar nas Preferences
            Preferences prefs;
            prefs.begin("remotas", false);
            
            String prefixo = "r" + String(remotaId - 1) + "_";
            String refPrefixo = prefixo + "ref" + String(refeicaoId - 1) + "_";
            
            prefs.putInt((refPrefixo + "hora").c_str(), h);
            prefs.putInt((refPrefixo + "min").c_str(), m);
            prefs.putInt((refPrefixo + "quant").c_str(), quantidade);
            
            prefs.end();
            
            // Forçar sincronização imediata com o display
            GerenciadorTelas::sincronizarComPreferences();
            
            // Enviar via MQTT
            bool mqttOk = gerenciadorMqtt->configurarHorarioRemota(remotaId, h, m, quantidade);
            
            doc["status"] = "ok";
            doc["message"] = "Configuração salva: Refeição " + String(refeicaoId) + 
                           " - " + hora + " - " + String(quantidade) + "g";
            doc["mqtt_enviado"] = mqttOk;
            
            DEBUG_PRINTF("✅ Configuração salva: Remota %d, Refeição %d, %02d:%02d, %dg\n", 
                       remotaId, refeicaoId, h, m, quantidade);
            
        } else {
            doc["status"] = "error";
            doc["message"] = "JSON inválido: " + String(erro.c_str());
            DEBUG_PRINTF("❌ Erro JSON: %s\n", erro.c_str());
        }
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
        
        // Limpar body para próxima requisição
        requestBody = "";
    }
}

void GerenciadorWeb::handleAPI_ObterConfig(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(2048);
    
    // Acessar dados reais das Preferences (mesmo sistema do GerenciadorTelas)
    Preferences prefs;
    prefs.begin("remotas", true); // true = somente leitura
    
    int numeroRemotas = prefs.getInt("count", 0);
    
    for (int i = 0; i < numeroRemotas && i < MAX_REMOTAS; i++) {
        String prefixo = "r" + String(i) + "_";
        String remotaKey = "remota" + String(i + 1);
        
        // Carregar dados de cada refeição
        for (int j = 0; j < REFEICOES_PER_REMOTA; j++) {
            String refPrefixo = prefixo + "ref" + String(j) + "_";
            String refeicaoKey = "refeicao" + String(j + 1);
            
            int hora = prefs.getInt((refPrefixo + "hora").c_str(), 8 + j * 6);
            int minuto = prefs.getInt((refPrefixo + "min").c_str(), 0);
            int quantidade = prefs.getInt((refPrefixo + "quant").c_str(), 40);
            
            // Formatar hora como string HH:MM
            String horaStr = String(hora);
            String minutoStr = String(minuto);
            if (horaStr.length() < 2) horaStr = "0" + horaStr;
            if (minutoStr.length() < 2) minutoStr = "0" + minutoStr;
            
            doc[remotaKey][refeicaoKey]["hora"] = horaStr + ":" + minutoStr;
            doc[remotaKey][refeicaoKey]["quantidade"] = quantidade;
        }
    }
    
    prefs.end();
    
    // Se não há dados salvos, usar valores padrão
    if (numeroRemotas == 0) {
        doc["remota1"]["refeicao1"]["hora"] = "08:00";
        doc["remota1"]["refeicao1"]["quantidade"] = 40;
        doc["remota1"]["refeicao2"]["hora"] = "14:30";
        doc["remota1"]["refeicao2"]["quantidade"] = 40;
        doc["remota1"]["refeicao3"]["hora"] = "20:00";
        doc["remota1"]["refeicao3"]["quantidade"] = 40;
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void GerenciadorWeb::handleAPI_Configurar(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(512);
    
    // Capturar dados do body da requisição
    if (request->hasParam("body", true)) {
        String body = request->getParam("body", true)->value();
        DEBUG_PRINTF("Body recebido: %s\n", body.c_str());
        
        DynamicJsonDocument configDoc(512);
        DeserializationError erro = deserializeJson(configDoc, body);
        
        if (!erro) {
            int remotaId = configDoc["remota"] | 1;
            int refeicaoId = configDoc["refeicao"] | 1;
            String hora = configDoc["hora"] | "08:00";
            int quantidade = configDoc["quantidade"] | 40;
            
            DEBUG_PRINTF("Dados parseados: Remota=%d, Refeicao=%d, Hora=%s, Quantidade=%d\n", 
                        remotaId, refeicaoId, hora.c_str(), quantidade);
            
            // Extrair hora e minuto
            int h = hora.substring(0, 2).toInt();
            int m = hora.substring(3, 5).toInt();
            
            // Salvar direto nas Preferences (mesmo sistema da central)
            Preferences prefs;
            prefs.begin("remotas", false); // false = leitura/escrita
            
            String prefixo = "r" + String(remotaId - 1) + "_"; // ID começa em 1, array em 0
            String refPrefixo = prefixo + "ref" + String(refeicaoId - 1) + "_";
            
            prefs.putInt((refPrefixo + "hora").c_str(), h);
            prefs.putInt((refPrefixo + "min").c_str(), m);
            prefs.putInt((refPrefixo + "quant").c_str(), quantidade);
            
            prefs.end();
            
            // Forçar sincronização imediata com o display
            GerenciadorTelas::sincronizarComPreferences();
            
            // Também enviar via MQTT para sincronizar com a remota
            bool mqttOk = gerenciadorMqtt->configurarHorarioRemota(remotaId, h, m, quantidade);
            
            doc["status"] = "ok";
            doc["message"] = "Configuração salva: Refeição " + String(refeicaoId) + 
                           " - " + hora + " - " + String(quantidade) + "g";
            doc["mqtt_enviado"] = mqttOk;
            
            DEBUG_PRINTF("Configuração salva: Remota %d, Refeição %d, %02d:%02d, %dg\n", 
                       remotaId, refeicaoId, h, m, quantidade);
        } else {
            doc["status"] = "error";
            doc["message"] = "JSON inválido: " + String(erro.c_str());
            DEBUG_PRINTF("Erro JSON: %s\n", erro.c_str());
        }
    } else {
        // Tentar capturar de outras formas
        String body = "";
        
        // Verificar se há parâmetros individuais
        if (request->hasParam("remota", true)) {
            int remotaId = request->getParam("remota", true)->value().toInt();
            int refeicaoId = request->getParam("refeicao", true)->value().toInt();
            String hora = request->getParam("hora", true)->value();
            int quantidade = request->getParam("quantidade", true)->value().toInt();
            
            DEBUG_PRINTF("Dados por parâmetros: Remota=%d, Refeicao=%d, Hora=%s, Quantidade=%d\n", 
                        remotaId, refeicaoId, hora.c_str(), quantidade);
            
            // Processar da mesma forma
            int h = hora.substring(0, 2).toInt();
            int m = hora.substring(3, 5).toInt();
            
            Preferences prefs;
            prefs.begin("remotas", false);
            
            String prefixo = "r" + String(remotaId - 1) + "_";
            String refPrefixo = prefixo + "ref" + String(refeicaoId - 1) + "_";
            
            prefs.putInt((refPrefixo + "hora").c_str(), h);
            prefs.putInt((refPrefixo + "min").c_str(), m);
            prefs.putInt((refPrefixo + "quant").c_str(), quantidade);
            
            prefs.end();
            
            // Forçar sincronização imediata com o display
            GerenciadorTelas::sincronizarComPreferences();
            
            bool mqttOk = gerenciadorMqtt->configurarHorarioRemota(remotaId, h, m, quantidade);
            
            doc["status"] = "ok";
            doc["message"] = "Configuração salva via parâmetros";
            doc["mqtt_enviado"] = mqttOk;
        } else {
            doc["status"] = "error";  
            doc["message"] = "Nenhum dado encontrado na requisição";
            DEBUG_PRINTLN("Nenhum parâmetro encontrado na requisição");
        }
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void GerenciadorWeb::handleAPI_StatusRemota(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(512);
    
    // Solicitar status da Remota 1
    bool sucesso = gerenciadorMqtt->solicitarStatusRemota(1);
    
    if (sucesso) {
        doc["status"] = "ok";
        doc["message"] = "Solicitação de status enviada para Remota 1";
        DEBUG_PRINTLN("Solicitação de status enviada via web interface");
    } else {
        doc["status"] = "error";
        doc["message"] = "Erro ao solicitar status via MQTT";
        DEBUG_PRINTLN("Falha ao solicitar status via web interface");
    }
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void GerenciadorWeb::handleAPI_Feed(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(512);

    // Obter ID da remota da query string
    int remotaId = 1; // padrão
    if (request->hasParam("remota")) {
        remotaId = request->getParam("remota")->value().toInt();
    }

    // Tentar diferentes comandos que a remota pode aceitar
    bool sucesso = false;

    // Primeira tentativa: comando simples "alimentar"
    sucesso = gerenciadorMqtt->enviarComandoGeral("alimentar", 5, remotaId);

    if (!sucesso) {
        // Segunda tentativa: comando específico
        sucesso = gerenciadorMqtt->enviarComandoRemota(remotaId, "alimentar", 5);
    }

    if (sucesso) {
        doc["status"] = "ok";
        doc["message"] = "Comando de alimentação enviado para Remota " + String(remotaId);
        DEBUG_PRINTF("Comando enviado via web interface para Remota %d\n", remotaId);
    } else {
        doc["status"] = "error";
        doc["message"] = "Erro ao enviar comando MQTT";
        DEBUG_PRINTF("Falha ao enviar comando via web interface para Remota %d\n", remotaId);
    }

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void GerenciadorWeb::handleAPI_Config(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(1024);

    doc["sistema"]["versao"] = SYSTEM_VERSION;
    doc["sistema"]["build_date"] = SYSTEM_BUILD_DATE;
    doc["sistema"]["build_time"] = SYSTEM_BUILD_TIME;
    doc["wifi"]["ssid"] = WiFi.SSID();
    doc["wifi"]["ip"] = WiFi.localIP().toString();
    doc["wifi"]["rssi"] = WiFi.RSSI();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}

void GerenciadorWeb::handleAPI_ListarRemotas(AsyncWebServerRequest *request) {
    DynamicJsonDocument doc(2048);

    // Acessar dados reais das Preferences
    Preferences prefs;
    prefs.begin("remotas", true); // true = somente leitura

    int numeroRemotas = prefs.getInt("count", 0);

    // Array de remotas conectadas
    JsonArray remotasArray = doc.createNestedArray("remotas");

    for (int i = 0; i < numeroRemotas && i < MAX_REMOTAS; i++) {
        String prefixo = "r" + String(i) + "_";

        int remotaId = prefs.getInt((prefixo + "id").c_str(), i + 1);
        String remotaNome = prefs.getString((prefixo + "nome").c_str(), "Remota " + String(i + 1));
        bool remotaConectada = prefs.getBool((prefixo + "conn").c_str(), false);

        // Verificar status de conexão em tempo real via MQTT
        if (gerenciadorMqtt) {
            remotaConectada = gerenciadorMqtt->remotaEstaConectada(remotaId);
        }

        // Apenas adicionar remotas conectadas
        if (remotaConectada) {
            JsonObject remotaObj = remotasArray.createNestedObject();
            remotaObj["id"] = remotaId;
            remotaObj["nome"] = remotaNome;
            remotaObj["conectada"] = true;
        }
    }

    prefs.end();

    doc["total_conectadas"] = remotasArray.size();

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
}