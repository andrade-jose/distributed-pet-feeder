// ========== CONFIGURAÇÕES SEGURAS ==========
// IMPORTANTE: As credenciais MQTT agora são obtidas do servidor de forma segura
let MQTT_CONFIG = null; // Será carregado após autenticação

// Tópicos MQTT - OTIMIZADOS (compatíveis com Central ESP32)
const TOPICS = {
    // Tópicos de operação (remotas)
    command: 'a/r/c',           // Comandos para remotas
    heartbeat: 'a/r/hb',        // Heartbeat das remotas
    alerta: 'a/r/al',           // Alertas de ração

    // Tópicos de configuração (via Central)
    configSet: 'a/c/cs',        // Enviar configuração para Central
    configQuery: 'a/c/cq',      // Solicitar configuração da Central
    configUpdate: 'a/c/cu',     // Receber notificações de mudanças
    state: 'a/c/s/+',           // Receber estados completos (retain)

    // Legados (manter compatibilidade)
    status_legacy: 'alimentador/remota/status',
    heartbeat_legacy: 'alimentador/remota/heartbeat',
    resposta_legacy: 'alimentador/remota/resposta',
    alerta_legacy: 'alimentador/remota/alerta_racao'
};

// Usuários e permissões
const USERS = {
    'operador': { password: 'operador123', role: 'operador' },
    'dev': { password: 'dev123', role: 'dev' },
    'admin': { password: 'admin123', role: 'dev' }
};

// ================================================

// Estado da aplicação
let client = null;
let currentUser = null;
let feedersData = {};
let connectionAttempts = 0;
const MAX_CONNECTION_ATTEMPTS = 5;

// Funções de autenticação
function login() {
    const username = document.getElementById('username').value;
    const password = document.getElementById('password').value;
    const role = document.getElementById('role').value;
    const rememberPassword = document.getElementById('rememberPassword').checked;

    if (!username || !password) {
        showToast('Preencha todos os campos', 'error');
        return;
    }

    // Verificar credenciais
    if (USERS[username] && USERS[username].password === password) {
        // Verificar se o tipo de acesso selecionado corresponde ao usuário
        if (USERS[username].role !== role) {
            showToast('Tipo de acesso não permitido para este usuário', 'error');
            return;
        }

        currentUser = {
            username: username,
            role: role
        };

        // Salvar credenciais se "Lembrar senha" estiver marcado
        if (rememberPassword) {
            localStorage.setItem('savedUsername', username);
            localStorage.setItem('savedPassword', password);
            localStorage.setItem('savedRole', role);
        } else {
            // Limpar credenciais salvas
            localStorage.removeItem('savedUsername');
            localStorage.removeItem('savedPassword');
            localStorage.removeItem('savedRole');
        }

        // Atualizar interface
        document.getElementById('userDisplay').textContent = username;
        document.getElementById('userRole').textContent = role === 'dev' ? 'Desenvolvedor' : 'Operador';

        // Mostrar dashboard e esconder login
        document.getElementById('loginModal').classList.add('hidden');
        document.getElementById('dashboard').style.display = 'block';

        // Mostrar seção MQTT manual se for desenvolvedor
        if (role === 'dev') {
            document.getElementById('mqttManualSection').style.display = 'block';
        } else {
            document.getElementById('mqttManualSection').style.display = 'none';
        }

        addLog('Sistema', `Usuário ${username} logou como ${role}`);
        showToast(`Bem-vindo, ${username}!`, 'success');

        // Conectar ao MQTT se for desenvolvedor
        if (role === 'dev') {
            connectMQTT();
        }
    } else {
        showToast('Credenciais inválidas', 'error');
    }
}

// Carregar credenciais salvas ao iniciar
function loadSavedCredentials() {
    const savedUsername = localStorage.getItem('savedUsername');
    const savedPassword = localStorage.getItem('savedPassword');
    const savedRole = localStorage.getItem('savedRole');

    if (savedUsername && savedPassword && savedRole) {
        document.getElementById('username').value = savedUsername;
        document.getElementById('password').value = savedPassword;
        document.getElementById('role').value = savedRole;
        document.getElementById('rememberPassword').checked = true;
    }
}

