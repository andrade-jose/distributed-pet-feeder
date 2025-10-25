// controlador_alimentacao.cpp
#include "controlador_alimentacao.h"
#include "gerenciador_mqtt.h"
#include "sistema_estado.h"

// ✅ CORREÇÃO: Usar pointer (declaração consistente)
extern GerenciadorMQTT* gerenciadorMQTT;
extern EstadoSistema estadoSistema;

bool ControladorAlimentacao::_inicializado = false;
unsigned long ControladorAlimentacao::_ultimaExecucao = 0;
int ControladorAlimentacao::_ultimaHoraExecutada = -1;
int ControladorAlimentacao::_ultimoMinutoExecutado = -1;

void ControladorAlimentacao::inicializar() {
    if (_inicializado) {
        return;
    }
    
    Serial.println("=== Controlador de Alimentação Inicializado ===");
    _inicializado = true;
}

void ControladorAlimentacao::atualizar() {
    if (!_inicializado) {
        return;
    }
    
    static unsigned long ultimaVerificacao = 0;
    unsigned long agora = millis();
    
    if (agora - ultimaVerificacao >= 60000) {
        ultimaVerificacao = agora;
        verificarHorariosAlimentacao();
    }
}

void ControladorAlimentacao::verificarHorariosAlimentacao() {
    DadosTempo tempoAtual = GerenciadorTempo::obterTempoAtual();
    
    if (tempoAtual.hora == _ultimaHoraExecutada && 
        tempoAtual.minuto == _ultimoMinutoExecutado) {
        return;
    }
    
    bool executouAlimentacao = false;
    
    for (int i = 0; i < estadoSistema.numRemotas; i++) {
        for (int j = 0; j < 3; j++) {
            auto& refeicao = estadoSistema.remotas[i].refeicoes[j];
            
            if (tempoAtual.hora == refeicao.hora && 
                tempoAtual.minuto == refeicao.minuto) {
                
                executarAlimentacao(estadoSistema.remotas[i].id, j);
                executouAlimentacao = true;
            }
        }
    }
    
    if (executouAlimentacao) {
        _ultimaHoraExecutada = tempoAtual.hora;
        _ultimoMinutoExecutado = tempoAtual.minuto;
        _ultimaExecucao = millis();
    }
}

void ControladorAlimentacao::executarAlimentacao(int idRemota, int indiceRefeicao) {
    Serial.printf("[ALIMENTAÇÃO] Executando refeição %d para remota %d\n", 
                  indiceRefeicao, idRemota);
    
    // ✅ CORREÇÃO: Verificar pointer antes de usar
    if (gerenciadorMQTT && gerenciadorMQTT->estaConectado()) {
        String topico = "alimentador/remota/" + String(idRemota) + "/alimentar";
        String payload = String(indiceRefeicao);
        
        if (gerenciadorMQTT->publicar(topico, payload)) {
            Serial.printf("✓ Comando enviado: %s -> %s\n", topico.c_str(), payload.c_str());
            
            String tempoExecucao = GerenciadorTempo::obterTempoFormatado();
            estadoSistema.atualizarUltimaExecucao(idRemota, indiceRefeicao, tempoExecucao);
        } else {
            Serial.println("✗ Falha ao enviar comando MQTT");
        }
    } else {
        Serial.println("✗ MQTT não conectado, não foi possível enviar comando");
    }
}