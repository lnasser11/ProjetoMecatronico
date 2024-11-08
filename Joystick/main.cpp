#include "mbed.h"
#include "TextLCD.h"  // Biblioteca para o display I2C (use a biblioteca apropriada)

AnalogIn eixo_x(A0);
AnalogIn eixo_y(A1);

Serial bt(PA_11, PA_12); // Comunicação serial
Serial pc(USBTX, USBRX);

// I2C i2c_lcd(I2C_SDA, I2C_SCL);  // Configura pinos I2C
// TextLCD_I2C lcd(&i2c_lcd, 0x7E, TextLCD::LCD16x2); // Endereço 0x4E e display 16x2

DigitalIn BotaoA(PA_10);
DigitalIn BotaoSalva(PB_3); // botao B
DigitalIn BotaoVai(PB_5); //botao D

void leituraBT(void);

char ch;
int pos_X_recebida = 0;
int pos_Z_recebida = 0;

bool handshake_completo = false;

#define SIZE_OF_BUFFER 32

char buffer[SIZE_OF_BUFFER];  // Buffer para armazenar dados recebidos
int size_buffer = 0;

int main() {
    bt.baud(9600); //frequência de funcionamento (Já configurado para isso, não mudar)
    
    

    // lcd.cls(); // Limpa o display
    // lcd.setBacklight(TextLCD::LightOn);
    // lcd.printf("Iniciando BT");

    
    
    
    while (!handshake_completo) {
        if (bt.readable()) {  // Verifica se há sinais que podem ser lidos

            ch = bt.getc();
            if (ch == 'C') {
                bt.printf("P");

                handshake_completo = true;
            }
        }
        wait_ms(200);

    }

    int x, y;

    
    //bt.attach(&leituraBT);
    while (true) {
        

        x = eixo_y.read() * 1000; // float (0->1) to int (0-1000)
        y = eixo_x.read() * 1000; // float (0->1) to int (0-1000)

        bt.printf("P");
        // Controle eixo X
        if (x > 550) {
            bt.printf("D"); // Movimenta para a direita
        } else if (x < 450) {
            bt.printf("E"); // Movimenta para a esquerda
        } else {
            bt.printf("N"); // Parar eixo X (Neutro)
        }

        // Controle eixo Z
        if (y > 550) {
            bt.printf("C"); // Movimenta para cima
        } else if (y < 450) {
            bt.printf("B"); // Movimenta para baixo
        } else {
            bt.printf("M"); // Parar eixo Z (Neutro)
        }

        // if(size_buffer > 4){
        //     lcd.cls();
        //     lcd.locate(0, 0);  // Primeira linha
        //     lcd.printf("%s", buffer);  // Mostra a posição X
        //     size_buffer = 0;
        //     for (int i = 0; i <= SIZE_OF_BUFFER; i++) { buffer[i] = 0; }
        // }

        if (BotaoA == 0) {
            bt.printf("R");
        }

        if (BotaoSalva == 0) {
            bt.printf("S"); // Comando para salvar posição
        }

        // Verifica se o botão de mover para posição salva foi pressionado
        if (BotaoVai == 0) {
            bt.printf("G"); // Comando para ir para posição salva
        }

        // // Recebe as posições dos motores via Bluetooth
        // if (bt.readable()) {
        //     bt.gets(buffer, sizeof(buffer));

        //     // Faz o parsing dos dados recebidos para extrair as posições
        //     sscanf(buffer, "PX:%d,PZ:%d", &pos_X_recebida, &pos_Z_recebida);

        //     // Atualiza o display com as posições
        //     lcd.cls();  // Limpa o display
        //     lcd.locate(0, 0);  // Primeira linha
        //     lcd.printf("X: %d", pos_X_recebida);  // Mostra a posição X
        //     lcd.locate(0, 1);  // Segunda linha
        //     lcd.printf("Z: %d", pos_Z_recebida);  // Mostra a posição Z
        // }

        wait_ms(50);  // Controla o tempo de resposta do joystick
    }
}

void leituraBT() {
        pc.printf("attach!!!!!!!!!!!");
        if (handshake_completo) {
        char letra = bt.getc();
        while(letra!='\n') {
            pc.printf("\nwhile do attach krl");
            buffer[size_buffer] = letra;
            pc.printf("\nbuffer = letra");
            size_buffer++;
            pc.printf("\nsize buffer");
            letra = bt.getc();
            pc.printf("\nletra bt");
        }
}
else {
ch = bt.getc();
} 
}