function logout() {
    if (client && client.connected) {
        disconnectMQTT();
    }
    
    addLog('Sistema', `Usuário ${currentUser.username} fez logout`);
    currentUser = null;
    
    // Resetar interface
    document.getElementById('username').value = '';
    document.getElementById('password').value = '';
    document.getElementById('role').value = 'operador';
    
    // Mostrar login e esconder dashboard
    document.getElementById('loginModal').classList.remove('hidden');
    document.getElementById('dashboard').style.display = 'none';
    
    // Limpar dados dos alimentadores
    feedersData = {};
    document.getElementById('feedersGrid').innerHTML = '';
    
    showToast('Logout realizado com sucesso', 'info');
}

// Conectar ao MQTT
function connectMQTT() {
    if (!currentUser || currentUser.role !== 'dev') {
        showToast('Apenas desenvolvedores podem conectar ao MQTT', 'error');
        return;
    }

    if (client && client.connected) {
        showToast('Já conectado ao MQTT', 'info');
        return;
    }

    const url = `wss://${MQTT_CONFIG.host}:${MQTT_CONFIG.port}/mqtt`;
    
    addLog('Sistema', `Conectando a ${MQTT_CONFIG.host}...`);
    updateConnectionButtons(true, false);

    client = mqtt.connect(url, {
        username: MQTT_CONFIG.username,
        password: MQTT_CONFIG.password,
        clientId: MQTT_CONFIG.clientId,
        reconnectPeriod: 5000,
        connectTimeout: 10000
    });

    client.on('connect', () => {
        addLog('Sistema', '✅ Conectado ao MQTT!');
        updateMQTTStatus(true);
        connectionAttempts = 0;
        updateConnectionButtons(false, true);

        // Assinar tópicos NOVOS (otimizados)
        client.subscribe(TOPICS.heartbeat);      // a/r/hb
        client.subscribe(TOPICS.alerta);         // a/r/al
        client.subscribe(TOPICS.configUpdate);   // a/c/cu (notificações)
        client.subscribe(TOPICS.state);          // a/c/s/+ (estados retain)

        // Assinar tópicos LEGADOS (compatibilidade)
        client.subscribe(TOPICS.status_legacy);
        client.subscribe(TOPICS.heartbeat_legacy);
        client.subscribe(TOPICS.resposta_legacy);
        client.subscribe(TOPICS.alerta_legacy);

        addLog('Sistema', '📥 Inscrito em 8 tópicos (4 novos + 4 legados)');

        // Solicitar status inicial (não necessário com retain, mas força refresh)
        requestStatusAll();

        showToast('Conectado ao MQTT com sucesso!', 'success');
    });

    client.on('message', (topic, message) => {
        handleMQTTMessage(topic, message.toString());
    });

    client.on('error', (error) => {
        addLog('Erro', error.message, 'error');
        updateMQTTStatus(false);
        updateConnectionButtons(false, false);
        
        connectionAttempts++;
        if (connectionAttempts >= MAX_CONNECTION_ATTEMPTS) {
            showToast(`Falha na conexão após ${MAX_CONNECTION_ATTEMPTS} tentativas`, 'error');
        }
    });

    client.on('close', () => {
        addLog('Sistema', '⚠️ Conexão MQTT fechada', 'warning');
        updateMQTTStatus(false);
        updateConnectionButtons(false, false);
    });

    client.on('reconnect', () => {
        addLog('Sistema', '🔄 Tentando reconectar...', 'warning');
    });
}

function disconnectMQTT() {
    if (client) {
        client.end();
        addLog('Sistema', 'Desconectado do MQTT');
        updateMQTTStatus(false);
        updateConnectionButtons(false, false);
        showToast('Desconectado do MQTT', 'info');
    }
}

