#include "mbed.h"
#include "TextLCD.h"

// Definindo as variáveis e ponteiros para arrays dinâmicos
Serial bt(PA_11, PA_12);
Serial pc(USBTX, USBRX);
DigitalIn Button(PC_13);

I2C i2c_lcd(I2C_SDA, I2C_SCL); // Configuração pinos I2C para o display
TextLCD_I2C lcd(&i2c_lcd, 0x7E, TextLCD::LCD20x4); // Endereço 0x7E e display 20x4

Ticker toggle;
Ticker posicaoTicker;

// Definição dos pinos dos motores e dos sensores de fim de curso
DigitalOut CLK_Y(PB_4); 
DigitalOut D_Y(PB_10); 
DigitalOut EN_Y(PA_8); 

DigitalOut CLK_X(PA_10); 
DigitalOut D_X(PB_3);
DigitalOut EN_X(PB_5);

DigitalOut CLK_Z(PA_9);
DigitalOut D_Z(PC_7);
DigitalOut EN_Z(PB_6);

DigitalIn FDC_MIN_Y(PB_15); 
DigitalIn FDC_MAX_Y(PB_1);

DigitalIn FDC_MIN_X(PB_2);
DigitalIn FDC_MAX_X(PB_12);

DigitalIn FDC_MIN_Z(PB_14);
DigitalIn FDC_MAX_Z(PB_13);

InterruptIn BotaoEmergencia(PC_10);

// Definição dos pinos do teclado matricial
DigitalOut col1(PC_9);
DigitalOut col2(PA_6);
DigitalOut col3(PA_5);
DigitalIn row1(PC_4, PullUp);
DigitalIn row2(PC_5, PullUp);
DigitalIn row3(PC_6, PullUp);
DigitalIn row4(PC_8, PullUp);


Timer timerX, timerY, timerZ;  // Temporizadores para cada eixo
int timeout_ms = 200;  


// Declaração das funções
void flip(void);
void iniciarMotores(void);
void moverEixoY(int sentido);
void moverEixoX(int sentido);
void moverEixoZ(int sentido);
void pararEixoZ(void);
void pararEixoY(void);
void pararEixoX(void);
void atualizarPosicoes(void);
void referenciarMotores(void);
void enviarPosicoes(void);
void moverParaPosicoes(void);
void moverParaPosicao(void);
void salvarPosicaoAtual(void);
void salvarPosicaoColeta(void);
void realizarHandshake(void);
void qtdPosicoes(void);
void resetarPosicoesSalvas(void);
void reiniciarPrograma(void);
void emergencia(void);

// Variáveis globais para controle de posição e estado
int posicao_X = 0;
int posicao_Y = 0;
int posicao_Z = 0;
int *posicoes_X = NULL;  // Ponteiros para arrays dinâmicos
int *posicoes_Y = NULL;
int *posicoes_Z = NULL;
int num_posicoes_salvas = 0;
int tamanho_array = 0;
bool posicao_salva = false;  
bool handshake_completo = false;
int posicao_coleta_X = 0;
int posicao_coleta_Y = 0;
int posicao_coleta_Z = 0;
bool posicao_coleta_salva = false;

bool referenciamento_concluido = false;
bool emergencia_ativada = false;

bool eixo_x_neutro = true; // Indica se o eixo X está em posição neutra
bool eixo_y_neutro = true; // Indica se o eixo Y está em posição neutra
bool eixo_z_neutro = true;

char ch = ' ';

