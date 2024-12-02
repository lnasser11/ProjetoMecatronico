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
Ticker ledTicker;
Ticker redLedTicker;
Ticker BuzzerTicker;

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

// Definindo os sensores de fim de curso
DigitalIn FDC_MAX_Y(PB_1); // Ajustado: Fim de curso máximo para Y
DigitalIn FDC_MIN_Y(PB_15); // Ajustado: Fim de curso mínimo para Y

DigitalIn FDC_MAX_X(PB_12); // Ajustado: Fim de curso máximo para X
DigitalIn FDC_MIN_X(PB_2);  // Ajustado: Fim de curso mínimo para X

DigitalIn FDC_MAX_Z(PB_14);
DigitalIn FDC_MIN_Z(PB_13);

// Adicione essas linhas no início, junto com as outras declarações de pinos.
DigitalOut led_verde(PC_12);
DigitalOut led_vermelho(PA_15);
DigitalOut buzzer(PC_11);

InterruptIn BotaoEmergencia(PC_10);

// Definição dos pinos do teclado matricial
DigitalOut col1(PC_9);
DigitalOut col2(PA_6);
DigitalOut col3(PA_5);
DigitalIn row1(PC_4, PullUp);
DigitalIn row2(PC_5, PullUp);
DigitalIn row3(PC_6, PullUp);
DigitalIn row4(PC_8, PullUp);


DigitalOut pipeta(PA_7);

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
void acionarPipeta(void);
void piscarLedVerde(void);
void alternarLedVermelho(void);
void alternarBuzzer(void);

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

int contador_posicoes_salvas = 0;  // Contador de posições salvas
bool posicoes_completas = false;   // Sinalizador para indicar se todas as posições foram salvas

