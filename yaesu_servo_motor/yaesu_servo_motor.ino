/*
 * Yaesu G-250 antenna controller
 * Created December 2022
 *
 * bragofsky 2022
 */

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Bridge.h>
#include <BridgeServer.h>
#include <BridgeClient.h>

//definições LCD
#define endereco  0x27
#define colunas   20
#define linhas    4

//Objeto LCD
LiquidCrystal_I2C lcd(endereco, colunas, linhas);

//Objeto bridge
BridgeServer server;

//Definição PIN's I/O
const int PIN_RELAY_CW = 12;
const int PIN_RELAY_CCW = 11;
const int PIN_BUTTON_CW = 7;
const int PIN_BUTTON_CCW = 9;
const int PIN_BUTTON_LR = 10;

const int PIN_POT = A2;

int softAzimuth = 977;                //Variavel global para guardar o valor de azimute recebido por software

void setup() {
  
  Serial.begin(9600);                     //Iniciar porta serie arduino
  pinMode(PIN_RELAY_CW, OUTPUT);          //definição pin output relé CW
  pinMode(PIN_RELAY_CCW, OUTPUT);         //definição pin output relé CCW
  pinMode(PIN_BUTTON_CW, INPUT);          //definição pin input botão CW
  pinMode(PIN_BUTTON_CCW, INPUT);         //definição pin input botão CCW
  pinMode(PIN_BUTTON_LR, INPUT);          //definição pin input botão Local/Remote
 // pinMode(13, OUTPUT);                    //definição led status bridge

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

  // Bridge startup
  digitalWrite(13, LOW);
  Bridge.begin();
  digitalWrite(13, HIGH);
  // Listen for incoming connection only from localhost
  // (no one from the external network could connect)
  server.listenOnLocalhost();
  server.begin();

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

int readpot() {                   //Ler o valor do potenciometro do motor e converter para azimute, serve para limitar o fim e inicio do movimento

  int potVal;
  int angle;

  potVal = analogRead(PIN_POT);
  //Serial.print("potVal : ");
  //Serial.print(potVal);
  angle = map(potVal,20,880,0,359);
  //Serial.print(", angle: ");
  //Serial.println(angle);
  //delay(500);
  return potVal;
  
}

int azimuthVal(){                     //Retornar o valor de azimute do motor
  
  int potVal, angle;

  potVal = analogRead(PIN_POT);
  angle = map(potVal,20,880,0,359);
  
  if(angle>=softAzimuth-1 && angle<=softAzimuth+1){   //Corrige em 1 unidade a variação do potenciometro
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

void process(BridgeClient client) {
  // read the command
  String command = client.readStringUntil('/');
  // is "digital" command?
  if (command == "digital") {
    digitalCommand(client);
  }
  // is "analog" command?
  if (command == "analog") {
    analogCommand(client);
  }
  // is "mode" command?
  if (command == "mode") {
    modeCommand(client);
  }
}

void digitalCommand(BridgeClient client) {
  int pin, value;
  // Read pin number
  pin = client.parseInt();
  // If the next character is a '/' it means we have an URL
  // with a value like: "/digital/13/1"
  if (client.read() == '/') {
    value = client.parseInt();
    digitalWrite(pin, value);
  } else {
    value = digitalRead(pin);
  }
  // Send feedback to client
  client.print(F("Pin D"));
  client.print(pin);
  client.print(F(" set to "));
  client.println(value);
  // Update datastore key with the current pin value
  String key = "D";
  key += pin;
  Bridge.put(key, String(value));
}

void analogCommand(BridgeClient client) {
  int pin, value;
  // Read pin number
  pin = client.parseInt();
  // If the next character is a '/' it means we have an URL
  // with a value like: "/analog/5/120"
  if (client.read() == '/') {
    // Read value and execute command
    value = client.parseInt();
    analogWrite(pin, value);
    // Send feedback to client
    client.print(F("Pin D"));
    client.print(pin);
    client.print(F(" set to analog "));
    client.println(value);
    // Update datastore key with the current pin value
    String key = "D";
    key += pin;
    Bridge.put(key, String(value));
  } else {
    // Read analog pin
    value = analogRead(pin);
    // Send feedback to client
    client.print(F("Pin A"));
    client.print(pin);
    client.print(F(" reads analog "));
    client.println(value);
    // Update datastore key with the current pin value
    String key = "A";
    key += pin;
    Bridge.put(key, String(value));
  }
}

void modeCommand(BridgeClient client) {
  int pin;
  // Read pin number
  pin = client.parseInt();
  // If the next character is not a '/' we have a malformed URL
  if (client.read() != '/') {
    client.println(F("error"));
    return;
  }
  String mode = client.readStringUntil('\r');
  if (mode == "input") {
    pinMode(pin, INPUT);
    // Send feedback to client
    client.print(F("Pin D"));
    client.print(pin);
    client.print(F(" configured as INPUT!"));
    return;
  }

  if (mode == "output") {
    pinMode(pin, OUTPUT);
    // Send feedback to client
    client.print(F("Pin D"));
    client.print(pin);
    client.print(F(" configured as OUTPUT!"));
    return;
  }
  client.print(F("error: invalid mode "));
  client.print(mode);
}

void loop() {
  
  setQuadrante();                 //Write azimuth information in LCD
  set3digit();                  //Add/print zeros to azimuth value in LCD
  
  //Serial.println(readpot());
  
  // if(buttonState(PIN_BUTTON_LR) == 1){        //Modo de controlo com pushbuttons
  
  //   setLcd(6,1,"Local ");               //Escreve local na linha de modo

  //   if(buttonState(PIN_BUTTON_CW) == 1 && buttonState(PIN_BUTTON_CCW) == 0){  //Garante que o relé é ativado apenas quando o botão CW é premido

  //     if(readpot() >= 880){         //Limita o fim de curso superior
  //       relayOnOff(PIN_RELAY_CW,0);     //Impede que o relé se ative se for ultrapassado o limite
  //     }
  //     else{
  //       relayOnOff(PIN_RELAY_CW,1);     //Ativa o relé CW até ao limite definido acima
  //     } 
  //     setLcd(8,2,"Moving CW ");           //Escreve "Moving CW " na linha de modo 

  //   }
  //   else if(buttonState(PIN_BUTTON_CCW) == 1 && buttonState(PIN_BUTTON_CW) == 0){ //Garante que o relé é ativado apenas quando o botão CCW é premido

  //     if(readpot() <= 20){          //Limita o fim de curso inferior
  //       relayOnOff(PIN_RELAY_CCW,0);    //Impede que o relé se ative se for ultrapassado o limite
  //     }
        
  //     else{
  //       relayOnOff(PIN_RELAY_CCW,1);    //Ativa o relé CW até ao limite definido acima
  //     }
  //     setLcd(8,2,"Moving CCW");           //Escreve "Moving CCW" na linha de modo 
  //   }

  //   else if (buttonState(PIN_BUTTON_CCW) == 0 && buttonState(PIN_BUTTON_CW) == 0){  //Quando ambos os pushbuttons estão a zero
  //     relayOnOff(PIN_RELAY_CW,0);     //desativa o botão CW
  //     relayOnOff(PIN_RELAY_CCW,0);      //e o botão CCW
  //     setLcd(8,2,"Stopped   ");           //Escreve "Stopped   " na linha de modo 
  //   }
  // }
  // else if(buttonState(PIN_BUTTON_LR) == 0){   //Modo de controlo por software

    setLcd(6,1,"Remote");

    // Get clients coming from server
    BridgeClient client = server.accept();
    // There is a new client?
    if (client) {
      // Process request
      process(client);
      // Close connection and free resources.
      client.stop();
    }
    delay(50); // Poll every 50ms


    

    //   readSerialARS();

    //   if(azimuthVal()==softAzimuth  || softAzimuth==977){
    //     relayOnOff(PIN_RELAY_CCW,0);
    //     relayOnOff(PIN_RELAY_CW,0);
    //     setLcd(8,2,"Stopped   ");          //Escreve "Stopped   " na linha de modo

    //   } 
    //   else if(azimuthVal()<softAzimuth){
    //     relayOnOff(PIN_RELAY_CW,1);
    //     setLcd(8,2,"Moving CW ");          //Escreve "Moving CW " na linha de modo

    //   }
    //   else if(azimuthVal()>softAzimuth){
    //     relayOnOff(PIN_RELAY_CCW,1);
    //     setLcd(8,2,"Moving CCW");          //Escreve "Moving CCW" na linha de modo
    //   }
  //}
}