// Função principal
int main() {
    lcd.setBacklight(TextLCD::LightOn);
    lcd.setCursor(TextLCD::CurOff_BlkOff);

    // Configura interrupção para o botão de emergência com debounce
    emergencia();
    BotaoEmergencia.fall(&reiniciarPrograma);

    timerX.start();
    timerY.start();
    timerZ.start();

    while (true) {

        iniciarMotores();
        toggle.attach(&flip, 0.0015); // Intervalo de 1/1000 de segundo

        bt.baud(9600); // Configuração da frequência de funcionamento

        while (!handshake_completo) {
            
            realizarHandshake();
        }

        if (handshake_completo) {
            lcd.cls();
            lcd.locate(0,0);
            lcd.printf("Iniciar Referenciamento");

            // Loop principal de operação dos motores
            while (!referenciamento_concluido) {
                if (bt.readable()) {
                    ch = bt.getc();


                    if (ch == 'S') referenciarMotores();
                    // Controle do eixo X
                    if (ch == 'D') {         // Comando para mover X à direita
                        moverEixoX(1);
                    } else if (ch == 'E') {  // Comando para mover X à esquerda
                        moverEixoX(0);
                    } else if (ch == 'N') {  // Comando de parada para o eixo X
                        pararEixoX();
                    }

                    // Controle do eixo Y
                    if (ch == 'C') {         // Comando para mover Y para cima
                        moverEixoY(1);
                    } else if (ch == 'B') {  // Comando para mover Y para baixo
                        moverEixoY(0);
                    } else if (ch == 'M') {  // Comando de parada para o eixo Y
                        pararEixoY();
                    }

                    // Controle do eixo Z
                    if (ch == 'U') {         // Comando para mover Z para cima
                        moverEixoZ(1);
                    } else if (ch == 'Z') {  // Comando para mover Z para baixo
                        moverEixoZ(0);
                    } else if (ch == 'K') {  // Comando de parada para o eixo Z
                        pararEixoZ();
                    }
                }
        }
    }

        }
        // NN ESTA FUNCIONANDO AINDA, ESTAVA FAZENDO A PARTE DO REFERENCIAMENTO, Q TBM N ESTA FUNCIONANDO
        while (true) {
            if (tamanho_array == 0) {
                qtdPosicoes();
                if (tamanho_array > 0) {
                    posicoes_X = new int[tamanho_array]();  // Aloca o array de posições dinamicamente
                    posicoes_Y = new int[tamanho_array]();
                }
            }
            
            if (bt.readable()) {
                ch = bt.getc();
                if (ch == 'S') {  // Sai do loop ao receber "S" pelo Bluetooth
                    referenciarMotores();
                    ch = ' ';
                }

                // Controle eixo X
                if (ch == 'D') {  // Comando para mover à direita
                    moverEixoX(1);  
                    ch = ' ';
                    //eixo_x_neutro = false;
                } else if (ch == 'E') {  // Comando para mover à esquerda
                    moverEixoX(0);  
                    ch = ' ';
                    //eixo_x_neutro = false;
                } else if (ch == 'N') {  // Comando de parada para o eixo X
                    EN_X = 1;  
                    ch = ' ';
                    //eixo_x_neutro = true;
                }

                // Controle do eixo Y
                if (ch == 'C') {  // Comando para mover para cima
                    moverEixoY(1);  
                    ch = ' ';

                } else if (ch == 'B') {  // Comando para mover para baixo
                    moverEixoY(0);  
                    ch = ' ';

                } else if (ch == 'M') {  // Comando de parada para o eixo Y
                    EN_Y = 1;  
                    ch = ' ';

                }

                // Controle do eixo Z (se aplicável)
                if (ch == 'U') {  // Comando para mover Z para cima
                    moverEixoZ(1);  
                    ch = ' ';

                } else if (ch == 'D') {  // Comando para mover Z para baixo
                    moverEixoZ(0);  
                    ch = ' ';

                } else if (ch == 'K') {  // Comando de parada para o eixo Z
                    EN_Z = 1;  
                    ch = ' ';

                }
                // Comandos adicionais
                if (referenciamento_concluido) {
                    if (ch == 'S') {
                        wait_ms(100);
                        if (!posicao_coleta_salva) salvarPosicaoColeta();
                        else salvarPosicaoAtual();
                    }
                    if (ch == 'G') moverParaPosicoes();
                }
            }
        }
    }


// Funções de controle do sistema
void iniciarMotores() {
    EN_X = 1;
    EN_Y = 1;
    EN_Z = 1;
}


void moverEixoX(int sentido) {
    D_X = sentido;
    EN_X = 0;
    timerX.reset();  // Reinicia o temporizador
    lcd.locate(0, 1);
}

void moverEixoY(int sentido) {
    D_Y = sentido;
    EN_Y = 0;
    timerY.reset();  // Reinicia o temporizador
    lcd.locate(0, 2);
}

void moverEixoZ(int sentido) {
    D_Z = sentido;
    EN_Z = 0;
    timerZ.reset();  // Reinicia o temporizador
    lcd.locate(0, 3);
}
// Funções auxiliares para controle de posição
void flip(void) {
    atualizarPosicoes();
}

