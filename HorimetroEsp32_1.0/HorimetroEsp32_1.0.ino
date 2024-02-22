#include <Arduino.h>
#include <LiquidCrystal.h>  // Biblioteca LCD
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include "FS.h"
#include <SPIFFS.h>

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
//----------------------char strings-----------------------------------
char buf[21];
char buf2[23];
char hr[12];
char minutes[10];
char secs[10];
char ciclos2[12];
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

void setup() {

  //-------------definição da inicialização dos sistemas--------------------------
  lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();
  Serial.begin(9600);
  //-------------Entradas e Saidas----------------------------------------
  pinMode(17, OUTPUT);        //TBJ for Buzzer
  pinMode(25, OUTPUT);        // Reset LED
  pinMode(27, INPUT_PULLUP);  //Reset BUTTON1
  pinMode(26, INPUT_PULLUP);  //Calibration BUTTON2
  pinMode(33, INPUT);         //Input Voltage Sensor.
  delay(200);
  //inicialização do cartão---------------------------------------------
  if (!SD.begin(5)) {
    lcd.clear();
    lcd.print("Falha no SD Card");
    //Serial.println("Falha no SD Card.");
    while (1)
      ;
  }
  //------------------------Bruteforce Reset System---------------------
  if (digitalRead(BUTTON1) == LOW) {
    //SD.remove("/dados3.txt");
    reset();  //chama o reset e eresta o horimetro
  }





  if (!SD.exists("/dados3.txt") && !SD.exists("/dados4.txt")) {

    sprintf(buf2, "%05d:%02d:%02d:%08d", hrs, minute, sec, ciclos);
    WriteFile("/dados3.txt", buf2);
    WriteFile("/dados4.txt", buf2);


  } else {
    
    variaveis = SD.open("/dados3.txt");

    int tamanho1=variaveis.size();
    




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
    }


    //faz a conversão para Inteiros e coloca na memoria de trabalho do esp
    sec = std::__cxx11::stoi(secs);
    minute = std::__cxx11::stoi(minutes);
    hrs = std::__cxx11::stoi(hr);
    ciclos = std::__cxx11::stoi(ciclos2);
  }
























  if (!SD.exists("/dados3.txt")) {
    variaveis = SD.open("/dados3.txt", FILE_WRITE);
    if (variaveis) {
      //Serial.println("Arquivo não exites aqui");
      //delay(3000);
      sprintf(buf2, "%05d:%02d:%02d:%08d", hrs, minute, sec, ciclos);
      WriteFile("/dados3.txt", buf2);
    }
  } else {
    variaveis = SD.open("/dados3.txt");
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
    }


    Serial.println("testando leitura hr");
    Serial.println(hr);
    Serial.println("testando leitura minutes");
    Serial.println(minutes);
    Serial.println("testando leitura secs");
    Serial.println(secs);
    Serial.println("testando leitura ciclos");
    Serial.println(ciclos2);


    //faz a conversão para Inteiros e coloca na memoria de trabalho do esp
    sec = std::__cxx11::stoi(secs);
    minute = std::__cxx11::stoi(minutes);
    hrs = std::__cxx11::stoi(hr);
    ciclos = std::__cxx11::stoi(ciclos2);
  }

  //maquina ligou, adiciona um ciclo
  ciclos++;
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
  //contador oscilador auto calibrante, "calibra" com o tempo, contador preciso. e que só aconta apenas e apenas se valorFinal > 80 Volts.

  if (((millis() / 1000) >= next_millis) && (valorFinal > 80)) {
    next_millis += 1;  //registrador de clock.

    sec++;  // conta um segundo!.

    minute += (sec / 60);
    hrs += minute / 60;
    minute %= 60;
    sec = sec % 60;

    sprintf(buf2, "%05d:%02d:%02d:%08d", hrs, minute, sec, ciclos);  //string a ser salva na memória cartão SD
    sprintf(buf, "%05d:%02d:%02d     ", hrs, minute, sec);           //string a ser impressa na LCD

    lcd.print("Hora-maquina");
    lcd.setCursor(0, 1);
    lcd.println(buf);

    WriteFile("/dados3.txt", buf2);  //fução que salva os dados no SD
  }
}




void WriteFile(const char* path, const char* message) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  variaveis = SD.open(path, FILE_WRITE);
  // if the file opened okay, write to it:
  if (variaveis) {
    Serial.println("Objeto Aberto.");
    Serial.println("Writing to %s ", path);
    delay(5000);

    //aponta para o final e salva a string contendo os dados atualizados do horimetro, assim, acumulando os dados no aquivo h
    variaveis.seek(variaveis.size());
    variaveis.println(message);
    variaveis.close();  // close the file:

    delay(5000);
    Serial.println("completo, objeto Fechado..");
    //Serial.println("Objeto Fechado.");
    //lcd.noBacklight();
    //delay(5000);

  }
  // if the file didn't open, print an error:
  else {
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
  funcReset();

  //volta ao "normal/operação" função principal
  loop();
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





/*

    Serial.println("Teste de pre-falha"); //linha utilizada para debug e teste da execução do código
    delay(25000);

    */


//medir tempo com millis
//float millis_voltimetroinicial = millis();
//float millis_voltimetrofinal = millis();
// float time_diference = (millis_voltimetrofinal - millis_voltimetroinicial);
//Serial.println("Tempo de execução da função voltimetro");
//Serial.println(time_diference);
//delay(5000);
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