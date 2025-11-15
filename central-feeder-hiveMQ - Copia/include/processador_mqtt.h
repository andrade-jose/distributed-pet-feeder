#pragma once
#include <Arduino.h>
#include "sistema_estado.h"

class ProcessadorMQTT {
public:
    static void inicializar();
    static void processarMensagem(String topico, String mensagem);

    // ✅ NOVO: Processadores SparkplugB
    static void processarDBIRTH(String topico, String payload);
    static void processarDDATA(String topico, String payload);
    static void processarDDEATH(String topico, String payload);
    static void processarNCMD(String topico, String payload);

    // Processadores antigos (manter para compatibilidade temporária)
    static void processarStatus(String mensagem, int remotaId);
    static void processarHeartbeat(String mensagem, int remotaId);
    static void processarAlerta(String mensagem, int remotaId);
    static void processarConfiguracaoDashboard(String mensagem);
    static void processarSolicitacaoConfig(String mensagem);

private:
    static int extrairIdRemota(String topico);
    static int extrairIdRemotaSparkplug(String topico);  // ✅ NOVO
    static String extrairTipoTopico(String topico);
    static void atualizarEstadoRemota(int id, bool online, String status = "");
};

// Callback para o GerenciadorMQTT
extern void onMensagemMQTTRecebida(String topico, String mensagem);