void atualizarPosicoes() {
    CLK_X = !CLK_X;
    CLK_Y = !CLK_Y;

    if (EN_X == 0) posicao_X += (D_X == 1) ? 1 : -1;
    if (EN_Y == 0) posicao_Y += (D_Y == 1) ? -1 : 1;
}

void referenciarMotores() {
    lcd.cls();
    lcd.printf("Aguardando 'S' p/ Referenc.");

    // Inicia o processo de referenciamento após receber "S"
    int estado = 0;
    int EST_Y = 0;
    int EST_X = 0;
    int EST_Z = 0;
    int animacao_pontos = 0;

    lcd.cls();
    lcd.printf("Referenciando");

    while (estado != 3) {
        lcd.locate(14, 0);
        switch (animacao_pontos % 4) {
            case 0: lcd.printf("   "); break;
            case 1: lcd.printf(".  "); break;
            case 2: lcd.printf(".. "); break;
            case 3: lcd.printf("..."); break;
        }
        animacao_pontos++;
        wait_ms(300);

        switch (estado) {
            case 0:
                if (EST_Y == 0) {
                    moverEixoZ(1);
                    if (FDC_MAX_Z == 0) EST_Z = 1;
                } else if (EST_Z == 1) {
                    toggle.attach(&flip, 0.0045); 
                    moverEixoZ(0);
                    wait_ms(800);
                    EST_Z = 2;
                } else {
                    toggle.attach(&flip, 0.0015); 
                    EN_Z = 1;
                    posicao_Z = 0;
                    estado = 1;
                }
                break;
            case 1:
                if (EST_X == 0) {
                    moverEixoX(1);
                    if (FDC_MAX_X == 0) EST_X = 1;
                } else if (EST_X == 1) {
                    toggle.attach(&flip, 0.0045); 
                    moverEixoX(0);
                    wait_ms(1000);
                    EST_X = 2;
                } else {
                    toggle.attach(&flip, 0.0015); 
                    EN_X = 1;
                    posicao_X = 0;
                    estado = 2;
                }
                break;
            case 2:
                if (EST_Y == 0) {
                    moverEixoY(1);
                    if (FDC_MAX_Y == 0) EST_Y = 1;
                } else if (EST_Y == 1) {
                    toggle.attach(&flip, 0.0045); 
                    moverEixoY(0);
                    wait_ms(800);
                    EST_Y = 2;
                } else {
                    toggle.attach(&flip, 0.0015); 
                    EN_Y = 1;
                    posicao_Y = 0;
                    estado = 3;
                }
                break;
        }
    }

    lcd.cls();
    lcd.locate(0, 0);
    lcd.printf("Referenciamento");
    lcd.locate(0, 1);
    lcd.printf("completo!");
    referenciamento_concluido = true;
    wait_ms(2000);
}

// Funções de salvamento e resgate de posições
void salvarPosicaoColeta() {
    posicao_coleta_X = posicao_X;
    posicao_coleta_Y = posicao_Y;
    posicao_coleta_salva = true;

    lcd.locate(0, 2);
    lcd.printf("Coleta salva:");
    lcd.locate(0, 3);
    lcd.printf("X=%d, Y=%d", posicao_coleta_X, posicao_coleta_Y);
    
    wait_ms(2000);
    lcd.cls();

    // Instrução para a primeira posição de liberação
    lcd.locate(0, 2);
    lcd.printf("Va para posicao 1 deliberacao e salve");
}

void salvarPosicaoAtual() {
    if (num_posicoes_salvas < tamanho_array) {
        // Salva a posição atual
        posicoes_X[num_posicoes_salvas] = posicao_X;
        posicoes_Y[num_posicoes_salvas] = posicao_Y;
        posicoes_Z[num_posicoes_salvas] = posicao_Z;

        // Confirmação rápida de posição salva nas linhas 3 e 4
        lcd.locate(0, 0); // Linha 3
        lcd.printf("Pos %d salva!", num_posicoes_salvas + 1);

        num_posicoes_salvas++;

        // Atualiza as instruções nas linhas 1 e 2 para a próxima posição ou para iniciar pipetagem
        if (num_posicoes_salvas < tamanho_array) {
            lcd.locate(0, 2); // Linha 1
            lcd.printf("Va para posicao %d de", num_posicoes_salvas+1);
            lcd.locate(0, 3);
            lcd.printf("de liberacao");

        } else {
            // Exibe a pergunta para iniciar o processo de pipetagem
            lcd.cls();
            lcd.printf("Iniciar pipetagem?");
            lcd.locate(0, 1);
            lcd.printf("Envie comando 'G'");
        }
    } else {
        lcd.cls();
        lcd.printf("Limite de %d posicoes alcancado.", tamanho_array);
        wait_ms(1000);
    }
}

