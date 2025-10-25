// sistema_estado.cpp - IMPLEMENTAÇÕES
#include "sistema_estado.h"

// Definir a instância global
EstadoSistema estadoSistema;

// =============================================================================
// IMPLEMENTAÇÕES DOS MÉTODOS
// =============================================================================

void EstadoSistema::atualizarStatusWifi(String ssid, bool conectado, String qualidade, String ip) {
    bool mudou = (wifiConectado != conectado);
    
    wifiSSID = ssid;
    wifiConectado = conectado;
    qualidadeWifi = qualidade;
    ipWifi = ip;
    
    if (mudou && onWifiMudou) {
        onWifiMudou(conectado);
    }
    
    Serial.printf("[ESTADO] WiFi: %s, Conectado: %s, IP: %s\n", 
                 ssid.c_str(), conectado ? "SIM" : "NÃO", ip.c_str());
}

void EstadoSistema::atualizarStatusMqtt(bool conectado) {
    bool mudou = (mqttConectado != conectado);
    mqttConectado = conectado;
    
    if (mudou && onMqttMudou) {
        onMqttMudou(conectado);
    }
    
    Serial.printf("[ESTADO] MQTT: %s\n", conectado ? "CONECTADO" : "DESCONECTADO");
}

void EstadoSistema::atualizarRemota(int id, bool conectada, bool online, String nivelRacao) {
    for (int i = 0; i < numRemotas; i++) {
        if (remotas[i].id == id) {
            bool statusAnterior = remotas[i].conectada && remotas[i].online;
            bool statusNovo = conectada && online;
            bool mudou = (remotas[i].conectada != conectada) || 
                        (remotas[i].online != online);
            
            remotas[i].conectada = conectada;
            remotas[i].online = online;
            if (nivelRacao != "") {
                remotas[i].nivelRacao = nivelRacao;
            }
            if (online) {
                remotas[i].ultimoHeartbeat = millis();
            }
            
            if (mudou && onRemotaMudou) {
                onRemotaMudou(id, statusNovo);
            }
            
            Serial.printf("[ESTADO] Remota %d: %s/%s, Nível: %s\n", 
                         id, conectada ? "CONECTADA" : "DESCONECTADA",
                         online ? "ONLINE" : "OFFLINE", nivelRacao.c_str());
            return;
        }
    }
    
    // Se chegou aqui, a remota não existe - adicionar automaticamente
    Serial.printf("[ESTADO] Remota %d não encontrada, adicionando automaticamente\n", id);
    adicionarRemota(id);
    atualizarRemota(id, conectada, online, nivelRacao);
}

void EstadoSistema::adicionarRemota(int id) {
    if (numRemotas < MAX_REMOTAS) {
        remotas[numRemotas].id = id;
        remotas[numRemotas].nome = "Remota " + String(id);
        remotas[numRemotas].conectada = false;
        remotas[numRemotas].online = false;
        remotas[numRemotas].ultimoHeartbeat = 0;
        remotas[numRemotas].nivelRacao = "OK";
        
        // Inicializar refeições com valores padrão
        for (int j = 0; j < 3; j++) {
            remotas[numRemotas].refeicoes[j].hora = 8 + (j * 6);
            remotas[numRemotas].refeicoes[j].minuto = 0;
            remotas[numRemotas].refeicoes[j].quantidade = 40;
            remotas[numRemotas].refeicoes[j].ultimaExecucao = "Nunca";
        }
        
        numRemotas++;
        Serial.printf("[ESTADO] Remota %d adicionada. Total: %d\n", id, numRemotas);
    } else {
        Serial.printf("[ERRO] Máximo de remotas (%d) atingido\n", MAX_REMOTAS);
    }
}

bool EstadoSistema::remotaConectada(int id) {
    for (int i = 0; i < numRemotas; i++) {
        if (remotas[i].id == id) {
            return remotas[i].conectada && remotas[i].online;
        }
    }
    return false;
}

void EstadoSistema::verificarTimeouts() {
    unsigned long agora = millis();
    bool mudou = false;
    
    for (int i = 0; i < numRemotas; i++) {
        bool estavaOnline = remotas[i].online;
        bool estavaConectada = remotas[i].conectada;
        
        // Timeout de heartbeat (60s = offline)
        if (remotas[i].online && 
            (agora - remotas[i].ultimoHeartbeat > TIMEOUT_HEARTBEAT_REMOTA)) {
            remotas[i].online = false;
            mudou = true;
            Serial.printf("[ESTADO] Remota %d offline (timeout heartbeat)\n", 
                         remotas[i].id);
        }
        
        // Timeout de conexão (5min = desconectada)
        if (remotas[i].conectada && 
            (agora - remotas[i].ultimoHeartbeat > TIMEOUT_CONEXAO_REMOTA)) {
            remotas[i].conectada = false;
            mudou = true;
            Serial.printf("[ESTADO] Remota %d desconectada (timeout conexão)\n", 
                         remotas[i].id);
        }
        
        // Notificar mudanças
        if (mudou && onRemotaMudou) {
            bool statusNovo = remotas[i].conectada && remotas[i].online;
            bool statusAntigo = estavaConectada && estavaOnline;
            
            if (statusNovo != statusAntigo) {
                onRemotaMudou(remotas[i].id, statusNovo);
            }
        }
    }
}

