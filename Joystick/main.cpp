#include "mbed.h"

// Definindo pinos e variáveis
AnalogIn eixo_x(PA_0);
AnalogIn eixo_y(PA_1);

Serial bt(PA_11, PA_12); // Comunicação serial Bluetooth

InterruptIn BotaoJoystick(PA_5);
InterruptIn BotaoSim(PA_10); // Botão Sim com interrupção

Ticker tickerP; // Ticker para enviar "P" periodicamente

void enviarComandoSim(void);
void enviarComandoP(void);
void trocaEixo(void);

bool controle_eixo_Z = false; // Indica se o eixo Z está sendo controlado

// Variáveis de estado para armazenar o último comando enviado
char ultimo_comando_X = 'N';
char ultimo_comando_Y = 'M';
char ultimo_comando_Z = 'K';

int main() {
    bt.baud(9600); // Configura a frequência do Bluetooth

    // Configuração da interrupção para o botão "Sim" e o botão de troca de eixo
    BotaoSim.rise(&enviarComandoSim); // Envia o comando "S" ao pressionar o botão "Sim"
    BotaoJoystick.rise(&trocaEixo);   // Alterna entre os modos de controle (X/Y ou Z)
    
    // Configura o Ticker para enviar "P" periodicamente para manter a conexão ativa
    tickerP.attach(&enviarComandoP, 0.5);

    int x = 0, y = 0;

    while (true) {
        x = eixo_x.read() * 1000; 
        y = eixo_y.read() * 1000;

        if (!controle_eixo_Z) { // Controle dos eixos X e Y
            // Controle do eixo X
            if (x > 550 && ultimo_comando_X != 'D') { 
                bt.printf("D");  // Comando para mover à direita
                ultimo_comando_X = 'D';
            } else if (x < 450 && ultimo_comando_X != 'E') { 
                bt.printf("E");  // Comando para mover à esquerda
                ultimo_comando_X = 'E';
            } else if (x >= 450 && x <= 550 && ultimo_comando_X != 'N') { 
                bt.printf("N");  // Comando para parar o motor X
                ultimo_comando_X = 'N';
            }

            // Controle do eixo Y
            if (y > 550 && ultimo_comando_Y != 'C') { 
                bt.printf("C");  // Comando para mover para cima
                ultimo_comando_Y = 'C';
            } else if (y < 450 && ultimo_comando_Y != 'B') { 
                bt.printf("B");  // Comando para mover para baixo
                ultimo_comando_Y = 'B';
            } else if (y >= 450 && y <= 550 && ultimo_comando_Y != 'M') { 
                bt.printf("M");  // Comando para parar o motor Y
                ultimo_comando_Y = 'M';
            }

        } else { // Controle do eixo Z isoladamente
            if (y > 550 && ultimo_comando_Z != 'U') { 
                bt.printf("U");  // Comando para Z subir
                ultimo_comando_Z = 'U';
            } else if (y < 450 && ultimo_comando_Z != 'Z') { 
                bt.printf("Z");  // Comando para Z descer
                ultimo_comando_Z = 'Z';
            } else if (y >= 450 && y <= 550 && ultimo_comando_Z != 'K') { 
                bt.printf("K");  // Comando para parar o motor Z
                ultimo_comando_Z = 'K';
            }
        }
        
        wait_ms(100);  // Atraso para enviar os comandos de movimento continuamente
    }
}

void trocaEixo() {
    controle_eixo_Z = !controle_eixo_Z;
    bt.printf("Y");
}

void enviarComandoSim() {
    wait_ms(200);
    bt.printf("S");  
    
}

void enviarComandoP() {
    bt.printf("P");  // Envia "P" periodicamente para manter o handshake ativo
}
