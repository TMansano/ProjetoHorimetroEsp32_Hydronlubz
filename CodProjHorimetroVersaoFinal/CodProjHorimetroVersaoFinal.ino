#include <Arduino.h>
#include <LiquidCrystal.h>  // Biblioteca LCD
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#define CS 4  //Chip Select.

//----------------------Declaração de variáveis globais-------------------------
long int sec = 0;
int minute = 0;
int h = 0;
int ciclos = 0;
int ciclos_aux = 0;
int sec2=0;
int sec3=0;

long int timerUltimaoportunidade=0;
long int millisegundo=0;

//variaveis do voltimetro--------------------------
// Constante do sensor de tensão
int sensorTensaoAC = 0;
//Declaração da variavel que recebe o valor lido na porta analogica
float valorTensao;
//Declaração da variavel da media de amostragem
int amostragem = 1000;
//Declaração da variavel da soma da media de amostragem
float mediaTotal = 0;
//Declaração de variavel que recebe o valor final convertido em volts
float valorFinal = 0;
//Declaração da variavel da constante do ADC 5/1023
float voltsporUnidade = 0.004887586;
//fim das variaveis do voltimetro--------------------------

//define o arquivo a ser salvo os dados
File variaveis;

//define a inicialização do LCD i2c
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() 
{
  //definição da inicialização dos sistemas

  lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();
  //Serial.begin(9600);
  lcd.print("Hydronlubz JHF");
  //delay(300);
  pinMode(sensorTensaoAC, INPUT);
  //analogReference(DEFAULT);
  
//inicialização do cartão
  if (!SD.begin(4)) {
    lcd.clear();
    lcd.print("Falha no SD Card");
    while (1);
  }

if(SD.exists("dados.txt"))
  {
    variaveis = SD.open("dados.txt");
    if (variaveis) {
    sec = variaveis.parseInt();
    minute = variaveis.parseInt();
    h = variaveis.parseInt();
    ciclos = variaveis.parseInt();

    sec2=variaveis.parseInt();
    sec3=variaveis.parseInt();

    variaveis.close();
    SD.remove("dados.txt");
    }
  }
  
}

void loop() 
{
  //Chama a função para fazer a detecção do nível de tensão
  voltimetro_f();

  if (valorFinal > 80) 
  {
    //começo da contagem----------------------------------
    lcd.clear();//limpa o lcd
    lcd.print("Hora-Maquina...");
    ciclos++;//adiciona um ciclo
    //lcd.backlight();//blink

    while (valorFinal > 80) 
    {
      //Chama a função para fazer a detecção do nível de tensão novamente e atualizar a variavel
      voltimetro_f();
      
      
           if  (   (sec2 - sec3) < 40 )
          {  
           //if (millis () – 40000 >=0)

            if(  ( ciclos - ciclos_aux) == 15  )
           {
            lcd.clear();
            lcd.print("Redefinindo");
            lcd.setCursor(0, 1);
            lcd.print("Horimetro...");

            sec = 0;
            minute = 0;
            h = 0;
            ciclos = 0;
            ciclos_aux=ciclos;
            timerUltimaoportunidade=millisegundo ;
            delay(5000);
            lcd.clear();
            lcd.print("Resetado");
            loop();
            
           }
          else
           {
            ciclos_aux=ciclos; 
            sec3=sec2;
            timerUltimaoportunidade=millisegundo ;
           }

          
          
        }
      //a cada um minuta mostra os valores de ciclos e ten~soa da rede.
      //if(sec>50)
      if (sec==50)
       {
        lcd.clear();
        lcd.print("Ciclos da Maquina");
        lcd.setCursor(0, 1);
        lcd.print(ciclos);
        delay(1000);
        lcd.clear();
        
        //Valor da tensão Ac da Rede, print no LCD
        lcd.print("Tensao da Rede");
        lcd.setCursor(0, 1);
        lcd.print(valorFinal);
        lcd.print(" V");
        delay(1000);
       // lcd.clear();
        
      }

      //chama a função e conta, de sedundo em segundo (cada vez que é chamada).
      counting_f();  

      lcd.setCursor(0, 0);
      lcd.print("Hora-Maquina...");
      lcd.setCursor(0, 1);  // Define o cursor na coluna 0, linha 1

      // Algoritimo para exibição no formato 0x:0y:0z, se x, y, z forem compostos por um único dígito
      // onde os caracteres no formato horas:minutos:segundos serão exibidos
      if (h < 10)         
      {
        lcd.print("000");
        lcd.print(h);
        lcd.print(":");
      } else {
        lcd.print(h);  // Se a hora for um número (2 dígitos), exibe normalmente, sem adicionar um 0 à frente
        lcd.print(":");
      }
      if (minute < 10)  // Se o minuto for um número (2 dígitos), exibe normalmente, sem adicionar um 0 à frente
      {
        lcd.print("0");
        lcd.print(minute);
        lcd.print(":");
      } else {
        lcd.print(minute);
        lcd.print(":");
      }
      if (sec < 10) {
        lcd.print("0");
        lcd.print(sec);
      } else {
        lcd.print(sec);
      }
    }
  }  // Fim do IF

  else
  //if (valorFinal < 80)  // Se o botão de Parar estiver pressionado (máquina desligada)
  {
    //Aguardando estado da máquina
    lcd.clear();
    lcd.setCursor(0, 0);  // Exibe na primeira linha, na primeira coluna, a mensagem
    //lcd.print("Contagem Pausada, Aguardando Maquina");
    lcd.print("Aguardando");
    lcd.setCursor(0, 1);
    lcd.print("Maquina");
    delay(1000);

    //verifica no nivel de tensão e caso a maquina esteja ligada então volta para a contagem.
    voltimetro_f();
    if (valorFinal > 80) {
      loop();
    }
    
    //printa o número de ciclos da máquina
    lcd.clear();
    lcd.print("Numero de Ciclos");
    lcd.setCursor(0, 1);
    lcd.print(ciclos);
    delay(1000);
    lcd.clear();

    //verifica no nivel de tensão e caso a maquina esteja ligada então volta para a contagem.
    voltimetro_f();
    if (valorFinal > 80) {
      loop();
    }
    //printa a tensão instantânea da rede.
    lcd.print("Tensao da Rede");
    lcd.setCursor(0, 1);
    lcd.print(valorFinal);
    lcd.print(" V");
    delay(1000);
  }  
}  // Fim do loop

