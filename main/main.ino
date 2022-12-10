/*
 * Yaesu G-250 antenna controller
 * Created July 2021
 *
 * bragofsky 2021
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Bridge.h>

//definições LCD
#define endereco  0x27
#define colunas   20
#define linhas    4

//Objeto LCD
LiquidCrystal_I2C lcd(endereco, colunas, linhas);

//Definição PIN's I/O
const int PIN_RELAY_RCCW = 0;
const int PIN_RELAY_RCW = 1;
const int PIN_RELAY_RCOMR = 4;
const int PIN_RELAY_RPOTPOS = 5;
const int PIN_RELAY_RPOTCOM = 6;
const int PIN_RELAY_RPOTNEG = 7;
const int PIN_RELAY_CW = 12;
const int PIN_RELAY_CCW = 11;
const int PIN_BUTTON_CW = 13;
const int PIN_BUTTON_CCW = 9;
const int PIN_BUTTON_LR = 10;
const int PIN_BUTTON_R1R2 = 8;
const int PIN_POT = A2;

int softAzimuth = 977;                //Variavel global para guardar o valor de azimute recebido por software

void setup() {
  
  Serial.begin(9600);                     //Iniciar porta serie arduino
  pinMode(PIN_RELAY_RCCW, OUTPUT);          //definição pin output relé de seleção de rotor para o fio CCW
  pinMode(PIN_RELAY_RCW, OUTPUT);         //definição pin output relé de seleção de rotor para o fio CW
  pinMode(PIN_RELAY_RCOMR, OUTPUT);         //definição pin output relé de seleção de rotor para o fio COMUM
  pinMode(PIN_RELAY_RPOTPOS, OUTPUT);       //definição pin output relé de seleção de rotor para o fio POSITIVO POT
  pinMode(PIN_RELAY_RPOTCOM, OUTPUT);       //definição pin output relé de seleção de rotor para o fio COMUM POT
  pinMode(PIN_RELAY_RPOTNEG, OUTPUT);       //definição pin output relé de seleção de rotor para o fio NEGATIVO POT
  pinMode(PIN_RELAY_CW, OUTPUT);          //definição pin output relé CW
  pinMode(PIN_RELAY_CCW, OUTPUT);         //definição pin output relé CCW
  pinMode(PIN_BUTTON_CW, INPUT);          //definição pin input botão CW
  pinMode(PIN_BUTTON_CCW, INPUT);         //definição pin input botão CCW
  pinMode(PIN_BUTTON_LR, INPUT);          //definição pin input botão Local/Remote
  pinMode(PIN_BUTTON_R1R2, INPUT);        //definição pin input botão de seleção de rotor

  lcd.init();                   // INICIA A COMUNICAÇÃO COM O DISPLAY
  lcd.backlight();                // LIGA A ILUMINAÇÃO DO DISPLAY

  lcd.setCursor(2, 0);
  lcd.print("Rotor Controller");
  lcd.setCursor(0, 1);
  lcd.print("Mode:");
  lcd.setCursor(0, 2);
  lcd.print("Status:");
  lcd.setCursor(0, 3);
  lcd.print("Azimuth:");

}

int buttonState(int BUTTON){              //Verifica o estado do botão recebido como argumento

  int buttonstate = 0;

  buttonstate = digitalRead(BUTTON);
  //Serial.println(buttonstate);
  if(buttonstate == HIGH)
    return 1;
  else
    return 0;
}

void relayOnOff(int RELAY, int STATE){      // Controla relé mediante o estado e porta recebidos como argumento

  if(STATE == 0)
    digitalWrite(RELAY,HIGH);             // turn the relay ON      
  else
    digitalWrite(RELAY,LOW);              // turn the relay OFF

}

void readpot() {                   //Ler o valor do potenciometro do motor e converter para azimute, serve para limitar o fim e inicio do movimento

  int potVal;
  int angle;

  potVal = analogRead(PIN_POT);
  Serial.print("potVal : ");
  Serial.print(potVal);
  angle = map(potVal,5,1006,0,359);
  Serial.print(", angle: ");
  Serial.println(angle);
  delay(500);

  
}

int azimuthVal(){                     //Retornar o valor de azimute do motor
  
  int potVal, angle;

  potVal = analogRead(PIN_POT);
  angle = map(potVal,5,1006,0,359);
  
  if(angle>=softAzimuth-2 && angle<=softAzimuth+2){   //Corrige em 1 unidade a variação do potenciometro
    //Serial.println("com correcao");
    return softAzimuth;
  }
  else{
    //Serial.println("sem correcao");
    return angle;
  }
}

void readSerialARS(){                   //Ler a porta serie, retirar os dois primeiros caracteres e converter o valor para inteiro

  char output[3]={0,0,0};                   //Vetor de char's para guardar o valor de azimute
  int j=0, i=0;

  while (Serial.available() > 0) {          //inicia a comunicação serial
  
    String input = Serial.readString();

    if(input[2]==71){                       //Se o segundo caracter for "G" (HEX==71) quer dizer que tem valor de azimute
      for(i=0; i<=sizeof(input);i++){       //Percorre a string até ao tamanho
        if(isDigit(input[i])){              //Se encontrar um digito na posição "i" de input
          output[j++]=input[i];             //guarda esse digito na posição "j" de output
        }
      }
      softAzimuth=atoi(output);         //Caso o if seja TRUE, guarda o valor inteiro de output na variavél global "softazimuth"
    }
  }
}

void setLcd(int COLUNA, int LINHA, String INFO){  //Print's various info in LCD
  
  lcd.setCursor(COLUNA, LINHA);
  lcd.print(INFO);
  
}

void setQuadrante(){                    //Função para mostrar o quadrante no LCD mediante info de azimute
  
  if (azimuthVal() >= 0 && azimuthVal() <= 90){
    setLcd(12,3," NE");
  }
  if (azimuthVal() >= 91 && azimuthVal() <= 180){
    setLcd(12,3," SE");
  }
  if (azimuthVal() >= 181 && azimuthVal() <= 270){
    setLcd(12,3," SW");
  }
  if (azimuthVal() >= 271 && azimuthVal() <= 359){
    setLcd(12,3," NW");
  }
}

void set3digit(){                     //Função que acrescenta zeros antes das dezenas e unidades do valor de azimute
  if (azimuthVal() >=100){
    lcd.setCursor(9, 3);
    lcd.print(azimuthVal());
  }

  if (azimuthVal() <=99 && azimuthVal() >=10){
    lcd.setCursor(9, 3);
    lcd.print("0");
    lcd.print(azimuthVal());
  }

  if (azimuthVal() <=9 && azimuthVal() >=0){
    lcd.setCursor(9, 3);
    lcd.print("00");
    lcd.print(azimuthVal());
  }
}

void loop() {

  //readpot();                //Imprime o valor do potenciometro para teste             
  setQuadrante();             //Write azimuth information in LCD
  set3digit();                //Add/print zeros to azimuth value in LCD
  
  if(buttonState(PIN_BUTTON_R1R2) == 1){    //Close all relays to change rotor port
    relayOnOff(PIN_RELAY_RCCW, 1);
    relayOnOff(PIN_RELAY_RCW, 1);
    relayOnOff(PIN_RELAY_RCOMR, 1);
    relayOnOff(PIN_RELAY_RPOTPOS, 1);
    relayOnOff(PIN_RELAY_RPOTCOM, 1);
    relayOnOff(PIN_RELAY_RPOTNEG, 1);
  }
  else if(buttonState(PIN_BUTTON_R1R2) == 0){ //Open all relays to change rotor port
    relayOnOff(PIN_RELAY_RCCW, 0);
    relayOnOff(PIN_RELAY_RCW, 0);
    relayOnOff(PIN_RELAY_RCOMR, 0);
    relayOnOff(PIN_RELAY_RPOTPOS, 0);
    relayOnOff(PIN_RELAY_RPOTCOM, 0);
    relayOnOff(PIN_RELAY_RPOTNEG, 0);
  }
  
  if(buttonState(PIN_BUTTON_LR) == 1){        //Modo de controlo com pushbuttons
  
    setLcd(6,1,"Local ");               //Escreve local na linha de modo

    if(buttonState(PIN_BUTTON_CW) == 1 && buttonState(PIN_BUTTON_CCW) == 0){  //Garante que o relé é ativado apenas quando o botão CW é premido

      if(azimuthVal() >= 360){         //Limita o fim de curso superior
        relayOnOff(PIN_RELAY_CW,0);     //Impede que o relé se ative se for ultrapassado o limite
      }
      else{
        relayOnOff(PIN_RELAY_CW,1);     //Ativa o relé CW até ao limite definido acima
      } 
      setLcd(8,2,"Moving CW ");           //Escreve "Moving CW " na linha de modo 

    }
    else if(buttonState(PIN_BUTTON_CCW) == 1 && buttonState(PIN_BUTTON_CW) == 0){ //Garante que o relé é ativado apenas quando o botão CCW é premido

      if(azimuthVal() <= 0){         //Limita o fim de curso inferior
        relayOnOff(PIN_RELAY_CCW,0);    //Impede que o relé se ative se for ultrapassado o limite
      }
        
      else{
        relayOnOff(PIN_RELAY_CCW,1);    //Ativa o relé CW até ao limite definido acima
      }
      setLcd(8,2,"Moving CCW");           //Escreve "Moving CCW" na linha de modo 
    }

    else if (buttonState(PIN_BUTTON_CCW) == 0 && buttonState(PIN_BUTTON_CW) == 0){  //Quando ambos os pushbuttons estão a zero
      relayOnOff(PIN_RELAY_CW,0);     //desativa o botão CW
      relayOnOff(PIN_RELAY_CCW,0);      //e o botão CCW
      setLcd(8,2,"Stopped   ");           //Escreve "Stopped   " na linha de modo 
    }
  }
  else if(buttonState(PIN_BUTTON_LR) == 0){   //Modo de controlo por software

    setLcd(6,1,"Remote");

    readSerialARS();

    if(azimuthVal()==softAzimuth  || softAzimuth==977){
      relayOnOff(PIN_RELAY_CCW,0);
      relayOnOff(PIN_RELAY_CW,0);
      setLcd(8,2,"Stopped   ");          //Escreve "Stopped   " na linha de modo

    } 
    else if(azimuthVal()<softAzimuth){
      relayOnOff(PIN_RELAY_CW,1);
      setLcd(8,2,"Moving CW ");          //Escreve "Moving CW " na linha de modo

    }
    else if(azimuthVal()>softAzimuth){
      relayOnOff(PIN_RELAY_CCW,1);
      setLcd(8,2,"Moving CCW");          //Escreve "Moving CCW" na linha de modo

    }
  }
}
