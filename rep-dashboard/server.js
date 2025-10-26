// ========== SERVIDOR SEGURO - DASHBOARD PET FEEDER ==========
require('dotenv').config();
const express = require('express');
const session = require('express-session');
const bcrypt = require('bcryptjs');
const helmet = require('helmet');
const rateLimit = require('express-rate-limit');
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3000;

// ========== CONFIGURAÇÕES DE SEGURANÇA ==========

// CORS Manual - Permitir requisições de qualquer origem (necessário para Render.com)
app.use((req, res, next) => {
    res.header('Access-Control-Allow-Origin', req.headers.origin || '*');
    res.header('Access-Control-Allow-Credentials', 'true');
    res.header('Access-Control-Allow-Methods', 'GET, POST, PUT, DELETE, OPTIONS');
    res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept, Authorization');

    // Preflight
    if (req.method === 'OPTIONS') {
        return res.sendStatus(200);
    }
    next();
});

// Helmet para headers de segurança (desabilitado temporariamente para debug)
// app.use(helmet({
//     contentSecurityPolicy: {
//         directives: {
//             defaultSrc: ["'self'"],
//             scriptSrc: ["'self'", "'unsafe-inline'", "https://unpkg.com"],
//             scriptSrcAttr: ["'unsafe-inline'"], // Permite onclick, onsubmit, etc
//             styleSrc: ["'self'", "'unsafe-inline'"],
//             connectSrc: ["'self'", "wss://*.hivemq.cloud", "wss://56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud"],
//             imgSrc: ["'self'", "data:", "https:"],
//         }
//     }
// }));

// Rate limiting para prevenir ataques de força bruta
const loginLimiter = rateLimit({
    windowMs: 15 * 60 * 1000, // 15 minutos
    max: 5, // 5 tentativas
    message: { error: 'Muitas tentativas de login. Tente novamente em 15 minutos.' }
});

const apiLimiter = rateLimit({
    windowMs: 1 * 60 * 1000, // 1 minuto
    max: 30, // 30 requisições
    message: { error: 'Muitas requisições. Tente novamente em breve.' }
});

// Middleware
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// Sessão segura
app.use(session({
    secret: process.env.SESSION_SECRET || 'change-this-in-production-' + Math.random().toString(36),
    resave: false,
    saveUninitialized: false,
    cookie: {
        secure: process.env.NODE_ENV === 'production', // HTTPS apenas em produção
        httpOnly: true,
        maxAge: 24 * 60 * 60 * 1000 // 24 horas
    }
}));

// Servir arquivos estáticos
app.use(express.static(path.join(__dirname), {
    index: false // Não servir index.html diretamente
}));

// ========== USUÁRIOS (com senhas hashadas) ==========
// ATENÇÃO: Em produção, use um banco de dados!

const USERS = {
    'operador': {
        passwordHash: bcrypt.hashSync('operador123', 10),
        role: 'operador'
    },
    'dev': {
        passwordHash: bcrypt.hashSync('dev123', 10),
        role: 'dev'
    },
    'admin': {
        passwordHash: bcrypt.hashSync('admin123', 10),
        role: 'dev'
    }
};

// ========== MIDDLEWARE DE AUTENTICAÇÃO ==========

function requireAuth(req, res, next) {
    if (!req.session.user) {
        return res.status(401).json({ error: 'Não autenticado' });
    }
    next();
}

function requireDev(req, res, next) {
    if (!req.session.user || req.session.user.role !== 'dev') {
        return res.status(403).json({ error: 'Acesso negado. Apenas desenvolvedores.' });
    }
    next();
}

// ========== ROTAS DE AUTENTICAÇÃO ==========