// Função principal
int main() {

    lcd.setBacklight(TextLCD::LightOn);
    lcd.setCursor(TextLCD::CurOff_BlkOff);

    // Configura interrupção para o botão de emergência com debounce
    emergencia();
    BotaoEmergencia.fall(&reiniciarPrograma);
    pipeta = 1;
    led_verde = 1;

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
            lcd.locate(0, 0);
            lcd.printf("Iniciar Referenciamento");

            // Loop principal de operação dos motores
            while (!referenciamento_concluido) {
                if (bt.readable()) {
                    ch = bt.getc();

                    // Utilizando switch case para controlar os motores e referenciamento
                    switch (ch) {
                        // Comando para iniciar o referenciamento
                        case 'S':
                            referenciarMotores();
                            ch = ' ';
                            break;

                        // Controle do motor X
                        case 'D':            // Comando para mover X à direita
                            moverEixoX(1);
                            ch = ' ';
                            break;
                        case 'E':            // Comando para mover X à esquerda
                            moverEixoX(0);
                            ch = ' ';
                            break;
                        case 'N':            // Comando para parar o motor X
                            pararEixoX();
                            ch = ' ';
                            break;

                        // Controle do motor Y
                        case 'C':            // Comando para mover Y para cima
                            moverEixoY(1);
                            ch = ' ';
                            break;
                        case 'B':            // Comando para mover Y para baixo
                            moverEixoY(0);
                            ch = ' ';
                            break;
                        case 'M':            // Comando para parar o motor Y
                            pararEixoY();
                            ch = ' ';
                            break;

                        // Controle do motor Z
                        case 'U':            // Comando para mover Z para cima
                            moverEixoZ(1);
                            ch = ' ';
                            break;
                        case 'Z':            // Comando para mover Z para baixo
                            moverEixoZ(0);
                            ch = ' ';
                            break;
                        case 'K':            // Comando para parar o motor Z
                            pararEixoZ();
                            ch = ' ';
                            break;

                        // Comando desconhecido (opcional)
                        default:
                            // Caso deseje tratar comandos desconhecidos
                            break;
            }
            ch = ' ';
        }
    }
}
    while (true) {
        
        if (tamanho_array == 0) {
            qtdPosicoes();
            if (tamanho_array > 0) {
                posicoes_X = new int[tamanho_array]();  // Aloca o array de posições dinamicamente
                posicoes_Y = new int[tamanho_array]();
                posicoes_Z = new int[tamanho_array]();
            }
        }

        if (referenciamento_concluido) {
            if (bt.readable()) ch = bt.getc();
            
                switch (ch) {
                    // Comando 'S' para salvar posição de coleta ou próxima posição
                    case 'S':
                        wait_ms(100);
                        if (!posicao_coleta_salva) {
                            // Salva a posição de coleta na primeira vez
                            salvarPosicaoColeta();
                            posicao_coleta_salva = true;  // Marca a posição de coleta como salva
                            lcd.cls();
                            lcd.printf("Va para a pos. 1 de liberacao e salve");

                            ch = ' ';
                        } else if (!posicoes_completas) {
                            // Salva uma nova posição de liberação
                            salvarPosicaoAtual();
                            contador_posicoes_salvas++;  // Incrementa o contador de posições salvas
                            ch = ' ';
                            lcd.cls();
                            lcd.printf("Posicao %d salva", contador_posicoes_salvas);
                            lcd.locate(0,1);
                            lcd.printf("Va para a prox.");

                            // Verifica se todas as posições foram salvas
                            if (contador_posicoes_salvas >= tamanho_array) {
                                posicoes_completas = true;  // Marca que todas as posições foram salvas
                                ch = ' ';
                                lcd.cls();
                                lcd.printf("Todas as posicoes");
                                lcd.locate(0,1);
                                lcd.printf("salvas!");
                                lcd.locate(0,2);
                                lcd.printf("Iniciar pipetagem?");

                            }
                        } else {
                            // Todas as posições foram salvas; agora o comando 'S' inicia o movimento
                            lcd.cls();
                            lcd.printf("Movendo para posicoes");
                            moverParaPosicoes();
                            ch = ' ';
                        }
                        ch = ' ';
                        wait_ms(300);
                        break;

                    // Comando para deletar a última posição salva e permitir que outra seja salva no lugar
                    case 'L':
                        if (contador_posicoes_salvas > 0) {
                            // Remove a última posição de liberação salva
                            contador_posicoes_salvas--;  // Decrementa o contador para "remover" a última posição
                            posicoes_completas = false;  // Define como incompleto para permitir novo salvamento
                            lcd.cls();
                            lcd.printf("Posicao %d removida", contador_posicoes_salvas + 1);
                            wait_ms(1000);
                        } else if (posicao_coleta_salva) {
                            // Se não há posições de liberação, permite apagar a posição de coleta
                            posicao_coleta_salva = false;  // Marca como não salva
                            posicao_coleta_X = 0;
                            posicao_coleta_Y = 0;
                            posicao_coleta_Z = 0;
                            lcd.cls();
                            lcd.printf("Pos de coleta apagada");
                            wait_ms(1000);
                        } else {
                            lcd.cls();
                            lcd.printf("Nenhuma posicao para remover");
                            wait_ms(1000);
                        }
                        ch = ' ';
                        break;


                    // Controle do motor X
                    case 'D':            // Comando para mover X à direita
                        moverEixoX(1);
                        ch = ' ';
                        break;
                    case 'E':            // Comando para mover X à esquerda
                        moverEixoX(0);
                        ch = ' ';
                        break;
                    case 'N':            // Comando para parar o motor X
                        pararEixoX();
                        ch = ' ';
                        break;

                    // Controle do motor Y
                    case 'C':            // Comando para mover Y para cima
                        moverEixoY(1);
                        ch = ' ';
                        break;
                    case 'B':            // Comando para mover Y para baixo
                        moverEixoY(0);
                        ch = ' ';
                        break;
                    case 'M':            // Comando para parar o motor Y
                        pararEixoY();
                        ch = ' ';
                        break;

                    // Controle do motor Z
                    case 'U':            // Comando para mover Z para cima
                        moverEixoZ(1);
                        ch = ' ';
                        break;
                    case 'Z':            // Comando para mover Z para baixo
                        moverEixoZ(0);
                        ch = ' ';
                        break;
                    case 'K':            // Comando para parar o motor Z
                        pararEixoZ();
                        ch = ' ';
                        break;

                    default:
                        // Caso deseje tratar comandos desconhecidos
                        break;
                }
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
}

void moverEixoY(int sentido) {
    D_Y = sentido;
    EN_Y = 0;
}

void moverEixoZ(int sentido) {
    D_Z = sentido;
    EN_Z = 0;
}
// Funções auxiliares para controle de posição

void flip(void) {
    // Verifica o eixo X
    if (D_X == 1 && FDC_MAX_X != 0) {      // Movendo para a direita e sem atingir o fim de curso máximo de X
        CLK_X = !CLK_X;
        if (EN_X == 0) posicao_X += 1;     // Incrementa posição se motor estiver ativo
    } else if (D_X == 0 && FDC_MIN_X != 0) { // Movendo para a esquerda e sem atingir o fim de curso mínimo de X
        CLK_X = !CLK_X;
        if (EN_X == 0) posicao_X -= 1;     // Decrementa posição se motor estiver ativo
    }

    // Verifica o eixo Y
    if (D_Y == 1 && FDC_MAX_Y != 0) {      // Movendo para cima e sem atingir o fim de curso máximo de Y
        CLK_Y = !CLK_Y;
        if (EN_Y == 0) posicao_Y -= 1;     // Decrementa posição se motor estiver ativo
    } else if (D_Y == 0 && FDC_MIN_Y != 0) { // Movendo para baixo e sem atingir o fim de curso mínimo de Y
        CLK_Y = !CLK_Y;
        if (EN_Y == 0) posicao_Y += 1;     // Incrementa posição se motor estiver ativo
    }

    // Verifica o eixo Z
    if (D_Z == 1 && FDC_MAX_Z != 0) {      // Movendo para cima e sem atingir o fim de curso máximo de Z
        CLK_Z = !CLK_Z;
        if (EN_Z == 0) posicao_Z += 1;     // Decrementa posição se motor estiver ativo
    } else if (D_Z == 0 && FDC_MIN_Z != 0) { // Movendo para baixo e sem atingir o fim de curso mínimo de Z
        CLK_Z = !CLK_Z;
        if (EN_Z == 0) posicao_Z -= 1;     // Incrementa posição se motor estiver ativo
    }

    // Exibe as posições dos motores no terminal
}


void referenciarMotores() {
    lcd.cls();
    lcd.printf("Referenciando...");

    // Configura o Ticker para piscar o LED verde
    ledTicker.attach(&piscarLedVerde, 0.5); // LED pisca a cada 500ms

    // Inicia o processo de referenciamento
    int estado = 0;
    int EST_Y = 0;
    int EST_X = 0;
    int EST_Z = 0;

    lcd.locate(0, 1);
    lcd.printf("X: ...");
    lcd.locate(0, 2);
    lcd.printf("Y: ...");
    lcd.locate(0, 3);
    lcd.printf("Z: ...");

    while (estado != 3) {
        switch (estado) {
            case 0: // Referenciando o eixo Z
                if (EST_Z == 0) {
                    moverEixoZ(0); // Define direção para descer
                    if (FDC_MIN_Z == 0) { // Fim de curso mínimo de Z atingido
                        EST_Z = 1; // Passa para o ajuste fino
                    }
                    lcd.locate(0, 3);
                    lcd.printf("Z: ...");
                } else if (EST_Z == 1) {
                    toggle.attach(&flip, 0.0045); // Reduz a velocidade do clock
                    moverEixoZ(1);                // Move lentamente para cima
                    wait_ms(1000);                // Aguarda um curto período
                    toggle.attach(&flip, 0.0015); // Restaura a velocidade normal
                    EN_Z = 1;                     // Desativa o motor Z
                    posicao_Z = 0;                // Marca a posição como zero
                    estado = 1;                   // Passa para o próximo estado
                    lcd.locate(0, 3);
                    lcd.printf("Z: Referenciado!");
                }
                break;

            case 1: // Referenciando o eixo X
                if (EST_X == 0) {
                    moverEixoX(1); // Define direção para a direita
                    if (FDC_MAX_X == 0) { // Fim de curso máximo de X atingido
                        EST_X = 1; // Passa para o ajuste fino
                    }
                    lcd.locate(0, 1);
                    lcd.printf("X: ...");
                } else if (EST_X == 1) {
                    toggle.attach(&flip, 0.0045); // Reduz a velocidade do clock
                    moverEixoX(0);                // Move lentamente para a esquerda
                    wait_ms(1000);                // Aguarda um curto período
                    toggle.attach(&flip, 0.0015); // Restaura a velocidade normal
                    EN_X = 1;                     // Desativa o motor X
                    posicao_X = 0;                // Marca a posição como zero
                    estado = 2;                   // Passa para o próximo estado
                    lcd.locate(0, 1);
                    lcd.printf("X: Referenciado!");
                }
                break;

            case 2: // Referenciando o eixo Y
                if (EST_Y == 0) {
                    moverEixoY(1); // Define direção para cima
                    if (FDC_MAX_Y == 0) { // Fim de curso máximo de Y atingido
                        EST_Y = 1; // Passa para o ajuste fino
                    }
                    lcd.locate(0, 2);
                    lcd.printf("Y: ...");
                } else if (EST_Y == 1) {
                    toggle.attach(&flip, 0.0045); // Reduz a velocidade do clock
                    moverEixoY(0);                // Move lentamente para baixo
                    wait_ms(1000);                // Aguarda um curto período
                    toggle.attach(&flip, 0.0015); // Restaura a velocidade normal
                    EN_Y = 1;                     // Desativa o motor Y
                    posicao_Y = 0;                // Marca a posição como zero
                    estado = 3;                   // Referenciamento concluído
                    lcd.locate(0, 2);
                    lcd.printf("Y: Referenciado!");
                }
                break;
        }

        wait_ms(100); // Aguarda para evitar sobrecarga no loop
    }

    // Para o Ticker e o LED verde
    ledTicker.detach();
    led_verde = 1;

    // Emite som no buzzer para indicar conclusão
    buzzer = 1;
    wait_ms(500);
    buzzer = 0;

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
    posicao_coleta_Z = posicao_Z;
    posicao_coleta_salva = true;

    lcd.cls();
    lcd.locate(0, 0);
    lcd.printf("Coleta salva:");
    lcd.locate(0, 1);
    lcd.printf("X=%d", posicao_coleta_X);
    lcd.locate(0,2);
    lcd.printf("Y=%d", posicao_coleta_Y);
    lcd.locate(0,3);
    lcd.printf("Z=%d", posicao_coleta_Z);
    
    wait_ms(2000);
    lcd.cls();
}

void salvarPosicaoAtual() {
    if (num_posicoes_salvas < tamanho_array) {
        // Salva a posição atual
        posicoes_X[num_posicoes_salvas] = posicao_X;
        posicoes_Y[num_posicoes_salvas] = posicao_Y;
        posicoes_Z[num_posicoes_salvas] = posicao_Z;

        // Confirmação rápida de posição salva nas linhas 3 e 4
        lcd.cls();
        lcd.locate(0, 0); // Linha 3
        lcd.printf("Pos %d salva!", num_posicoes_salvas + 1);
        lcd.locate(0,1);
        lcd.printf("X:%d", posicao_X);
        lcd.locate(0,2);
        lcd.printf("Y:%d", posicao_Y);
        lcd.locate(0,3);
        lcd.printf("Z:%d", posicao_Z);

        num_posicoes_salvas++;

        // Atualiza as instruções nas linhas 1 e 2 para a próxima posição ou para iniciar pipetagem
        if (num_posicoes_salvas < tamanho_array) {
            lcd.cls();
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
        lcd.cls();
        wait_ms(1000);
    }
}

void moverEixoZParaZero() {
    // Move o eixo Z para a posição 0
    while (posicao_Z > 0) moverEixoZ(0);
    EN_Z = 1;
    wait_ms(200);
}

void moverParaPosicao(int alvo_X, int alvo_Y, int alvo_Z) {
    // Move o eixo X e Y para as coordenadas alvo
    while (abs(posicao_X - alvo_X) > 0) moverEixoX(posicao_X < alvo_X ? 1 : 0);
    EN_X = 1;
    wait_ms(200);

    while (abs(posicao_Y - alvo_Y) > 0) moverEixoY(posicao_Y < alvo_Y ? 0 : 1);
    EN_Y = 1;
    wait_ms(200);

    // Finalmente, desce o eixo Z para a posição alvo de coleta ou liberação
    while (posicao_Z < alvo_Z) moverEixoZ(1);
    EN_Z = 1;
    wait_ms(200);
}

void moverParaPosicoes() {
    lcd.cls(); // Limpa a tela antes de exibir a mensagem inicial
    lcd.printf("Defina o volume:");
    lcd.locate(0, 1);
    lcd.printf("1 a 5 ml");

    int ml_selecionado = 0;
    bool confirmacao = false;

    // Perguntar o volume de pipetagem
    while (!confirmacao) {

         const char keys[4][3] = {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'*', '0', '#'}
    };
    
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

                if (tecla >= '1' && tecla <= '5') {  // Aceita apenas 1 a 5 ml
                    ml_selecionado = tecla - '0';
                    lcd.locate(0, 2);
                    lcd.printf("Selecionado: %d ml", ml_selecionado);
                } else if (tecla == '#') {  // Confirmação do valor
                    confirmacao = true;
                    lcd.cls();
                    lcd.printf("Volume definido:");
                    lcd.locate(0, 1);
                    lcd.printf("%d ml", ml_selecionado);
                    wait_ms(2000);
                }

                wait_ms(200);
            }
        }

        col1 = 1;
        col2 = 1;
        col3 = 1;

        wait_ms(100);
    }

    // Inicia o Ticker para piscar o LED verde
    ledTicker.attach(&piscarLedVerde, 0.5); // LED pisca a cada 500ms

    // Ciclos baseados no volume selecionado
    for (int ciclo = 0; ciclo < ml_selecionado; ciclo++) {
        for (int i = 0; i < num_posicoes_salvas; i++) {
            lcd.cls();
            lcd.locate(0, 0);
            lcd.printf("Pipetando...");
            lcd.locate(0, 1);
            lcd.printf("Ciclo %d de %d", ciclo + 1, ml_selecionado); // Exibe o ciclo correto

            // Subindo para Z=0
            lcd.locate(0, 2);
            lcd.printf("Subindo para Z=0");
            moverEixoZParaZero();

            // Indo para posição de coleta
            lcd.cls();
            lcd.locate(0, 2);
            lcd.printf("Indo para coleta");
            moverParaPosicao(posicao_coleta_X, posicao_coleta_Y, 0);

            // Descendo para posição de coleta
            lcd.cls();
            lcd.locate(0, 2);
            lcd.printf("Descendo para coleta");
            while (posicao_Z < posicao_coleta_Z) moverEixoZ(1);
            EN_Z = 1;
            wait_ms(500);

            // Coletando
            lcd.cls();
            lcd.locate(0, 2);
            lcd.printf("Coletando...");
            acionarPipeta();

            // Subindo para Z=0
            lcd.cls();
            lcd.locate(0, 2);
            lcd.printf("Subindo para Z=0");
            moverEixoZParaZero();

            // Indo para posição de liberação
            lcd.cls();
            lcd.locate(0, 2);
            lcd.printf("Indo para lib pos %d", i + 1);
            moverParaPosicao(posicoes_X[i], posicoes_Y[i], 0);

            // Descendo para posição de liberação
            lcd.cls();
            lcd.locate(0, 2);
            lcd.printf("Descendo p/ posicao");
            lcd.locate(0, 3);
            lcd.printf("de lib. %d", i + 1);
            while (posicao_Z < posicoes_Z[i]) moverEixoZ(1);
            EN_Z = 1;
            wait_ms(500);

            // Liberando
            lcd.cls();
            lcd.locate(0, 2);
            lcd.printf("Liberando...");
            acionarPipeta();

            // Subindo para Z=0
            lcd.cls();
            lcd.locate(0, 2);
            lcd.printf("Subindo para Z=0");
            moverEixoZParaZero();
        }
    }

    // Retornando à origem
    lcd.cls();
    lcd.printf("Retornando p/ origem");
    moverEixoZParaZero();
    moverParaPosicao(0, 0, 0);

    // Para o Ticker e o LED verde
    ledTicker.detach();
    led_verde = 1;

    // Emite som no buzzer para indicar conclusão
    buzzer = 1;
    wait_ms(200);
    buzzer = 0;
    wait_ms(200);
    buzzer = 1;
    wait_ms(200);
    buzzer = 0;
    wait_ms(200);
    buzzer = 1;
    wait_ms(200);
    buzzer = 0;

    lcd.cls(); // Mensagem final
    lcd.printf("Pipetagem completa!");
    wait_ms(2000);

    // Pergunta se o usuário deseja repetir
    lcd.cls();
    lcd.printf("Deseja pipetar");
    lcd.locate(0, 1);
    lcd.printf("nas mesmas pos.?");
    lcd.locate(0, 2);
    lcd.printf("Azul p/ sim,");
    lcd.locate(0, 3);
    lcd.printf("Vermelho p novas pos");

    bool aguardandoResposta = true;

    while (aguardandoResposta) {
        if (bt.readable()) {
            char resposta = bt.getc();
            if (resposta == 'S') {
                // Inicia o processo de pipetagem novamente
                aguardandoResposta = false;
                moverParaPosicoes(); // Recomeça a pipetagem
            } else if (resposta == 'L') {
                // Reinicia o processo desde o início
                aguardandoResposta = false;
                reiniciarPrograma();
                return; // Sai da função e volta ao início do programa
            }
        }
        wait_ms(100);
    }
}




