#include "mbed.h"

// Definindo pinos e variáveis
AnalogIn eixo_x(PA_0);
AnalogIn eixo_y(PA_1);

Serial bt(PA_11, PA_12); // Comunicação serial Bluetooth
Serial pc(USBTX, USBRX);

InterruptIn BotaoSim(PA_10); // Botão Sim com interrupção
DigitalIn BotaoNao(PB_3);    // Botão Não (sem interrupção)

Ticker tickerP; // Ticker para enviar "P" periodicamente

void enviarComandoSim(void);
void enviarComandoP(void);
void leituraBT(void);

char ch;
bool handshake_completo = false;
bool eixo_x_neutro = true; // Indica se o eixo X está em posição neutra
bool eixo_y_neutro = true; // Indica se o eixo Y está em posição neutra

int main() {
    bt.baud(9600); // Configura a frequência do Bluetooth

    // Configuração da interrupção para o botão "Sim"
    BotaoSim.rise(&enviarComandoSim); // Envia o comando "S" ao pressionar o botão "Sim"

    // Configura o Ticker para enviar "P" a cada 1 segundo
    tickerP.attach(&enviarComandoP, 1.0); // Intervalo de 1 segundo

    int x, y;

    while (true) {
        x = eixo_x.read() * 1000; // Converte a leitura para um valor entre 0 e 1000
        y = eixo_y.read() * 1000;

        // Removido: bt.printf("P");

        // Controle eixo X
        if (x > 550 && eixo_x_neutro) { // Mover para a direita
            bt.printf("D");
            eixo_x_neutro = false;
        } else if (x < 450 && eixo_x_neutro) { // Mover para a esquerda
            bt.printf("E");
            eixo_x_neutro = false;
        } else if (x >= 450 && x <= 550 && !eixo_x_neutro) { // Parar eixo X
            bt.printf("N");
            eixo_x_neutro = true;
        }

        // Controle eixo Y
        if (y > 550 && eixo_y_neutro) { // Mover para cima
            bt.printf("C");
            eixo_y_neutro = false;
        } else if (y < 450 && eixo_y_neutro) { // Mover para baixo
            bt.printf("B");
            eixo_y_neutro = false;
        } else if (y >= 450 && y <= 550 && !eixo_y_neutro) { // Parar eixo Y
            bt.printf("M");
            eixo_y_neutro = true;
        }

        // Envia comando "X" ao pressionar o botão "Não"
        if (BotaoNao == 1) {
            bt.printf("X");
        }

        wait_ms(50);  // Intervalo de controle para o joystick
    }
}

void enviarComandoSim() {
    pc.printf("S");
    bt.printf("S");  // Envia o comando "S" via Bluetooth quando o botão "Sim" é pressionado
}

void enviarComandoP() {
    bt.printf("P");  // Envia o comando "P" periodicamente via Ticker
}

void leituraBT() {
    if (handshake_completo) {
        char letra = bt.getc();
        while (letra != '\n') {
            letra = bt.getc();
        }
    } else {
        ch = bt.getc();
    }
}
