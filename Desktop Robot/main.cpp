#include "mbed.h"
#include "TextLCD.h"

// Definindo as variáveis e ponteiros para arrays dinâmicos
Serial bt(PA_11, PA_12);
DigitalIn Button(PC_13);

I2C i2c_lcd(I2C_SDA, I2C_SCL);  // Configura pinos I2C
TextLCD_I2C lcd(&i2c_lcd, 0x7E, TextLCD::LCD20x4); // Endereço 0x7E e display 20x4

Ticker toggle;
Ticker posicaoTicker;

// Definição dos pinos dos motores e dos sensores de fim de curso
DigitalOut CLK_Y(PA_6); 
DigitalOut D_Y(PA_5); 
DigitalOut EN_Y(PA_8); 

DigitalOut CLK_X(PC_7); 
DigitalOut D_X(PB_6);
DigitalOut EN_X(PA_7);

DigitalIn FDC_MIN_Y(PB_4); 
DigitalIn FDC_MAX_Y(PB_5);
DigitalIn FDC_MIN_X(PA_10);
DigitalIn FDC_MAX_X(PB_3);

InterruptIn BotaoEmergencia(PC_10);

// Definição dos pinos do teclado matricial
DigitalOut col1(PB_15);
DigitalOut col2(PB_14);
DigitalOut col3(PB_13);
DigitalIn row1(PC_5, PullUp);
DigitalIn row2(PC_8, PullUp);
DigitalIn row3(PB_2, PullUp);
DigitalIn row4(PB_1, PullUp);

// Declaração das funções
void flip(void);
void iniciarMotores(void);
void moverEixoY(int sentido);
void moverEixoX(int sentido);
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
int *posicoes_X = NULL;  // Ponteiros para arrays dinâmicos
int *posicoes_Y = NULL;
int num_posicoes_salvas = 0;
int tamanho_array = 0;
bool posicao_salva = false;  
bool handshake_completo = false;
int posicao_coleta_X = 0;
int posicao_coleta_Y = 0;
bool posicao_coleta_salva = false;

bool referenciamento_concluido = false;
bool emergencia_ativada = false;
// Função principal
int main() {
    lcd.setBacklight(TextLCD::LightOn);
    lcd.setCursor(TextLCD::CurOff_BlkOff);

    // Configura interrupção para o botão de emergência com debounce
    emergencia();
    BotaoEmergencia.fall(&reiniciarPrograma);
    while (true) {

        iniciarMotores();
        toggle.attach(&flip, 0.0015); // Intervalo de 1/1000 de segundo

        bt.baud(9600); // Configuração da frequência de funcionamento

        while (!handshake_completo) {
            realizarHandshake();
        }

        while (!referenciamento_concluido) {
            referenciarMotores();
        }

        // Loop principal do processo de pipetagem
        while (true) {
            if (tamanho_array == 0) {
                qtdPosicoes();
                if (tamanho_array > 0) {
                    posicoes_X = new int[tamanho_array]();  // Aloca o array de posições dinamicamente
                    posicoes_Y = new int[tamanho_array]();
                }
            }
            
            if (bt.readable()) {
                char ch = bt.getc();  // Lê o caractere recebido via Bluetooth

                if (ch == 'D') moverEixoX(1);  
                else if (ch == 'E') moverEixoX(0);  
                else if (ch == 'N') EN_X = 1;  

                if (ch == 'C') moverEixoY(1);  
                else if (ch == 'B') moverEixoY(0);  
                else if (ch == 'M') EN_Y = 1;  
            
                if (ch == 'S') {
                    wait_ms(100);
                    if (!posicao_coleta_salva) salvarPosicaoColeta();
                    else salvarPosicaoAtual();
                }
                if (ch == 'G') moverParaPosicoes();
            }

            if (!referenciamento_concluido) break;
        }
    }
}
// Funções de controle do sistema
void iniciarMotores() {
    EN_X = 1;
    EN_Y = 1;
}

// Funções de controle dos eixos
void moverEixoY(int sentido) {
    if (sentido == 1 && FDC_MAX_Y != 0) {
        D_Y = sentido;
        EN_Y = 0;
    } else if (sentido == 0 && FDC_MIN_Y != 0) {
        D_Y = sentido;
        EN_Y = 0;
    } else {
        EN_Y = 1;
    }
}

void moverEixoX(int sentido) {
    if (sentido == 1 && FDC_MAX_X != 0) {
        D_X = sentido;
        EN_X = 0;
    } else if (sentido == 0 && FDC_MIN_X != 0) {
        D_X = sentido;
        EN_X = 0;
    } else {
        EN_X = 1;
    }
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
    
        if (!referenciamento_concluido) {
            lcd.cls();
            lcd.printf("Iniciar referenc.?");
            lcd.locate(0, 1);
            lcd.printf("Envie comando 'R'");
            
            // Aguarda o comando "R" para iniciar o referenciamento
            while (true) {
                if (bt.readable()) {
                    char ch = bt.getc();
                    if (ch == 'R') {  // Sai do loop ao receber "R" pelo Bluetooth
                        break;
                    }
                    // Permite movimento dos motores durante a espera
                    if (ch == 'D') moverEixoX(1);  
                    else if (ch == 'E') moverEixoX(0);  
                    else if (ch == 'N') EN_X = 1;  

                    if (ch == 'C') moverEixoY(1);  
                    else if (ch == 'B') moverEixoY(0);  
                    else if (ch == 'M') EN_Y = 1;
                }
            }
        }

        // Inicia o processo de referenciamento após receber "R"
        int estado = 0;
        int EST_Y = 0;
        int EST_X = 0;
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
        wait_ms(2000);

        referenciamento_concluido = true;  // Marca como concluído para não repetir
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
    char ch;
    lcd.cls();
    lcd.printf("Iniciando BT...");

    while (!handshake_completo) {
        bt.printf("C");
        lcd.locate(0, 1);
        lcd.printf("Tentando handshake...");
        if (bt.readable()) {
            ch = bt.getc();
            if (ch == 'P') {
                lcd.cls();
                lcd.locate(0, 0);
                lcd.printf("Pipetudo OK");
                lcd.locate(0, 1);
                lcd.printf("100%% operante");
                handshake_completo = true;
                wait_ms(2000);
            }
        }
        wait_ms(200);
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