function handleMQTTMessage(topic, message) {
    console.log(`[MQTT] ${topic}: ${message}`);

    try {
        // Parsear JSON (compacto ou legado)
        let data;
        try {
            data = JSON.parse(message);
        } catch {
            data = { raw: message };
        }

        // === TÓPICO: Estado completo (a/c/s/X) - RETAIN ===
        if (topic.startsWith('a/c/s/')) {
            const remotaId = topic.split('/')[3];
            handleEstadoCompleto(remotaId, data);
            return;
        }

        // === TÓPICO: Notificação de mudança (a/c/cu) ===
        if (topic === TOPICS.configUpdate) {
            handleNotificacaoMudanca(data);
            return;
        }

        // === TÓPICO: Heartbeat (a/r/hb) ===
        if (topic === TOPICS.heartbeat || topic === TOPICS.heartbeat_legacy) {
            handleHeartbeat(data);
            return;
        }

        // === TÓPICO: Alerta (a/r/al) ===
        if (topic === TOPICS.alerta || topic === TOPICS.alerta_legacy) {
            handleAlerta(data);
            return;
        }

        // === TÓPICOS LEGADOS (compatibilidade) ===
        const parts = topic.split('/');
        const messageType = parts[parts.length - 1];

        switch(messageType) {
            case 'status':
                const feederId = parts[2] || '001';
                if (feedersData[feederId]) {
                    feedersData[feederId].online = true;
                    feedersData[feederId].lastHeartbeat = new Date();
                    addLog(feederId, `📊 Status recebido (legacy)`);
                }
                break;

            case 'resposta':
                addLog('Sistema', `📥 Resposta: ${message}`);
                if (data.comando === 'ALIMENTAR' || data.action === 'feed') {
                    showToast(`Alimentação realizada!`, 'success');
                }
                break;

            case 'alerta_racao':
                const fId = parts[2] || '001';
                if (feedersData[fId]) {
                    feedersData[fId].online = true;
                    addLog(fId, `⚠️ Alerta: ${message}`, 'warning');
                    showToast(`Atenção: Nível de ração baixo!`, 'error');
                    updateFeederCard(fId);
                }
                break;

            default:
                addLog('MQTT', `Tópico desconhecido: ${topic}`, 'warning');
        }
    } catch (e) {
        addLog('Erro', `Falha ao processar mensagem [${topic}]: ${e.message}`, 'error');
    }
}

// === NOVOS HANDLERS PARA TÓPICOS OTIMIZADOS ===

// Handler: Estado completo (a/c/s/X)
function handleEstadoCompleto(remotaId, data) {
    // data = {"r":1,"f":[{"h":8,"m":30,"q":250,"u":"08:30"},...],"o":1,"a":1,"t":123456}
    const feederId = String(remotaId).padStart(3, '0');

    // Criar ou atualizar feeder
    if (!feedersData[feederId]) {
        feedersData[feederId] = {
            id: feederId,
            name: `🐕 Alimentador Remoto ${remotaId}`,
            online: false,
            lastFeed: 'Nunca',
            lastHeartbeat: new Date(),
            schedule: [],
            autoDiscovered: true
        };
        addLog('Descoberta', `Remota ${remotaId} detectada via estado completo`);
    }

    // Atualizar dados
    feedersData[feederId].online = data.o === 1;
    feedersData[feederId].ativa = data.a === 1;

    // Atualizar horários (f = feeds)
    if (data.f && Array.isArray(data.f)) {
        feedersData[feederId].schedule = data.f.map(feed => ({
            time: `${String(feed.h).padStart(2, '0')}:${String(feed.m).padStart(2, '0')}`,
            portion: feed.q,
            lastExec: feed.u || 'Nunca'
        }));
    }

    addLog(feederId, `📡 Estado completo recebido (${data.f.length} refeições)`);
    renderFeeders();
}