//------------------Função de contagem------------------
void counting_f() 
{
  sec++;
  sec2++;
  //sec3++;      // Conta cada segundo a partir de 0
  delay(1000);  // 1000 ms = 1 segundo
  if (sec > 59) {
    sec = 0;  // sec=60 -> min=min+1, sec=0
    minute++;
  }
  if (minute > 59)  // min=60
  {
    h++;
    minute = 0;
  }
  // Toda a vez que se contar então atualiza - se o valor das variaveis.variaveis = SD.open("dados.txt", FILE_WRITE);
  //escreve no cartao SD as ultimas variaveis
  //faz a gravação dos dados do horímetro no cartão de memória.
  SD.remove("dados.txt");//remove o arquivo para criar outro, manter os dados atualizados
  variaveis = SD.open("dados.txt", FILE_WRITE);
  //escreve no cartao SD as ultimas variaveis
  if (variaveis) { 
    variaveis.println(sec);
    variaveis.println(minute);
    variaveis.println(h);
    variaveis.println(ciclos);


    variaveis.println(sec2);
    variaveis.println(sec3);

    variaveis.close();
  }  
}

  //------------------Função voltimetro------------------
  void voltimetro_f() 
  {
  //inicio parte do sensor de tesão AC-----------------------------------------------------------------------
  //Inicia a variavel zerada e atualiza na proxima leitura
  valorFinal = 0;
  //Inicia a variavel zerada e atualiza na proxima leitura
  mediaTotal = 0;
  for (int i = 0; i < amostragem; i++) {
    //Leitura do valor na porta analógica (varia de 0 a 1023)
    valorTensao = analogRead(sensorTensaoAC);
    mediaTotal = mediaTotal + valorTensao;
    //delay(100);
  }
  //Retira a media da tensao dos valores de leitura
  mediaTotal = mediaTotal / amostragem;
  //Bloco referente a tensao de 127V
  //Se variavel for maior que 100 e menor que 483 faz
  if ((mediaTotal > 100) & (mediaTotal < 483)) {
    valorFinal = ((mediaTotal * 5) / 1023) * 79.0;  //80.2 é o valor de calibração
  }
  //Bloco referente a tensao de 220V
  //Se variavel for maior que 100 e menor que 483 faz
  if ((mediaTotal > 665) & (mediaTotal < 1023)) {
    valorFinal = ((mediaTotal * 5) / 1023) * 74.5;  //valor de calibração
  }
  //final parte do sensor de tesão AC-----------------------------------------------------------------------
}