// Funções auxiliares de controle e interface
void realizarHandshake() {
    lcd.cls();
    lcd.locate(0, 0);
    lcd.printf("Aguardando");
    lcd.locate(0, 1);
    lcd.printf("Joystick");

    redLedTicker.attach(&alternarLedVermelho, 1.0);

    int animacao_pontos = 0; // Contador para a animação dos pontos

    while (!handshake_completo) {
        // Animação dos três pontos
        lcd.locate(9, 1); // Posição após "Joystick"
        switch (animacao_pontos % 4) {
            case 0: lcd.printf("   "); break; // Sem pontos
            case 1: lcd.printf(".  "); break; // Um ponto
            case 2: lcd.printf(".. "); break; // Dois pontos
            case 3: lcd.printf("..."); break; // Três pontos
        }
        animacao_pontos++;
        wait_ms(300); // Intervalo da animação

        // Verifica se o joystick respondeu
        if (bt.readable()) {
            ch = bt.getc();
            if (ch == 'P') {  // Recebeu "P" do joystick
                handshake_completo = true;
                redLedTicker.detach();
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
    lcd.locate(0,2);
    lcd.printf("# p/ confirmar");
    lcd.locate(0,3);
    lcd.printf("* p/ apagar");
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
    delete[] posicoes_Z;

    posicoes_X = NULL;
    posicoes_Y = NULL;
    posicoes_Z = NULL;
    
    num_posicoes_salvas = 0;
    tamanho_array = 0;
    posicao_coleta_salva = false;
    
    // Marcar o referenciamento como não concluído para reiniciar do início do main
    referenciamento_concluido = false;
    
    // Reseta também o estado de emergência
    emergencia_ativada = false;

    // Limpa o display para a nova inicialização
    lcd.cls();
}


void reiniciarPrograma() {
    // Resetar variáveis e liberar memória
    delete[] posicoes_X;
    delete[] posicoes_Y;
    delete[] posicoes_Z;

    posicoes_X = NULL;
    posicoes_Y = NULL;
    posicoes_Z = NULL;

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

        // Inicia os Tickers para o LED vermelho e o buzzer
        redLedTicker.attach(&alternarLedVermelho, 0.5);  // Pisca a cada 500ms
        BuzzerTicker.attach(&alternarBuzzer, 0.5);    // Buzzer sincronizado com o LED

        lcd.cls();
        lcd.printf("EMERGENCIA ATIVADA");

        // Desativa os motores imediatamente
        EN_X = 1; // Desativa o motor X
        EN_Y = 1; // Desativa o motor Y
        EN_Z = 1; // Desativa o motor Z

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

        // Para os Tickers de emergência
        redLedTicker.detach();
        BuzzerTicker.detach();

        // Reseta o estado do LED vermelho e do buzzer
        led_vermelho = 0;
        buzzer = 0;

        referenciamento_concluido = false;
        emergencia_ativada = false;

        lcd.cls();
        lcd.printf("Reiniciando...");
        wait_ms(2000);
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

void acionarPipeta() {
    pipeta = 0;           // Ativa a pipeta
    wait_ms(2000);            // Aguarda 0,6 segundos
    pipeta = 1;           // Desativa a pipeta
    wait_ms(2000);            // Aguarda 1,5 segundos antes da próxima ação
}

void piscarLedVerde() {
    led_verde = !led_verde;  // Alterna o estado do LED verde
}

void alternarLedVermelho() {
    led_vermelho = !led_vermelho; // Alterna o estado do LED vermelho
}

void alternarBuzzer() {
    buzzer = !buzzer; // Alterna o estado do buzzer
}