// Handler: Notificação de mudança (a/c/cu)
function handleNotificacaoMudanca(data) {
    // data = {"r":1,"i":0,"h":9,"m":15,"q":300,"s":"l"}
    const remotaId = data.r;
    const feederId = String(remotaId).padStart(3, '0');
    const indice = data.i;
    const hora = data.h;
    const minuto = data.m;
    const quantidade = data.q;
    const origem = data.s === 'l' ? 'LCD' : 'Dashboard';

    addLog(feederId, `🔔 Mudança de config (${origem}): Refeição ${indice + 1} → ${String(hora).padStart(2, '0')}:${String(minuto).padStart(2, '0')} (${quantidade}g)`);

    // Atualizar localmente se já existe
    if (feedersData[feederId] && feedersData[feederId].schedule[indice]) {
        feedersData[feederId].schedule[indice] = {
            time: `${String(hora).padStart(2, '0')}:${String(minuto).padStart(2, '0')}`,
            portion: quantidade,
            lastExec: feedersData[feederId].schedule[indice].lastExec || 'Nunca'
        };
        renderFeeders();
        showToast(`Horário ${indice + 1} atualizado por ${origem}`, 'info');
    }
}

// Handler: Heartbeat (a/r/hb)
function handleHeartbeat(data) {
    // Formato novo: {"s":1,"i":1,"r":-45,"a":0,"t":0}
    // Formato legado: {"status":"ALIVE","remota_id":1,...}
    let remotaId = data.i || data.remota_id || 1;
    const feederId = String(remotaId).padStart(3, '0');

    // Descoberta automática
    if (!feedersData[feederId]) {
        feedersData[feederId] = {
            id: feederId,
            name: `🐕 Alimentador Remoto ${remotaId}`,
            online: true,
            lastFeed: 'Nunca',
            lastHeartbeat: new Date(),
            schedule: [
                { time: '08:00', portion: 50, lastExec: 'Nunca' },
                { time: '14:00', portion: 50, lastExec: 'Nunca' },
                { time: '20:00', portion: 50, lastExec: 'Nunca' }
            ],
            autoDiscovered: true
        };
        addLog('Descoberta', `Remota ${remotaId} detectada via heartbeat`);
        renderFeeders();
    }

    feedersData[feederId].online = true;
    feedersData[feederId].lastHeartbeat = new Date();
    addLog(feederId, `💓 Heartbeat`);
    updateFeederCard(feederId);
}

// Handler: Alerta (a/r/al)
function handleAlerta(data) {
    // Formato novo: {"i":1,"n":1,"d":12.5}
    // Formato legado: {"remota_id":1,"nivel":"BAIXO",...}
    const remotaId = data.i || data.remota_id || 1;
    const nivel = data.n === 1 || data.nivel === 'BAIXO';
    const feederId = String(remotaId).padStart(3, '0');

    if (nivel) {
        addLog(feederId, `⚠️ ALERTA: Ração baixa!`, 'warning');
        showToast(`Remota ${remotaId}: Ração baixa!`, 'error');
    } else {
        addLog(feederId, `✅ Nível de ração OK`);
    }

    if (feedersData[feederId]) {
        updateFeederCard(feederId);
    }
}

function sendCommand(feederId, command, data = {}) {
    if (!client || !client.connected) {
        addLog('Erro', 'MQTT desconectado', 'error');
        showToast('Erro: MQTT desconectado', 'error');
        return false;
    }

    if (!currentUser || currentUser.role !== 'dev') {
        showToast('Apenas desenvolvedores podem enviar comandos', 'error');
        return false;
    }

    let payload;

    // Mapear comandos para formato esperado pela remota
    switch(command) {
        case 'FEED_NOW':
            // Formato: {"acao":"alimentar","tempo":5,"remota_id":1}
            payload = JSON.stringify({
                acao: "alimentar",
                tempo: data.portion ? Math.round(data.portion / 10) : 5, // Converte gramas para segundos (50g = 5s)
                remota_id: parseInt(feederId)
            });
            break;

        case 'TEST_SERVO':
            // Usar comando legado: a3 (3 segundos)
            payload = "a3";
            break;

        case 'REQUEST_STATUS':
            payload = "STATUS";
            break;

        case 'UPDATE_SCHEDULE':
            // Por enquanto não implementado na remota
            addLog('Aviso', 'Atualização de horário não suportada pela remota', 'warning');
            showToast('Função não disponível na remota', 'error');
            return false;

        case 'STOP':
            payload = "STOP";
            break;

        case 'PING':
            payload = "PING";
            break;

        default:
            addLog('Erro', `Comando desconhecido: ${command}`, 'error');
            return false;
    }

    client.publish(TOPICS.command, payload);
    addLog('Comando', `📤 ${command} enviado para remota ${feederId}`);
    return true;
}

