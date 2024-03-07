
/*
//3E UEL  Universidade Estadual de Londrina 2024
------------------------------- Codificação de desenvolvimento de software de arduino ------------------
horimetro contador ultra preciso com controle automatico de escala e fator de erro previsivél

Este código se refere ao dispositivo desenvolvido pela 3E UEL Engenharia, na qual se refe a um contador de alta precisão que se automaticamente 
corrige com tempo, sendo assim um contador em malha fechada com relação a sua saida [numero de horas totais e ciclos], o mesmo também conta 
com sensor ultra rápido de tensão para aquisição dos dados de inicio e fim de contagem e também salvamento dos dados em um cartão de 
memória micro SD, o código é sequencial entretante após melhorias o código funcina com base em dois núcloeos do processodr presente 
na ESP32, a programação multicore foi necessaria para aumentar a velocidade de processamento do contador e melhorar sua precisão.

as bibliotecas necessaria estão abaixo com observação a biblioteca LiquidCrystal_I2C.h que deve ser instalada separadamente na maquina assim como a placa ESP32 Dev Module da ESP 32 de 36 pinos.
*/

#include <Arduino.h>
#include <LiquidCrystal.h>  // Biblioteca LCD
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

#define CS 5  //Chip Select SD Card.
#define LEDPIN 25
#define BUTTON1 27
#define BUTTON2 26
#define ANALOGREADING 33
#define BUZZERPIN 17
//----------------------Declaração de variáveis globais-------------------------
int sec = 0;
int minute = 0;
unsigned int hrs = 0;  // o horimetro tem uma contagem máxima de 60 mil horas, com precisão de SEGUNDOS!
unsigned long ciclos = 0;
int bstop = 0;
int i = 0;
unsigned long next_millis = 0;
int erro_medio = 0;
//----------------------char strings-----------------------------------
char buf[25];
char buf2[25];
char buf3[28];
char hr[12];
char minutes[10];
char secs[10];
char ciclos2[12];
//char arquivo_maior[15];  //define o char onde sera o arquivo não corrompido ou intacto
//char dados[]= "/dados.h"; //loca onde será salvo os dados na memoria SD
//variaveis do voltimetro--------------------------
int amostragem = 600;
float VResistor = 0;
float VPico = 0;
float valorFinal = 0;
//função de reboot do microcontrolador
void (*funcReset)() = 0;
//define o arquivo a ser salvo os dados
File variaveis;
//define a inicialização do LCD i2c
LiquidCrystal_I2C lcd(0x27, 16, 2);
float millis_voltimetrofinal=0;
//variaveis que indicam o núcleo
static uint8_t taskCoreZero = 0;
float millis_voltimetroinicial =0;


