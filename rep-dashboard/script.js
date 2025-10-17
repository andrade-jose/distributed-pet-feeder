// ========== CONFIGURAÇÕES - EDITE AQUI ==========
const MQTT_CONFIG = {
    host: window.MQTT_HOST || '56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud:8884/mqtt',
    port: window.MQTT_PORT || 8884,
    username: window.MQTT_USERNAME || 'NewWeb',
    password: window.MQTT_PASSWORD || 'Senha1234',
    clientId: 'web_feeder_' + Math.random().toString(16).substr(2, 8)
};

// Tópicos MQTT
const TOPICS = {
    command: 'feeder/central/command',
    schedule: 'feeder/central/schedule',
    status: 'feeder/remote/+/status',
    heartbeat: 'feeder/remote/+/heartbeat',
    feed: 'feeder/remote/+/feed'
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

        // Atualizar interface
        document.getElementById('userDisplay').textContent = username;
        document.getElementById('userRole').textContent = role === 'dev' ? 'Desenvolvedor' : 'Operador';
        
        // Mostrar dashboard e esconder login
        document.getElementById('loginModal').classList.add('hidden');
        document.getElementById('dashboard').style.display = 'block';
        
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

        // Assinar tópicos
        client.subscribe(TOPICS.status.replace('+', '#'));
        client.subscribe(TOPICS.heartbeat.replace('+', '#'));
        client.subscribe(TOPICS.feed.replace('+', '#'));
        
        addLog('Sistema', '📥 Inscrito nos tópicos de status');
        
        // Solicitar status inicial de todas as remotas
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
    const parts = topic.split('/');
    const feederId = parts[2]; // feeder/remote/ID/status
    const messageType = parts[3];

    // Descoberta automática de novas remotas
    if (!feedersData[feederId]) {
        feedersData[feederId] = {
            id: feederId,
            name: `🐕 Alimentador ${feederId}`,
            online: true,
            lastFeed: 'Nunca',
            lastHeartbeat: new Date(),
            schedule: [
                { time: '08:00', portion: 50 },
                { time: '14:00', portion: 50 },
                { time: '20:00', portion: 50 }
            ],
            autoDiscovered: true
        };
        addLog('Descoberta', `Novo alimentador detectado: ${feederId}`);
        showToast(`Novo alimentador detectado: ${feederId}`, 'success');
    }

    try {
        const data = JSON.parse(message);

        switch(messageType) {
            case 'status':
                feedersData[feederId].online = true;
                feedersData[feederId].lastFeed = data.lastFeed || 'Nunca';
                feedersData[feederId].servoPosition = data.servoPosition || 0;
                feedersData[feederId].hallSensor = data.hallSensor || false;
                addLog(feederId, `Status recebido`);
                break;

            case 'heartbeat':
                feedersData[feederId].online = true;
                feedersData[feederId].lastHeartbeat = new Date();
                break;

            case 'feed':
                feedersData[feederId].lastFeed = data.timestamp || new Date().toLocaleString();
                addLog(feederId, `🍽️ Alimentação realizada (${data.portion || 50}g)`);
                showToast(`${feedersData[feederId].name} foi alimentado!`, 'success');
                break;
        }

        updateFeederCard(feederId);
    } catch (e) {
        addLog('Erro', `Falha ao processar mensagem: ${e.message}`, 'error');
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

    const payload = JSON.stringify({
        target: feederId,
        command: command,
        data: data,
        timestamp: new Date().toISOString(),
        user: currentUser.username
    });

    client.publish(TOPICS.command, payload);
    addLog('Comando', `📤 ${command} → ${feederId}`);
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
    const timeInput = document.getElementById(`time-${feederId}-${scheduleIndex}`);
    const portionInput = document.getElementById(`portion-${feederId}-${scheduleIndex}`);

    // Validação básica
    if (!timeInput.value) {
        showToast('Por favor, selecione um horário válido', 'error');
        return;
    }

    const portion = parseInt(portionInput.value);
    if (isNaN(portion) || portion < 10 || portion > 200) {
        showToast('Porção deve ser entre 10g e 200g', 'error');
        return;
    }

    const schedule = {
        index: scheduleIndex,
        time: timeInput.value,
        portion: portion
    };

    feedersData[feederId].schedule[scheduleIndex] = schedule;

    if (sendCommand(feederId, 'UPDATE_SCHEDULE', schedule)) {
        addLog(feederId, `⏰ Horário ${scheduleIndex + 1} atualizado: ${schedule.time}`);
        showToast('Programação atualizada com sucesso!', 'success');
    }
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
};