function feedNow(feederId) {
    if (sendCommand(feederId, 'FEED_NOW', { portion: 50 })) {
        showLoading(feederId, true);
        setTimeout(() => showLoading(feederId, false), 5000);
    }
}

function testServo(feederId) {
    if (sendCommand(feederId, 'TEST_SERVO')) {
        showLoading(feederId, true);
        setTimeout(() => showLoading(feederId, false), 3000);
    }
}

function updateSchedule(feederId, scheduleIndex) {
    if (!client || !client.connected) {
        showToast('MQTT desconectado', 'error');
        return;
    }

    if (!currentUser || currentUser.role !== 'dev') {
        showToast('Apenas desenvolvedores podem configurar horários', 'error');
        return;
    }

    const timeInput = document.getElementById(`time-${feederId}-${scheduleIndex}`);
    const portionInput = document.getElementById(`portion-${feederId}-${scheduleIndex}`);

    // Validação básica
    if (!timeInput.value) {
        showToast('Por favor, selecione um horário válido', 'error');
        return;
    }

    const portion = parseInt(portionInput.value);
    if (isNaN(portion) || portion < 10 || portion > 990) {
        showToast('Porção deve ser entre 10g e 990g', 'error');
        return;
    }

    // Extrair hora e minuto
    const [hora, minuto] = timeInput.value.split(':').map(Number);

    // Converter feederId (string "001") para remotaId (número 1)
    const remotaId = parseInt(feederId);

    // JSON compacto: {"r":1,"i":0,"h":9,"m":15,"q":300}
    const payload = JSON.stringify({
        r: remotaId,
        i: scheduleIndex,
        h: hora,
        m: minuto,
        q: portion
    });

    // Enviar para Central via tópico a/c/cs
    client.publish(TOPICS.configSet, payload);

    addLog(feederId, `📤 Config enviada: Refeição ${scheduleIndex + 1} → ${hora}:${minuto} (${portion}g)`);
    showToast(`Aguardando confirmação da Central...`, 'info');

    // Nota: A atualização local será feita quando receber a/c/cu (notificação)
}

function requestStatusAll() {
    Object.keys(feedersData).forEach(feederId => {
        sendCommand(feederId, 'REQUEST_STATUS');
    });
}

