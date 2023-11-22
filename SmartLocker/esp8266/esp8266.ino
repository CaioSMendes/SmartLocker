void setup() {
  Serial.begin(2400); // Inicia a comunicação serial com o computador
}

void loop() {
  String numbersOnly;
  // Verifica se há dados disponíveis para leitura
  if (Serial.available() > 0) {
    // Lê a mensagem do Arduino
    String mensagem = Serial.readStringUntil('\n');

    // Imprime a mensagem no console do ESP8266
    Serial.println(mensagem);// Msg vinda do Arduino

    String tresPrimeirasLetras = mensagem.substring(0, 3);
    // Verifica se as três primeiras letras são "LGN"
    if (tresPrimeirasLetras.equals("LGN")) {
      Serial.println("LOGIN");
      // Faça o que você precisa fazer aqui
    }else if (mensagem.startsWith("SmartLockerLS")){
      numbersOnly = mensagem.substring(12);
      Serial.println("SmartLocker:"+numbersOnly);
      //Serial.println(mensagem);
    }
  }
}