// Funções de movimento para posições salvas
void moverParaPosicao(int alvo_X, int alvo_Y) {
    while (abs(posicao_X - alvo_X) > 0) moverEixoX(posicao_X < alvo_X ? 1 : 0);
    EN_X = 1;
    wait_ms(200);

    while (abs(posicao_Y - alvo_Y) > 0) moverEixoY(posicao_Y < alvo_Y ? 0 : 1);
    EN_Y = 1;
}

void moverParaPosicoes() {
    lcd.cls();
    lcd.printf("Simulando Pipetagem");

    if (!posicao_coleta_salva) {
        lcd.locate(0, 1);
        lcd.printf("Pos de coleta n salva");
        wait_ms(1000);
        return;
    }

    for (int i = 0; i < num_posicoes_salvas; i++) {
        lcd.locate(0, 1);
        lcd.printf("Indo para pos coleta");
        moverParaPosicao(posicao_coleta_X, posicao_coleta_Y);

        lcd.locate(0, 3);
        lcd.printf("Coletando     ");
        for (int j = 0; j < 3; j++) {
            lcd.locate(10 + j, 3);
            lcd.printf(".");
            wait_ms(330);
        }
        lcd.locate(0, 2);
        lcd.printf("                    ");

        lcd.locate(0, 1);
        lcd.printf("                    ");
        lcd.printf("Indo para pos %d", i + 1);
        moverParaPosicao(posicoes_X[i], posicoes_Y[i]);

        lcd.locate(0, 3);
        lcd.printf("Liberando     ");
        for (int j = 0; j < 3; j++) {
            lcd.locate(10 + j, 3);
            lcd.printf(".");
            wait_ms(1666);
        }
        lcd.locate(0, 2);
        lcd.printf("                    ");
    }

    lcd.locate(0, 1);
    lcd.printf("Retornando p origem");
    moverParaPosicao(0, 0);

    lcd.locate(0, 2);
    lcd.printf("Pipetagem completa!");
    wait_ms(2000);

    // Pergunta ao usuário se deseja fazer outra pipetagem
    lcd.cls();
    lcd.printf("Deseja outra");
    lcd.locate(0, 1);
    lcd.printf("pipetagem? (S/N)");

    bool aguardandoResposta = true;
    while (aguardandoResposta) {
        if (bt.readable()) {
            char resposta = bt.getc();
            if (resposta == 'S' || resposta == 's') {
                // Reinicia o ciclo de pipetagem voltando para o segundo while do main
                aguardandoResposta = false;
            } else if (resposta == 'N' || resposta == 'n') {
                // Redefine as variáveis e sai para o ciclo principal
                aguardandoResposta = false;
                resetarPosicoesSalvas();  // Limpa as posições salvas
                tamanho_array = 0;         // Permite redefinir a quantidade de posições
                posicao_coleta_salva = false; // Reseta a posição de coleta salva
                return; // Sai da função e volta para o segundo while no main
            }
        }
        wait_ms(100);
    }
}

// Funções auxiliares de controle e interface
void realizarHandshake() {
    lcd.cls();
    lcd.printf("Aguardando Joystick...");
    while (!handshake_completo) {
        if (bt.readable()) {
            ch = bt.getc();
            if (ch == 'P') {  // Recebeu "P" do joystick
                handshake_completo = true;
                lcd.cls();
                lcd.printf("Joystick Conectado!");
                wait_ms(2000);  // Mostra a mensagem por 2 segundos
                lcd.cls();
            }
        }
    }
}