function renderFeeders() {
    const grid = document.getElementById('feedersGrid');
    grid.innerHTML = '';

    const feederIds = Object.keys(feedersData);
    
    if (feederIds.length === 0) {
        grid.innerHTML = `
            <div class="feeder-card" style="grid-column: 1 / -1; text-align: center; padding: 40px;">
                <h3>Nenhum alimentador detectado</h3>
                <p>Conecte-se ao MQTT para descobrir alimentadores automaticamente</p>
            </div>
        `;
        return;
    }

    feederIds.forEach(feederId => {
        const data = feedersData[feederId];
        
        const card = document.createElement('div');
        card.className = 'feeder-card';
        card.id = `feeder-${feederId}`;

        card.innerHTML = `
            <div class="loading-overlay" id="loading-${feederId}">
                <div class="spinner"></div>
            </div>
            <div class="feeder-header">
                <span class="feeder-name">${data.name}</span>
                <span class="feeder-status ${data.online ? 'online' : 'offline'}"></span>
            </div>

            <div class="info-row">
                <span class="info-label">ID</span>
                <span class="info-value">${data.id}</span>
            </div>

            <div class="info-row">
                <span class="info-label">Status</span>
                <span class="info-value">${data.online ? '🟢 Online' : '🔴 Offline'}</span>
            </div>

            <div class="info-row">
                <span class="info-label">Última alimentação</span>
                <span class="info-value">${data.lastFeed}</span>
            </div>

            ${data.autoDiscovered ? `
            <div class="info-row">
                <span class="info-label">Descoberta</span>
                <span class="info-value">🔄 Automática</span>
            </div>
            ` : ''}

            <div class="schedule-section">
                <div class="schedule-title">⏰ Programação de Horários</div>
                ${data.schedule.map((sched, idx) => `
                    <div class="schedule-item">
                        <input type="time" id="time-${feederId}-${idx}" value="${sched.time}" 
                            ${currentUser.role !== 'dev' ? 'disabled' : ''}>
                        <input type="number" id="portion-${feederId}-${idx}" value="${sched.portion}" 
                            min="10" max="200" step="10" ${currentUser.role !== 'dev' ? 'disabled' : ''}>
                        <button onclick="updateSchedule('${feederId}', ${idx})" 
                            ${currentUser.role !== 'dev' ? 'disabled' : ''}>💾</button>
                    </div>
                `).join('')}
            </div>

            <div class="control-buttons">
                <button class="btn btn-primary" onclick="feedNow('${feederId}')" 
                    ${!data.online || currentUser.role !== 'dev' ? 'disabled' : ''}>
                    🍽️ Alimentar Agora
                </button>
                <button class="btn btn-secondary" onclick="testServo('${feederId}')" 
                    ${!data.online || currentUser.role !== 'dev' ? 'disabled' : ''}>
                    🔧 Testar Servo
                </button>
            </div>
        `;

        grid.appendChild(card);
    });
}

function updateFeederCard(feederId) {
    renderFeeders();
}

function updateMQTTStatus(connected) {
    const statusEl = document.getElementById('mqttStatus');
    if (connected) {
        statusEl.textContent = '● Conectado';
        statusEl.className = 'status-badge connected';
    } else {
        statusEl.textContent = '● Desconectado';
        statusEl.className = 'status-badge disconnected';
    }
}

function updateConnectionButtons(connecting, connected) {
    const connectBtn = document.getElementById('connectBtn');
    const disconnectBtn = document.getElementById('disconnectBtn');
    
    if (connecting) {
        connectBtn.disabled = true;
        connectBtn.textContent = 'Conectando...';
        disconnectBtn.disabled = true;
    } else if (connected) {
        connectBtn.disabled = true;
        connectBtn.textContent = 'Conectado';
        disconnectBtn.disabled = false;
    } else {
        connectBtn.disabled = false;
        connectBtn.textContent = 'Conectar';
        disconnectBtn.disabled = true;
    }
}

function addLog(source, message, type = 'info') {
    const logContainer = document.getElementById('logContainer');
    const timestamp = new Date().toLocaleTimeString();
    
    const entry = document.createElement('div');
    entry.className = `log-entry ${type}`;
    entry.textContent = `[${timestamp}] [${source}] ${message}`;
    
    logContainer.appendChild(entry);
    logContainer.scrollTop = logContainer.scrollHeight;

    // Limitar a 100 linhas
    while (logContainer.children.length > 100) {
        logContainer.removeChild(logContainer.firstChild);
    }
}

function showToast(message, type = 'info') {
    const toast = document.getElementById('toast');
    toast.textContent = message;
    toast.className = `toast ${type === 'error' ? 'error' : ''}`;
    toast.classList.add('show');
    
    setTimeout(() => {
        toast.classList.remove('show');
    }, 3000);
}

function showLoading(feederId, show) {
    const loadingEl = document.getElementById(`loading-${feederId}`);
    if (loadingEl) {
        loadingEl.classList.toggle('active', show);
    }
}