void EstadoSistema::marcarRefeicoesModificadas() {
    refeicoesModificadas = true;
    Serial.println("[ESTADO] Refeições marcadas como modificadas");
}

bool EstadoSistema::verificarELimparFlagRefeicoes() {
    bool temp = refeicoesModificadas;
    refeicoesModificadas = false;
    if (temp) {
        Serial.println("[ESTADO] Flag de refeições limpa");
    }
    return temp;
}

void EstadoSistema::salvarConfiguracao() {
    Preferences prefs;
    if (!prefs.begin("remotas", false)) {
        Serial.println("[ERRO] Falha ao abrir preferences para salvar");
        return;
    }
    
    prefs.putInt("count", numRemotas);
    
    for (int i = 0; i < numRemotas; i++) {
        String prefixo = "r" + String(i) + "_";
        prefs.putInt((prefixo + "id").c_str(), remotas[i].id);
        prefs.putString((prefixo + "nome").c_str(), remotas[i].nome);
        prefs.putBool((prefixo + "conn").c_str(), remotas[i].conectada);
        
        for (int j = 0; j < 3; j++) {
            String refPrefixo = prefixo + "ref" + String(j) + "_";
            prefs.putInt((refPrefixo + "hora").c_str(), remotas[i].refeicoes[j].hora);
            prefs.putInt((refPrefixo + "min").c_str(), remotas[i].refeicoes[j].minuto);
            prefs.putInt((refPrefixo + "quant").c_str(), remotas[i].refeicoes[j].quantidade);
            prefs.putString((refPrefixo + "ultima").c_str(), remotas[i].refeicoes[j].ultimaExecucao);
        }
    }
    
    prefs.end();
    Serial.printf("[ESTADO] Configuração salva: %d remotas\n", numRemotas);
}

void EstadoSistema::carregarConfiguracao() {
    Preferences prefs;
    if (!prefs.begin("remotas", true)) {
        Serial.println("[ERRO] Falha ao abrir preferences para carregar");
        return;
    }
    
    numRemotas = prefs.getInt("count", 0);
    
    for (int i = 0; i < numRemotas && i < MAX_REMOTAS; i++) {
        String prefixo = "r" + String(i) + "_";
        remotas[i].id = prefs.getInt((prefixo + "id").c_str(), i + 1);
        remotas[i].nome = prefs.getString((prefixo + "nome").c_str(), "Remota " + String(i + 1));
        remotas[i].conectada = prefs.getBool((prefixo + "conn").c_str(), false);
        remotas[i].online = false; // Sempre começa offline
        remotas[i].ultimoHeartbeat = 0;
        remotas[i].nivelRacao = "OK";
        
        for (int j = 0; j < 3; j++) {
            String refPrefixo = prefixo + "ref" + String(j) + "_";
            remotas[i].refeicoes[j].hora = prefs.getInt((refPrefixo + "hora").c_str(), 8 + j * 6);
            remotas[i].refeicoes[j].minuto = prefs.getInt((refPrefixo + "min").c_str(), 0);
            remotas[i].refeicoes[j].quantidade = prefs.getInt((refPrefixo + "quant").c_str(), 40);
            remotas[i].refeicoes[j].ultimaExecucao = prefs.getString((refPrefixo + "ultima").c_str(), "Nunca");
        }
    }
    
    prefs.end();
    
    if (numRemotas > 0) {
        Serial.printf("[ESTADO] Configuração carregada: %d remotas\n", numRemotas);
    } else {
        Serial.println("[ESTADO] Nenhuma configuração anterior encontrada");
    }
}

void EstadoSistema::definirRefeicao(int idRemota, int indiceRefeicao, int hora, int minuto, int quantidade) {
    if (indiceRefeicao < 0 || indiceRefeicao >= 3) {
        Serial.printf("[ERRO] Índice de refeição inválido: %d\n", indiceRefeicao);
        return;
    }
    
    for (int i = 0; i < numRemotas; i++) {
        if (remotas[i].id == idRemota) {
            remotas[i].refeicoes[indiceRefeicao].hora = hora;
            remotas[i].refeicoes[indiceRefeicao].minuto = minuto;
            remotas[i].refeicoes[indiceRefeicao].quantidade = quantidade;
            
            marcarRefeicoesModificadas();
            Serial.printf("[ESTADO] Refeição %d da remota %d atualizada: %02d:%02d, %dg\n",
                         indiceRefeicao, idRemota, hora, minuto, quantidade);
            return;
        }
    }
    
    Serial.printf("[ERRO] Remota %d não encontrada para definir refeição\n", idRemota);
}

void EstadoSistema::atualizarUltimaExecucao(int idRemota, int indiceRefeicao, String tempo) {
    if (indiceRefeicao < 0 || indiceRefeicao >= 3) {
        Serial.printf("[ERRO] Índice de refeição inválido: %d\n", indiceRefeicao);
        return;
    }
    
    for (int i = 0; i < numRemotas; i++) {
        if (remotas[i].id == idRemota) {
            remotas[i].refeicoes[indiceRefeicao].ultimaExecucao = tempo;
            Serial.printf("[ESTADO] Última execução da remota %d, refeição %d: %s\n",
                         idRemota, indiceRefeicao, tempo.c_str());
            return;
        }
    }
    
    Serial.printf("[ERRO] Remota %d não encontrada para atualizar execução\n", idRemota);
}