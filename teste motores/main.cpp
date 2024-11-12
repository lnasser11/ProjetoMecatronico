#include "mbed.h"

// Definindo os pinos dos motores
DigitalOut CLK_X(PA_10); 
DigitalOut D_X(PB_3);
DigitalOut EN_X(PB_5);

DigitalOut CLK_Y(PB_4); 
DigitalOut D_Y(PB_10); 
DigitalOut EN_Y(PA_8); 

DigitalOut CLK_Z(PA_9);
DigitalOut D_Z(PC_7);
DigitalOut EN_Z(PB_6);

// Configuração do Bluetooth
Serial bt(PA_11, PA_12);  // Comunicação serial com o joystick

// Variável para armazenar o comando recebido
char comando;

void setupMotores() {
    // Configuração inicial para desligar todos os motores
    EN_X = 1;
    EN_Y = 1;
    EN_Z = 1;
}

// Funções para movimentar os motores
void moverX(int sentido) {
    D_X = sentido;  // Define a direção
    EN_X = 0;       // Liga o motor X
}

void pararX() {
    EN_X = 1;       // Desliga o motor X
}

void moverY(int sentido) {
    D_Y = sentido;  // Define a direção
    EN_Y = 0;       // Liga o motor Y
}

void pararY() {
    EN_Y = 1;       // Desliga o motor Y
}

void moverZ(int sentido) {
    D_Z = sentido;  // Define a direção
    EN_Z = 0;       // Liga o motor Z
}

void pararZ() {
    EN_Z = 1;       // Desliga o motor Z
}

// Função principal
int main() {
    bt.baud(9600);       // Define a taxa de comunicação do Bluetooth
    setupMotores();      // Inicializa os motores desligados

    while (true) {
        if (bt.readable()) {
            comando = bt.getc();  // Lê o comando do joystick

            // Controle do motor X
            if (comando == 'D') {           // Comando para mover X à direita
                moverX(1);
            } else if (comando == 'E') {    // Comando para mover X à esquerda
                moverX(0);
            } else if (comando == 'N') {    // Comando para parar o motor X
                pararX();
            }

            // Controle do motor Y
            if (comando == 'C') {           // Comando para mover Y para cima
                moverY(1);
            } else if (comando == 'B') {    // Comando para mover Y para baixo
                moverY(0);
            } else if (comando == 'M') {    // Comando para parar o motor Y
                pararY();
            }

            // Controle do motor Z
            if (comando == 'U') {           // Comando para mover Z para cima
                moverZ(1);
            } else if (comando == 'Z') {    // Comando para mover Z para baixo
                moverZ(0);
            } else if (comando == 'K') {    // Comando para parar o motor Z
                pararZ();
            }
        }
    }
}
