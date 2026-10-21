/*
  ==========================================================================
  TESTE DOS SENSORES  -  para achar a melhor posicao do ima (Arduino Uno)
  ==========================================================================
  Para que serve:
    Antes de colar qualquer coisa na luva, prenda o sensor e o ima com fita
    e use este programa para ver se o sinal acompanha o dedo do comeco ao fim.
    Nao precisa de servo nem de pilha: so o Uno na USB e os sensores.

  Ligacoes (iguais as da versao 1):
    SS49E: VCC -> 5V | GND -> GND | OUT -> A0 (polegar) A1 A2 A3 A4 (minimo)
    Pode testar com 1 sensor so (no A0): os outros mostram valores soltos.

  Como usar:
    1. Grave e abra Ferramentas -> Serial Plotter, em 115200.
    2. Dobre o dedo DEVAGAR, de esticado ate o punho fechado, varias vezes.
    3. O que voce quer ver: uma curva LISA, que muda do comeco ao fim.
         - Fica reta (achatada) no comeco? O ima esta perto demais: afaste.
         - Fica reta no fim?                O ima esta longe demais: aproxime.
         - Variacao menor que ~80?          Ima mais forte ou mais perto.
    4. Abra o Monitor Serial e digite t para ver o resumo em texto
       (minimo, maximo e variacao de cada sensor). Digite r para zerar.
  ==========================================================================
*/

const uint8_t PINO_SENSOR[5] = { A0, A1, A2, A3, A4 };
const char*   NOME_DEDO[5]   = { "polegar", "indicador", "medio", "anelar", "minimo" };
const int     VARIACAO_BOA   = 80;    // minimo para o movimento ficar suave (quanto mais, melhor)

int minimo[5];
int maximo[5];
bool modoTexto = false;
unsigned long proximoPrint = 0;

int lerSensor(uint8_t i) {
  int soma = 0;
  for (uint8_t k = 0; k < 4; k++) soma += analogRead(PINO_SENSOR[i]);
  return soma / 4;
}

void zerar() {
  for (uint8_t i = 0; i < 5; i++) { minimo[i] = 1023; maximo[i] = 0; }
}

void setup() {
  Serial.begin(115200);
  zerar();
}

void loop() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 't' || c == 'T') modoTexto = !modoTexto;
    if (c == 'r' || c == 'R') zerar();
  }

  int v[5];
  for (uint8_t i = 0; i < 5; i++) {
    v[i] = lerSensor(i);
    if (v[i] < minimo[i]) minimo[i] = v[i];
    if (v[i] > maximo[i]) maximo[i] = v[i];
  }

  if ((long)(millis() - proximoPrint) < 0) return;

  if (!modoTexto) {
    // Serial Plotter: nome:valor,nome:valor  (20 vezes por segundo)
    proximoPrint = millis() + 50;
    for (uint8_t i = 0; i < 5; i++) {
      Serial.print(NOME_DEDO[i]);
      Serial.print(':');
      Serial.print(v[i]);
      Serial.print(i < 4 ? ',' : '\n');
    }
  } else {
    // Resumo em texto (1 vez por segundo)
    proximoPrint = millis() + 1000;
    for (uint8_t i = 0; i < 5; i++) {
      int variacao = maximo[i] - minimo[i];
      Serial.print(NOME_DEDO[i]);
      Serial.print(F(": agora="));
      Serial.print(v[i]);
      Serial.print(F(" min="));
      Serial.print(minimo[i]);
      Serial.print(F(" max="));
      Serial.print(maximo[i]);
      Serial.print(F(" variacao="));
      Serial.print(variacao);
      if (v[i] < 20 || v[i] > 1003)     Serial.println(F("  FIO SOLTO?"));
      else if (variacao >= VARIACAO_BOA) Serial.println(F("  boa"));
      else                               Serial.println(F("  pouca: aproxime o ima"));
    }
    Serial.println();
  }
}
