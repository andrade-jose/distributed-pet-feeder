#pragma once
#include <Arduino.h>
#include "sistema_estado.h"

class ProcessadorMQTT {
public:
    static void inicializar();
    static void processarMensagem(String topico, String mensagem);
    static void processarStatus(String mensagem, int remotaId);
    static void processarHeartbeat(String mensagem, int remotaId);
    static void processarAlerta(String mensagem, int remotaId);
    static void processarConfiguracaoDashboard(String mensagem); // NOVO
    static void processarSolicitacaoConfig(String mensagem); // NOVO

private:
    static int extrairIdRemota(String topico);
    static String extrairTipoTopico(String topico);
    static void atualizarEstadoRemota(int id, bool online, String status = "");
};

// Callback para o GerenciadorMQTT
extern void onMensagemMQTTRecebida(String topico, String mensagem);