void qtdPosicoes() {
    if (tamanho_array > 0) return;

    const char keys[4][3] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'*', '0', '#'}
    };

    lcd.cls();
    lcd.locate(0, 0);
    lcd.printf("Salvar quantas pos?");
    int numero_digitado = 0;
    bool confirmacao = false;

    while (!confirmacao) {
        for (int col = 0; col < 3; col++) {
            col1 = (col == 0) ? 0 : 1;
            col2 = (col == 1) ? 0 : 1;
            col3 = (col == 2) ? 0 : 1;

            if (row1 == 0 || row2 == 0 || row3 == 0 || row4 == 0) {
                char tecla = ' ';
                if (row1 == 0) tecla = keys[0][col];
                if (row2 == 0) tecla = keys[1][col];
                if (row3 == 0) tecla = keys[2][col];
                if (row4 == 0) tecla = keys[3][col];

                if (tecla >= '0' && tecla <= '9') {
                    int digito = tecla - '0';
                    if (numero_digitado < 100) {  
                        numero_digitado = numero_digitado * 10 + digito;
                        lcd.locate(0, 1);
                        lcd.printf("Qtd: %d  ", numero_digitado);  
                    }
                } else if (tecla == '*') {  
                    numero_digitado /= 10;
                    lcd.locate(0, 1);
                    lcd.printf("Qtd: %d  ", numero_digitado);
                } else if (tecla == '#') {  
                    confirmacao = true;
                    tamanho_array = numero_digitado;  
                    lcd.cls();
                    lcd.locate(0, 0);
                    lcd.printf("Qtd salva: %d", tamanho_array);
                    wait_ms(2000);  
                    lcd.cls();
                    lcd.locate(0, 0);
                    lcd.printf("Mova para posicao");
                    lcd.locate(0, 1);
                    lcd.printf("de coleta e salve");
                }

                wait_ms(200);  
            }
        }

        col1 = 1;
        col2 = 1;
        col3 = 1;

        wait_ms(100);  
    }
}



void resetarPosicoesSalvas() {
    delete[] posicoes_X;
    delete[] posicoes_Y;

    posicoes_X = new int[tamanho_array];
    posicoes_Y = new int[tamanho_array];
    num_posicoes_salvas = 0;

    lcd.cls();
}


void reiniciarPrograma() {
    // Resetar variáveis e liberar memória
    delete[] posicoes_X;
    delete[] posicoes_Y;
    posicoes_X = NULL;
    posicoes_Y = NULL;
    num_posicoes_salvas = 0;
    tamanho_array = 0;
    posicao_coleta_salva = false;
    
    // Marcar o referenciamento como não concluído para reiniciar do início do main
    referenciamento_concluido = false;
    
    // Reseta também o estado de emergência
    emergencia_ativada = false;

    // Limpa o display para a nova inicialização
    lcd.cls();
    NVIC_SystemReset();
    // Nota: Como estamos no `while(true)` do main, ao terminar essa função, ele vai começar o ciclo novamente.
}

void emergencia() {
    if (BotaoEmergencia == 0) {
        emergencia_ativada = true;

        lcd.cls();
        lcd.printf("EMERGENCIA ATIVADA");

        // Desativa os motores imediatamente
        EN_X = 1; // Desativa o motor X
        EN_Y = 1; // Desativa o motor Y

        // Aguarda o botão ser solto após ativação de emergência
        while (BotaoEmergencia == 0) {
            wait_ms(100);  // Espera o botão ser solto
        }

        // Exibe instrução para reset
        lcd.cls();
        lcd.printf("Acione e solte o");
        lcd.locate(0, 1);
        lcd.printf("botao para resetar");

        // Aguarda o botão ser pressionado novamente para confirmar o reset
        bool aguardandoConfirmacao = true;
        while (aguardandoConfirmacao) {
            if (BotaoEmergencia == 0) { // Pressionado novamente
                lcd.cls();
                lcd.printf("Solte para resetar");

                while (BotaoEmergencia == 0) {
                    wait_ms(100); // Espera o botão ser solto
                }

                aguardandoConfirmacao = false;
            }
            wait_ms(100);
        }

        referenciamento_concluido = false;
        emergencia_ativada = false;

        lcd.cls();
        lcd.printf("Reiniciando...");
        wait_ms(2000);

          // Reinicia o microcontrolador completamente
    }
}

void pararEixoX() {
    EN_X = 1;
}

void pararEixoY() {
    EN_Y = 1;
}

void pararEixoZ() {
    EN_Z = 1;
}