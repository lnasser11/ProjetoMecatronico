#include "mbed.h"

// Definindo pinos e variáveis
AnalogIn eixo_x(PA_0);
AnalogIn eixo_y(PA_1);

Serial bt(PA_11, PA_12); // Comunicação serial Bluetooth
Serial pc(USBTX, USBRX);

InterruptIn BotaoJoystick(PB_13);
InterruptIn BotaoSim(PA_10); // Botão Sim com interrupção
DigitalIn BotaoNao(PB_3);    // Botão Não (sem interrupção)

Ticker tickerP; // Ticker para enviar "P" periodicamente

void enviarComandoSim(void);
void enviarComandoP(void);
void leituraBT(void);
void trocaEixo(void);

char ch;
bool handshake_completo = false;
bool controle_eixo_Z = false; // Indica se o eixo Z está sendo controlado
bool eixo_x_neutro = true; 
bool eixo_y_neutro = true; 
bool eixo_z_neutro = true; // Estado neutro do eixo Z

int main() {
    bt.baud(9600); // Configura a frequência do Bluetooth

    // Configuração da interrupção para o botão "Sim" e o botão de troca de eixo
    BotaoSim.rise(&enviarComandoSim); // Envia o comando "S" ao pressionar o botão "Sim"
    BotaoJoystick.fall(&trocaEixo); // Alterna entre os modos de controle (X/Y ou Z)

    // Configura o Ticker para enviar "P" a cada 1 segundo
    tickerP.attach(&enviarComandoP, 0.5); // Configura o ticker para 50 ms


    int x, y;

    while (true) {
        x = eixo_x.read() * 1000; 
        y = eixo_y.read() * 1000;

        if (!controle_eixo_Z) {
            // Controle do eixo X
            if (x > 550) { 
                bt.printf("D");  // Envia comando para direita continuamente
                eixo_x_neutro = false;
            } else if (x < 450) { 
                bt.printf("E");  // Envia comando para esquerda continuamente
                eixo_x_neutro = false;
            } else if (!eixo_x_neutro) { 
                bt.printf("N");  // Envia comando de parada apenas uma vez
                eixo_x_neutro = true;
            }

            // Controle do eixo Y
            if (y > 550) { 
                bt.printf("C");  // Envia comando para cima continuamente
                eixo_y_neutro = false;
            } else if (y < 450) { 
                bt.printf("B");  // Envia comando para baixo continuamente
                eixo_y_neutro = false;
            } else if (!eixo_y_neutro) { 
                bt.printf("M");  // Envia comando de parada apenas uma vez
                eixo_y_neutro = true;
            }

        } else { // Controle do eixo Z
            if (y > 550) { 
                bt.printf("U");  // Envia comando para Z subir continuamente
                eixo_z_neutro = false;
            } else if (y < 450) { 
                bt.printf("D");  // Envia comando para Z descer continuamente
                eixo_z_neutro = false;
            } else if (!eixo_z_neutro) { 
                bt.printf("K");  // Envia comando de parada para o eixo Z apenas uma vez
                eixo_z_neutro = true;
            }
        }
    }
}

void trocaEixo() {
    controle_eixo_Z = !controle_eixo_Z;
    bt.printf("TROQUEI");
}

void enviarComandoSim() {
    pc.printf("S");
    bt.printf("S");  
}

void enviarComandoP() {
    bt.printf("P");  // Envia "P" periodicamente para manter o handshake ativo
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
