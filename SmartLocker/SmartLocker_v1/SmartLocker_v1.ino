#include <Keypad.h>
#include <Keypad_I2C.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <MFRC522.h>

#define I2CADDR 0x20      // Endereço IC2 PCF8574 Keypad
#define rxPin 8
#define txPin 7
#define espRxPin 2
#define espTxPin 3
#define SS_PIN A1          //pino SDA do leitor
#define RST_PIN A3        //pino RST do leitor

const int ROW_NUM    = 4; // número de linhas do teclado
const int COLUMN_NUM = 4; // número de colunas do teclado
int buzzerPin = 9;        // Pino ao qual o buzzer está conectado
int door = 1;             //Mudar para selecionar a porta aberta ou fechada
bool flagDadosUser = false;

char keys[ROW_NUM][COLUMN_NUM] = {
  {'D', '#', '0', '*'}, //talvez mudar aqui
  {'C', '9', '8', '7'}, //certo
  {'A', '3', '2', '1'}, //certo
  {'D', '#', '0', '*'}  //talvez mudar aqui
};

byte pin_rows [ROW_NUM] = {0, 1, 2, 3}; // Connect to Keyboard Row Pin
byte pin_column [COLUMN_NUM] = {4, 5, 6, 7}; // Connect to Pin column of keypad.

Keypad_I2C keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM, I2CADDR);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço I2C do LCD, 16 colunas e 2 linhas
SoftwareSerial arduinoSerial =  SoftwareSerial(rxPin, txPin);
SoftwareSerial espSerial(espRxPin, espTxPin);
MFRC522 mfrc522(SS_PIN, RST_PIN);    // Cria uma nova instância para o leitor e passa os pinos como parâmetro

struct Usuario {
  int id;
  String senha;
  String cardRFID;
};

Usuario usuarios[10];  // Tamanho máximo de 10 usuários, ajuste conforme necessário
int numUsuarios = 0;   // Número atual de usuários cadastrados


String senhaDigitada = "";
int tentativasIncorretas = 0;
const int MAX_TENTATIVAS = 3;
const int TEMPO_BLOQUEIO = 10000;
const String SENHA_ADMIN = "9A9D";

void setup() {
  SPI.begin(); 
  Wire .begin (); // Call the connection Wire
  keypad.begin (makeKeymap (keys)); // Call the connection
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  Serial.begin (2400);
  arduinoSerial.begin(2400);
  pinMode(espRxPin, INPUT);
  pinMode(espTxPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  mfrc522.PCD_Init();
  lcd.init();
  lcd.backlight();
  lcd.begin(16, 2);
  lcd.print("Digite a senha:");
}

void repetirSom(int pinoBuzzer, int frequencia, int duracao, int numeroRepeticoes) {
  for (int i = 0; i < numeroRepeticoes; ++i) {
    tone(pinoBuzzer, frequencia);
    delay(duracao);
    noTone(pinoBuzzer);
    delay(duracao);
  }
}

bool cartaoPertenceAoUsuario(String idCartao) {
  for (int i = 0; i < numUsuarios; i++) {
    if (usuarios[i].cardRFID == idCartao) {
      return true;  // Cartão pertence a um usuário cadastrado
    }
  }
  return false;  // Cartão não cadastrado
}

void enviarSmartLocker(String mensagem) {
  String chave;
    espSerial.begin(2400);
    espSerial.println("SmartLocker" + mensagem);
    espSerial.end();
}

void confereSmartLocker(){
  String smartLocker;
  espSerial.end();
  arduinoSerial.begin(2400);
  if (arduinoSerial.available()) {
    smartLocker = arduinoSerial.readString();
    enviarSmartLocker(smartLocker);
  }
  // read from port 0, send to port 1:
  if (Serial.available()) {
    smartLocker = Serial.readString();
    arduinoSerial.println(smartLocker);
  }
}

void dadosUser(){
   espSerial.begin(2400);
    String idString = String(usuarios[numUsuarios - 1].id);
    espSerial.println("ID_NV_USR:" + idString); 
    espSerial.println("RFID_NV_USR:" + usuarios[numUsuarios - 1].cardRFID);
    espSerial.println("SNH_NV_USR:" + usuarios[numUsuarios - 1].senha);
    espSerial.end();
}


String getCardID(MFRC522 mfrc522) {
    String cardID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      cardID.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : ""));
      cardID.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    return cardID;
}

bool isCardValid(String cardRFID) {
  for (int i = 0; i < numUsuarios; i++) {
    if (usuarios[i].cardRFID == cardRFID) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Acesso Permitido");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Trava liberada");
      delay(5000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Digite a senha:");
      return true;  // Cartão cadastrado
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Acesso Negado");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Digite a senha:");
  return false;  // Cartão não cadastrado
}

void cadastrarCartao() {
  lcd.clear();
  lcd.setCursor(0, 0); // Posiciona o cursor na primeira linha e na primeira coluna
  lcd.print("Modo Cadastro");
  delay(4000);
  lcd.clear();
  lcd.setCursor(0, 0); // Posiciona o cursor na primeira linha e na primeira coluna
  lcd.print("Aproxime um card");
  lcd.setCursor(0, 1); // Posiciona o cursor na segunda linha e na primeira coluna
  lcd.print("para Cadastro");
 
  // Aguarde até que um novo cartão RFID seja apresentado
  while (!mfrc522.PICC_IsNewCardPresent()) {
    // Aguarde
  }

  if (mfrc522.PICC_ReadCardSerial()) {
    String novoCartaoID = getCardID(mfrc522);
    mfrc522.PICC_HaltA();
    //cartoesValidos[numCartoes++] = novoCartaoID;
    Serial.println("Novo cartão cadastrado: " + novoCartaoID);
    lcd.clear();
    lcd.setCursor(0, 0); // Posiciona o cursor na primeira linha e na primeira coluna
    lcd.print("Cadastrado");
    lcd.setCursor(0, 1); // Posiciona o cursor na segunda linha e na primeira coluna
    lcd.print("com sucesso");         // Aguarda 2 segundos antes de limpar o Serial Monitor
  }
}