void setup() {
  //-------------definição da inicialização dos sistemas--------------------------
  Serial.begin(9600);
  //-------------Entradas e Saidas----------------------------------------
  pinMode(17, OUTPUT);        //TBJ for Buzzer
  pinMode(25, OUTPUT);        // Reset LED
  pinMode(27, INPUT_PULLUP);  //Reset BUTTON1
  pinMode(26, INPUT_PULLUP);  //Calibration BUTTON2
  pinMode(33, INPUT_PULLDOWN);         //Input Voltage Sensor.
  delay(10);                 //delay for stabilize pins
   //cria uma tarefa que será executada na função coreTaskTwo, com prioridade 2 e execução no núcleo 0
   //coreTaskTwo: vigiar o botão para detectar quando pressioná-lo
   xTaskCreatePinnedToCore(
                    coreTaskTwo,   /* função que implementa a tarefa */
                    "coreTaskTwo", /* nome da tarefa */
                    10000,      /* número de palavras a serem alocadas para uso com a pilha da tarefa */
                    NULL,       /* parâmetro de entrada para a tarefa (pode ser NULL) */
                    0,          /* prioridade da tarefa (0 a N) */
                    NULL,       /* referência para a tarefa (pode ser NULL) */
                    taskCoreZero);         /* Núcleo que executará a tarefa */

        delay(500); //tempo para a tarefa iniciar

  
  //------------------------leitura de tensão inicial---------------------
  Voltimetro();
  //inicialização do cartão---------------------------------------------
  if (!SD.begin(5)) {
    delay(3500);
    lcd.clear();
    lcd.print("Falha no SD Card");
    while (!SD.begin(5));
    lcd.clear();
  }
  //------------------------Bruteforce Reset System---------------------
  if (digitalRead(BUTTON1) == LOW) {
    reset();  //chama o reset e eresta o horimetro
  }
  if (!SD.exists("/dados3.txt") && !SD.exists("/dados4.txt")) {

    variaveis = SD.open("/dados3.txt", FILE_WRITE);
    if (variaveis) {
      sprintf(buf2, "%05d:%02d:%02d:%08d", hrs, minute, sec, ciclos);
      WriteFile("/dados3.txt", buf2);
    }
    variaveis = SD.open("/dados4.txt", FILE_WRITE);
    if (variaveis) {
      WriteFile("/dados4.txt", buf2);
    }
  } else {
    

    variaveis = SD.open("/dados3.txt");
    int A = variaveis.size();
    variaveis.close();
    delay(5);

    variaveis = SD.open("/dados4.txt");
    int B = variaveis.size();
    variaveis.close();
    delay(5);
    
    if (A >= B) {variaveis = SD.open("/dados3.txt");} 
    else {variaveis = SD.open("/dados4.txt");}

    if (variaveis) {

      variaveis.seek(variaveis.size() - 22);  //aponta pra a ultima linha (ultimo salvamento dos dados)

      //4 fors: faz a varedura da linha buscando as variaveis salvas do horimetro
      for (i = 0; i < 5; i++) {
        hr[i] = variaveis.read();
      }
      variaveis.read();  //pula o ":"
      i = 0;
      for (i = 0; i < 2; i++) {
        minutes[i] = variaveis.read();
      }
      variaveis.read();  //pula o ":"
      i = 0;
      for (i = 0; i < 2; i++) {
        secs[i] = variaveis.read();
      }
      variaveis.read();  //pula o ":"
      i = 0;
      for (i = 0; i < 8; i++) {
        ciclos2[i] = variaveis.read();
      }
      variaveis.close();  //fecha o arquivo apos a leitura
      delay(10);
    }
    //faz a conversão para Inteiros e coloca na memoria de trabalho do esp
    sec = std::__cxx11::stoi(secs);
    minute = std::__cxx11::stoi(minutes);
    hrs = std::__cxx11::stoi(hr);
    ciclos = std::__cxx11::stoi(ciclos2);
  }
  //maquina ligou, adiciona um ciclo
  ciclos++;
  //fator de correção de escala (tempo total errado em s) é de: +-(0.008xTtotal) + (0.5xCiclos).
  erro_medio = (ciclos % 2);
  sec += erro_medio;
 
//------------------------------------Data Logger (registro de informações a longo prazo----------------------------------------------
  if (!SD.exists("/dados5.txt")) {
    variaveis = SD.open("/dados5.txt", FILE_WRITE);
    if (variaveis) {
      WriteFile("/dados5.txt", "Horas:Minutos:Segundos:Ciclos:Tensão");
      sprintf(buf3, "%05d:%02d:%02d:%08d:%.1f", hrs, minute, sec, ciclos,valorFinal);
      WriteFile("/dados5.txt", buf3);
    }
  } else {
    sprintf(buf3, "%05d:%02d:%02d:%08d:%.1f", hrs, minute, sec, ciclos, valorFinal);
    WriteFile("/dados5.txt", buf3);
    }
//--------------------------- Algoritmo de escrita no cartão de memória ---------------------------
    sprintf(buf2, "%05d:%02d:%02d:%08d", hrs, minute, sec, ciclos);  //string a ser salva na memória cartão SD

    WriteFile("/dados3.txt", buf2);  //fução que salva os dados no SD
    delay(5); //(delay de estabilização)
    WriteFile("/dados4.txt", buf2);
}

void loop() {
  //chama a função voltimetro e analisa o nivel de tensão
  Voltimetro();
  if (digitalRead(BUTTON1) == LOW) {
    reset();  //chama o reset e eresta o horimetro
  }
  if (digitalRead(BUTTON2) == LOW) {
    calibragem();  //chama o reset e eresta o horimetro
  }
  //a cada um minuta mostra os valores de ciclos e ten~soa da rede.
  if (sec == 45) {
    mostra_valores();
  }
  //Perfom a hardware reset to prevent Variables overflow
  if (next_millis > 60000) {
    funcReset();
  }
  //faz o aviso de quando passa de 100horas
  if (hrs >= 100 && bstop < 30) {
    manutencao_avisos();
    bstop++;
  }
  //faz o aviso de quando passa de 1000horas
  if (hrs >= 1000 && bstop < 40) {
    manutencao_avisos();
    bstop++;
  }

  //----------------------------Contador-------------------------------------
  //contador oscilador auto calibrante, "calibra" com o tempo, contador preciso.

  if (((millis() / 1000) >= next_millis)) {
    next_millis += 1;  //registrador de clock.
    sec++;  // conta um segundo!.
    minute += (sec / 60);
    hrs += minute / 60;
    minute %= 60;
    sec = sec % 60;

    //--------------------------- Algoritmo de escrita no LCD ---------------------------
    sprintf(buf, "%05d:%02d:%02d     ", hrs, minute, sec);  //string a ser impressa na LCD
    lcd.print("Hora-maquina");
    lcd.setCursor(0, 1);
    lcd.println(buf);
    //--------------------------- Algoritmo de escrita no cartão de memória ---------------------------
    sprintf(buf2, "%05d:%02d:%02d:%08d", hrs, minute, sec, ciclos);  //string a ser salva na memória cartão SD

    WriteFile("/dados3.txt", buf2);  //fução que salva os dados no SD
    delay(10); //(delay de estabilização)
    WriteFile("/dados4.txt", buf2);
  }
}