// Login
app.post('/api/login', loginLimiter, async (req, res) => {
    console.log('=== LOGIN REQUEST ===');
    console.log('Body:', req.body);
    console.log('Headers:', req.headers);

    try {
        const { username, password, role } = req.body;

        // Validação básica
        if (!username || !password || !role) {
            console.log('ERRO: Campos faltando');
            return res.status(400).json({ error: 'Campos obrigatórios faltando' });
        }

        // Verificar se usuário existe
        const user = USERS[username];
        if (!user) {
            return res.status(401).json({ error: 'Credenciais inválidas' });
        }

        // Verificar senha
        const passwordMatch = await bcrypt.compare(password, user.passwordHash);
        if (!passwordMatch) {
            return res.status(401).json({ error: 'Credenciais inválidas' });
        }

        // Verificar role
        if (user.role !== role) {
            return res.status(403).json({ error: 'Tipo de acesso não permitido para este usuário' });
        }

        // Criar sessão
        req.session.user = {
            username,
            role: user.role
        };

        const responseData = {
            success: true,
            user: {
                username,
                role: user.role
            }
        };

        console.log('Enviando resposta:', responseData);
        res.json(responseData);
    } catch (error) {
        console.error('Erro no login:', error);
        res.status(500).json({ error: 'Erro interno do servidor' });
    }
});

// Logout
app.post('/api/logout', (req, res) => {
    req.session.destroy((err) => {
        if (err) {
            return res.status(500).json({ error: 'Erro ao fazer logout' });
        }
        res.json({ success: true });
    });
});

// Verificar sessão
app.get('/api/session', (req, res) => {
    if (req.session.user) {
        res.json({
            authenticated: true,
            user: req.session.user
        });
    } else {
        res.json({ authenticated: false });
    }
});

// ========== ROTA PARA OBTER CREDENCIAIS MQTT ==========
// IMPORTANTE: Apenas usuários autenticados e desenvolvedores podem acessar!

app.get('/api/mqtt-config', requireAuth, requireDev, (req, res) => {
    try {
        // Retornar credenciais MQTT das variáveis de ambiente
        const mqttConfig = {
            host: process.env.MQTT_HOST || '56d05fe4fbc64e80964aa78d92456f22.s1.eu.hivemq.cloud',
            port: parseInt(process.env.MQTT_PORT || '8884'),
            username: process.env.MQTT_USERNAME || 'NewWeb',
            password: process.env.MQTT_PASSWORD || 'Senha1234',
            clientId: 'web_feeder_' + Math.random().toString(16).substr(2, 8)
        };

        res.json(mqttConfig);
    } catch (error) {
        console.error('Erro ao obter config MQTT:', error);
        res.status(500).json({ error: 'Erro ao obter configuração MQTT' });
    }
});

// ========== ROTAS DE VALIDAÇÃO ==========

// Validar mensagem MQTT antes de enviar
app.post('/api/validate-mqtt-message', requireAuth, requireDev, apiLimiter, (req, res) => {
    try {
        const { topic, message } = req.body;

        // Validações básicas
        if (!topic || typeof topic !== 'string') {
            return res.status(400).json({ error: 'Tópico inválido' });
        }

        if (!message) {
            return res.status(400).json({ error: 'Mensagem inválida' });
        }

        // Validar tópico (deve começar com 'a/' ou 'alimentador/')
        if (!topic.startsWith('a/') && !topic.startsWith('alimentador/')) {
            return res.status(400).json({ error: 'Tópico não permitido' });
        }

        // Validar JSON se necessário
        if (typeof message === 'string' && (message.startsWith('{') || message.startsWith('['))) {
            try {
                JSON.parse(message);
            } catch (e) {
                return res.status(400).json({ error: 'JSON inválido' });
            }
        }

        res.json({ valid: true });
    } catch (error) {
        console.error('Erro na validação:', error);
        res.status(500).json({ error: 'Erro ao validar mensagem' });
    }
});

// ========== ROTA PRINCIPAL ==========

app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'index.html'));
});

// ========== TRATAMENTO DE ERROS ==========

app.use((err, req, res, next) => {
    console.error('Erro não tratado:', err);
    res.status(500).json({ error: 'Erro interno do servidor' });
});

// 404
app.use((req, res) => {
    res.status(404).json({ error: 'Rota não encontrada' });
});

// ========== INICIAR SERVIDOR ==========

app.listen(PORT, () => {
    console.log(`🚀 Servidor rodando na porta ${PORT}`);
    console.log(`📡 Ambiente: ${process.env.NODE_ENV || 'development'}`);
    console.log(`🔒 MQTT Host: ${process.env.MQTT_HOST ? '***' + process.env.MQTT_HOST.slice(-10) : 'usando padrão'}`);
});