void processarTecla(char key) {
  char passwordUser[10];
  if (key == '#') {
      strcpy(passwordUser, senhaDigitada.c_str());
      espSerial.println("SNH_KEYPAD_USR:" + String(passwordUser));
    if (verificarSenha(senhaDigitada)) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Senha correta!");
      delay(2000);
      senhaDigitada = "";
      tentativasIncorretas = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Digite a senha:");
    } else {
      tentativasIncorretas++;
      repetirSom(buzzerPin, 1500, 200, 1);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Senha incorreta!");
      delay(2000);
      senhaDigitada = "";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Digite a senha:");

      if (tentativasIncorretas >= MAX_TENTATIVAS) {
        repetirSom(buzzerPin, 1500, 200, 3);
        lcd.clear();
        lcd.setCursor(0, 0); // Posiciona o cursor na primeira linha e na primeira coluna
        lcd.print("Acesso bloqueado");
        lcd.setCursor(0, 1); // Posiciona o cursor na segunda linha e na primeira coluna
        lcd.print("temporariamente");
        delay(TEMPO_BLOQUEIO);
        repetirSom(buzzerPin, 1000, 200, 3);
        tentativasIncorretas = 0;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Digite a senha:");
      }
    }
  } else if (key == '*') {
    // Apagar o último dígito
    if (senhaDigitada.length() > 0) {
      senhaDigitada = senhaDigitada.substring(0, senhaDigitada.length() - 1);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Digite a senha: " + senhaDigitada);
    }
  } else {
    senhaDigitada += key;
    
    if (senhaDigitada == SENHA_ADMIN) {
      cadastrarNovoUsuario();
      senhaDigitada = "";
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Digite a senha: " + senhaDigitada);
    }
  }
}

bool verificarSenha(String senha) {
  for (int i = 0; i < numUsuarios; i++) {
    if (senha == usuarios[i].senha) {
      return true;
    }
  }
  return false;
}

void cadastrarNovoUsuario() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Modo Administrador");
  lcd.setCursor(0, 1);
  lcd.print("cadastro");
  delay(3000);
  lcd.clear();
  lcd.print("Aproxime o card:");

  String cardRFID = getCardID(mfrc522);

  while (!mfrc522.PICC_IsNewCardPresent()) {
    
  }
  
  if (mfrc522.PICC_ReadCardSerial()) {
    mfrc522.PICC_HaltA();
    Serial.println("Novo cartão cadastrado: " + cardRFID);
    lcd.clear();
    lcd.setCursor(0, 0); // Posiciona o cursor na primeira linha e na primeira coluna
    lcd.print("Cartao Cadastrado");
    repetirSom(buzzerPin, 1000, 200, 3); // Repetir som médio 3 vezes
    delay(3000);
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Digite a senha");
  lcd.setCursor(0, 1);
  lcd.print("do usuario");
 

  String novaSenha = "";
  char key;
  do {
    key = keypad.getKey();
    if (key && key != '#' && key != '*') {
      tone(buzzerPin, 1000, 100);
      novaSenha += key;
      //lcd.print('*'); // Exibe "*" na tela
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Digite a senha");
      lcd.setCursor(0, 1);
      lcd.print("do usuario");
    }
  } while (key != '#');

  if (numUsuarios < sizeof(usuarios) / sizeof(usuarios[0])) {
    usuarios[numUsuarios].id = numUsuarios + 1;
    usuarios[numUsuarios].cardRFID = cardRFID;
    usuarios[numUsuarios].senha = novaSenha;
    numUsuarios++;

    Serial.println("--------------------------------");
    Serial.print("ID: ");
    Serial.println(usuarios[numUsuarios - 1].id);
    Serial.print("Card RFID: ");
    Serial.println(usuarios[numUsuarios - 1].cardRFID);
    Serial.print("Senha: ");
    Serial.println(usuarios[numUsuarios - 1].senha);
    Serial.println("--------------------------------");

    dadosUser();

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Usuario");
    lcd.setCursor(0, 1);
    lcd.print("cadastrado!");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Digite a senha:");
  } else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Limite atingido!");
    delay(2000);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Digite a senha:");
  }
}

void loop() {
  if (door == 0) {
    confereSmartLocker();
  } else {
    char key = keypad.getKey();
    if (key) {
      tone(buzzerPin, 1000, 100);
      processarTecla(key);
    }

    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      String cardID = getCardID(mfrc522);  // Obtém o ID do cartão RFID
      mfrc522.PICC_HaltA();
      tone(buzzerPin, 1500, 200);  // Frequência: 1500 Hz, Duração: 200 ms 
      if (isCardValid(cardID)) {
        espSerial.println("CARD_RFID:" + cardID);
        repetirSom(buzzerPin, 1000, 200, 3);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Digite a senha:");
      } else {
        repetirSom(buzzerPin, 1500, 200, 1);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Acesso negado!");
        delay(3000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Digite a senha:");
      }
    }
  }
}
