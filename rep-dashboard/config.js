// ========== CONFIGURAÇÃO DO DASHBOARD ==========
// Este arquivo contém as configurações do dashboard

// IMPORTANTE: Configure a URL do seu servidor Render.com aqui
// Exemplo: 'https://pet-feeder-dashboard.onrender.com'
const RENDER_URL = 'https://distributed-pet-feeder.onrender.com';

// Não altere este código abaixo
const API_URL = window.location.protocol === 'file:'
    ? RENDER_URL
    : (window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1')
        ? 'http://localhost:3000'
        : window.location.origin;