void WriteFile(const char* path, const char* message) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  variaveis = SD.open(path, FILE_WRITE);
  // if the file opened okay, write to it:
  if (variaveis) {
    //aponta para o final e salva a string contendo os dados atualizados do horimetro, assim, acumulando os dados no aquivo h
    variaveis.seek(variaveis.size());
    variaveis.println(message);
    variaveis.close();  // close the file:
    delay(10);
  }
  // if the file didn't open, print an error:
  else {
    lcd.clear();
    lcd.println("Erro ao Abrir o ");
    lcd.setCursor(0, 1);
    lcd.println("Cartao SD.      ");
    Serial.println("Erro ao Abrir o Arquivo");
    Serial.println(path);
    
  }
}

//------------------Função voltimetro------------------
void Voltimetro() {
  for (int i = 0; i < amostragem; i++) {
    //Leitura do valor na porta analógica (varia de 0 a 4096)
    if (analogRead(ANALOGREADING) > VPico) {
      VPico = analogRead(ANALOGREADING);
    }
  }
  VResistor = ((VPico)*3.3 / 4096);  //tensão na entrada do arduino/esp32
  valorFinal = (VResistor)*57;       //o (VResistor)*57 é equivalente a vpico/raizde2(multiplica pelo o quanto atunuou a tensão)
  VPico = 0;
}

//------------------------------Função Resetar Horimetro-----------------------
void reset() {
  digitalWrite(LEDPIN, HIGH);
  delay(130);
  lcd.clear();
  lcd.print("Redefinindo");
  Serial.print("Horimetro Redefinido");
  lcd.setCursor(0, 1);
  lcd.print("Horimetro...");
  delay(1000);
  lcd.clear();
  lcd.print("Aguarde");
  delay(2000);
  digitalWrite(LEDPIN, LOW);
  delay(100);
  lcd.clear();
  lcd.print("Horimetro");
  lcd.setCursor(0, 1);
  lcd.print("Redefinido");
  digitalWrite(BUZZERPIN, HIGH);
  digitalWrite(LEDPIN, HIGH);
  delay(250);
  digitalWrite(LEDPIN, LOW);
  digitalWrite(BUZZERPIN, LOW);
  delay(1000);

  //--RESET geral do Horimetro/dispositivo
  sec = 0;
  minute = 0;
  hrs = 0;
  ciclos = 0;
  SD.remove("/dados3.txt");
  delay(20);
  SD.remove("/dados4.txt");
  delay(20);
  SD.remove("/dados5.txt");
  delay(20);
  funcReset();
}

//-------------------Função Calibragem-------------------
void calibragem() {
  delay(1000);
  lcd.clear();

  while (digitalRead(BUTTON2) == HIGH) {
    Voltimetro();  //atualiza o valor da tensão //Valor da tensão Ac da Rede, print no LCD

    lcd.print("Tensao da Rede");
    lcd.setCursor(0, 1);
    lcd.print(valorFinal);
    lcd.print(" V          ");
  }
  delay(1000);
  lcd.clear();
  loop();  //chama loop e volta ao "normal/operação" função principal
}
void manutencao_avisos() {

  lcd.clear();
  lcd.print("   Manutencao");
  lcd.setCursor(0, 1);
  lcd.print("   Programada!");
  digitalWrite(BUZZERPIN, HIGH);
  delay(250);
  digitalWrite(BUZZERPIN, LOW);
  delay(200);
  bstop++;
  lcd.clear();
}
void mostra_valores() {
  lcd.clear();
  lcd.print("Ciclos da Maquina");
  lcd.setCursor(0, 1);
  lcd.print(ciclos);
  delay(1300);
  lcd.clear();
  //Valor da tensão Ac da Rede, print no LCD
  lcd.print("Tensao da Rede");
  lcd.setCursor(0, 1);
  lcd.print(valorFinal);
  lcd.print(" V");
  delay(1400);
  lcd.clear();
}








//essa função será responsável por ler o estado do botão
//e atualizar a variavel de controle.
void coreTaskTwo( void * pvParameters ){

//--------------Inicializa o LCD------------------------
  lcd.begin(16,2);
  lcd.init();
  // Liga a luz do display
  lcd.backlight();

     while(true){
      delay(20);
    } 
}








/*
//------------------------Função utilizada para leitura na comunicação serial do esp com computador, lê o Cartão SD
void ReadFile(const char* path) {
  // open the file for reading:
  variaveis = SD.open(path);
  if (variaveis) {
    Serial.printf("Reading file from %s\n", path);
    // read from the file until there's nothing else in it:
    //variaveis.seek(variaveis.size() - 19);  //aponta pro final menos uma linha
    while (variaveis.available()) {
      Serial.write(variaveis.read());
    }
    variaveis.close();  // close the file:
    delay(2000);
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening archive");
  }
}
*/