function updateClock() {
    const now = new Date();
    document.getElementById('currentTime').textContent = now.toLocaleTimeString('pt-BR');
}

// Verificar heartbeat (marcar offline se não receber em 30s)
setInterval(() => {
    const now = new Date();
    Object.keys(feedersData).forEach(feederId => {
        const data = feedersData[feederId];
        if (data.lastHeartbeat) {
            const diff = (now - data.lastHeartbeat) / 1000;
            if (diff > 30 && data.online) {
                data.online = false;
                addLog(feederId, '⚠️ Sem heartbeat - marcado como offline', 'warning');
                updateFeederCard(feederId);
            }
        }
    });
}, 5000);

// Funções para envio manual de mensagens MQTT
function sendManualMQTT() {
    if (!client || !client.connected) {
        showToast('Erro: MQTT desconectado', 'error');
        return;
    }

    if (!currentUser || currentUser.role !== 'dev') {
        showToast('Apenas desenvolvedores podem enviar mensagens manuais', 'error');
        return;
    }

    const topic = document.getElementById('mqttTopic').value.trim();
    const messageText = document.getElementById('mqttMessage').value.trim();

    if (!topic) {
        showToast('Por favor, informe o tópico MQTT', 'error');
        return;
    }

    if (!messageText) {
        showToast('Por favor, informe a mensagem', 'error');
        return;
    }

    // Validar JSON se parecer ser JSON (começa com { ou [)
    if (messageText.startsWith('{') || messageText.startsWith('[')) {
        try {
            JSON.parse(messageText);
        } catch (e) {
            showToast('Erro: Mensagem JSON inválida', 'error');
            return;
        }
    }

    // Enviar mensagem
    try {
        client.publish(topic, messageText);
        addLog('Manual', `📤 Enviado para ${topic}: ${messageText.substring(0, 50)}...`);
        showToast('Mensagem enviada com sucesso!', 'success');
    } catch (error) {
        addLog('Erro', `Falha ao enviar mensagem: ${error.message}`, 'error');
        showToast('Erro ao enviar mensagem', 'error');
    }
}

function clearManualMQTT() {
    document.getElementById('mqttMessage').value = '';
    showToast('Campos limpos', 'info');
}

function loadTemplate(type) {
    const topicInput = document.getElementById('mqttTopic');
    const messageInput = document.getElementById('mqttMessage');

    const templates = {
        feed: {
            topic: 'alimentador/remota/comando',
            message: {
                acao: "alimentar",
                tempo: 5,
                remota_id: 1
            }
        },
        status: {
            topic: 'alimentador/remota/comando',
            message: "STATUS"
        },
        servo: {
            topic: 'alimentador/remota/comando',
            message: "a3"
        },
        schedule: {
            topic: 'alimentador/remota/comando',
            message: {
                info: "Programação de horários não implementada na remota",
                nota: "Use o sistema central para programação"
            }
        },
        ping: {
            topic: 'alimentador/remota/comando',
            message: "PING"
        },
        stop: {
            topic: 'alimentador/remota/comando',
            message: "STOP"
        }
    };

    if (templates[type]) {
        topicInput.value = templates[type].topic;
        const msg = templates[type].message;
        messageInput.value = typeof msg === 'string' ? msg : JSON.stringify(msg, null, 2);
        showToast(`Template "${type}" carregado!`, 'info');
    }
}

// Permitir login com Enter
document.getElementById('password').addEventListener('keypress', function(e) {
    if (e.key === 'Enter') {
        login();
    }
});

// Inicialização
window.onload = () => {
    updateClock();
    setInterval(updateClock, 1000);

    // Configurar botões de conexão
    document.getElementById('connectBtn').addEventListener('click', connectMQTT);
    document.getElementById('disconnectBtn').addEventListener('click', disconnectMQTT);

    // Carregar credenciais salvas
    loadSavedCredentials();
};
