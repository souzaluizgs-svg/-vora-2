/*
  ==========================================================================
  MAO ROBOTICA - VERSAO 1 (COM FIO)  -  Arduino Uno
  ==========================================================================
  O que faz:
    Le 5 sensores Hall da luva (um por dedo). Quando o sensor chega perto
    do ima, o dedo da mao robotica fecha. Quando afasta, o dedo abre.

  Ligacoes:
    Sensores Hall (SS49E):  VCC -> 5V do Uno | GND -> GND | OUT -> pino abaixo
      polegar = A0, indicador = A1, medio = A2, anelar = A3, minimo = A4
    Servos SG90 (sinal):    polegar = D3, indicador = D5, medio = D6,
                            anelar = D9, minimo = D10
    Alimentacao dos servos: pack 4 pilhas AA recarregaveis (4,8 V).
                            + do pack -> fio vermelho dos servos
                            - do pack -> fio marrom dos servos E GND do Uno
    O Uno fica ligado na USB do computador.
    ATENCAO: os sensores usam o 5V do UNO (nao o pack). Nunca ligue o +
    do pack no pino 5V do Uno: so o GND (-) e compartilhado.

  Ao ligar (CALIBRACAO, so no modo SS49E):
    1) LED do pino 13 piscando DEVAGAR: deixe a mao ABERTA (~3 s)
    2) LED piscando RAPIDO: feche o PUNHO (~3 s)
    3) LED apagado: pronto, pode usar!
    Para calibrar de novo, aperte o botao RESET do Uno.

  Abra o Monitor Serial em 115200 para ver as mensagens e os numeros.
  ==========================================================================
*/

#include <Servo.h>

// ---------------- Tipo de sensor ----------------
// 0 = SS49E (analogico, recomendado)   1 = A3144 (digital, liga/desliga)
// A3144: VCC -> 5V, GND -> GND, OUT -> A0..A4. Usa o pull-up interno.
// Ele so liga com o polo SUL do ima virado para a face com escrita.
#define USAR_A3144 0

// ---------------- Pinos (ordem: polegar, indicador, medio, anelar, minimo)
const uint8_t PINO_SENSOR[5] = { A0, A1, A2, A3, A4 };
const uint8_t PINO_SERVO[5]  = { 3, 5, 6, 9, 10 };
const char*   NOME_DEDO[5]   = { "polegar", "indicador", "medio", "anelar", "minimo" };
const uint8_t PINO_LED = 13;

// ---------------- Calibracao dos servos ----------------
// Use o programa "teste_servos" para descobrir estes numeros.
// Sempre entre 0 e 180. ABERTO pode ser MAIOR que FECHADO (servo montado
// ao contrario), o programa aceita os dois jeitos.
// Copie os MESMOS numeros para o "mao_sem_fio" se for usar a versao 2.
int ANG_ABERTO[5]  = { 20, 20, 20, 20, 20 };     // dedo esticado
int ANG_FECHADO[5] = { 160, 160, 160, 160, 160 }; // dedo dobrado

// ---------------- Ajustes finos ----------------
const unsigned long PERIODO_MS = 20;  // 50 vezes por segundo
const int   ZONA_MORTA = 2;           // graus: mudancas menores sao ignoradas (evita tremedeira)
const int   PASSO_MAX  = 6;           // graus por ciclo: limita a velocidade (e o pico de corrente)
const float ALFA = 0.3;               // filtro: perto de 0 = mais suave/lento, 1 = sem filtro
const unsigned long TEMPO_CALIBRACAO_MS = 3000; // tempo de cada etapa (use 2000 ou mais: o 1o segundo e ignorado)
const int   DIFERENCA_MINIMA = 30;    // leitura minima de diferenca entre aberto e fechado (nunca 0)
const float LIMIAR_FECHAR = 70;       // so modo A3144: fecha quando o filtro passa de 70 %
const float LIMIAR_ABRIR  = 30;       // so modo A3144: abre quando cai abaixo de 30 % (histerese)

// ---------------- Variaveis ----------------
Servo servo[5];
int   angAtual[5];      // angulo que o servo esta agora
int   alvo[5];          // angulo para onde ele quer ir
int   leituraCrua[5];   // ultimo valor lido do sensor
int   valorAberto[5];   // leitura com a mao aberta (calibracao)
int   valorFechado[5];  // leitura com o punho fechado (calibracao)
bool  sensorOk[5];      // false = sensor quase nao mudou na calibracao
float pctFiltrado[5];   // fechamento filtrado, 0 a 100 %
bool  dedoFechado[5];   // so modo A3144
int   pctFinal[5];      // fechamento usado nos servos, 0 a 100 %

unsigned long proximoCiclo = 0;
unsigned long proximoPrint = 0;

// Le um sensor. SS49E: media de 4 leituras (0..1023). A3144: 1 = ima perto.
int lerSensor(uint8_t i) {
#if USAR_A3144
  return (digitalRead(PINO_SENSOR[i]) == LOW) ? 1 : 0;  // A3144 puxa para LOW com ima
#else
  int soma = 0;
  for (uint8_t k = 0; k < 4; k++) soma += analogRead(PINO_SENSOR[i]);
  return soma / 4;
#endif
}

// Mede a media dos 5 sensores durante TEMPO_CALIBRACAO_MS, piscando o LED.
// O primeiro 1 segundo e ignorado (tempo para voce posicionar a mao).
void medirMedia(int resultado[5], unsigned long meioPiscar) {
  long soma[5] = { 0, 0, 0, 0, 0 };
  long n = 0;
  unsigned long inicio = millis();
  unsigned long ultimoPisca = inicio;
  bool led = false;
  while (millis() - inicio < TEMPO_CALIBRACAO_MS) {
    if (millis() - ultimoPisca >= meioPiscar) {
      ultimoPisca = millis();
      led = !led;
      digitalWrite(PINO_LED, led);
    }
    if (millis() - inicio > 1000) {
      for (uint8_t i = 0; i < 5; i++) soma[i] += lerSensor(i);
      n++;
    }
    delay(5);
  }
  if (n == 0) n = 1;  // protecao: TEMPO_CALIBRACAO_MS curto demais (1 s ou menos)
  for (uint8_t i = 0; i < 5; i++) resultado[i] = soma[i] / n;
  digitalWrite(PINO_LED, LOW);
}

void calibrarSensores() {
  Serial.println(F("CALIBRACAO 1/2: deixe a mao ABERTA (LED piscando devagar)..."));
  medirMedia(valorAberto, 400);
  Serial.println(F("CALIBRACAO 2/2: FECHE o punho (LED piscando rapido)..."));
  medirMedia(valorFechado, 80);

  Serial.println(F("Resultado:"));
  for (uint8_t i = 0; i < 5; i++) {
    int diferenca = abs(valorFechado[i] - valorAberto[i]);
    sensorOk[i] = (diferenca >= DIFERENCA_MINIMA && diferenca > 0);  // > 0: evita dividir por zero
    Serial.print(F("  "));
    Serial.print(NOME_DEDO[i]);
    Serial.print(F(": aberto="));
    Serial.print(valorAberto[i]);
    Serial.print(F(" fechado="));
    Serial.print(valorFechado[i]);
    if (sensorOk[i]) {
      Serial.println(F("  OK"));
    } else {
      Serial.println(F("  POUCA DIFERENCA! Aproxime o ima do sensor. Este dedo vai ficar parado."));
    }
  }
}

// Converte a leitura do sensor em fechamento de 0 a 100 %.
// Funciona com o ima de qualquer lado (a leitura pode subir ou descer).
int paraPorcento(uint8_t i, int leitura) {
  if (!sensorOk[i]) return 0;
  long p = (long)(leitura - valorAberto[i]) * 100L / (valorFechado[i] - valorAberto[i]);
  return constrain(p, 0, 100);
}

// Leva o servo i na direcao do fechamento pct (0..100 %),
// com zona morta e limite de velocidade.
void atualizarServo(uint8_t i, int pct) {
  int alvoNovo = map(pct, 0, 100, ANG_ABERTO[i], ANG_FECHADO[i]);
  bool extremo = (pct == 0 || pct == 100);      // nos extremos vai ate o fim
  if (abs(alvoNovo - alvo[i]) > ZONA_MORTA || extremo) {
    alvo[i] = alvoNovo;
  }
  int falta = alvo[i] - angAtual[i];
  if (falta != 0) {
    angAtual[i] += constrain(falta, -PASSO_MAX, PASSO_MAX);
    servo[i].write(angAtual[i]);
  }
}

void imprimirDepuracao() {
  Serial.print(F("cru:"));
  for (uint8_t i = 0; i < 5; i++) { Serial.print(' '); Serial.print(leituraCrua[i]); }
  Serial.print(F(" | %:"));
  for (uint8_t i = 0; i < 5; i++) { Serial.print(' '); Serial.print(pctFinal[i]); }
  Serial.print(F(" | ang:"));
  for (uint8_t i = 0; i < 5; i++) { Serial.print(' '); Serial.print(angAtual[i]); }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  pinMode(PINO_LED, OUTPUT);
  Serial.println(F("\n=== Mao Robotica - Versao 1 (com fio) ==="));

  // Liga os servos UM DE CADA VEZ, ja na posicao aberta.
  for (uint8_t i = 0; i < 5; i++) {
    servo[i].write(ANG_ABERTO[i]);
    servo[i].attach(PINO_SERVO[i]);
    angAtual[i] = ANG_ABERTO[i];
    alvo[i] = ANG_ABERTO[i];
    pctFiltrado[i] = 0;
    pctFinal[i] = 0;
    dedoFechado[i] = false;
    delay(300);
  }

#if USAR_A3144
  for (uint8_t i = 0; i < 5; i++) pinMode(PINO_SENSOR[i], INPUT_PULLUP);
  Serial.println(F("Sensor: A3144 (digital). Sem calibracao."));
#else
  Serial.println(F("Sensor: SS49E (analogico)."));
  calibrarSensores();
#endif

  Serial.println(F("Pronto! Mexa os dedos da luva."));
  proximoCiclo = millis();
}

void loop() {
  // Tudo acontece a cada PERIODO_MS (20 ms = 50 vezes por segundo)
  if ((long)(millis() - proximoCiclo) < 0) return;
  proximoCiclo += PERIODO_MS;

  for (uint8_t i = 0; i < 5; i++) {
    leituraCrua[i] = lerSensor(i);

#if USAR_A3144
    // Liga/desliga: filtra e usa dois limites (histerese) para nao "piscar"
    float bruto = leituraCrua[i] ? 100.0 : 0.0;
    pctFiltrado[i] += ALFA * (bruto - pctFiltrado[i]);
    if (!dedoFechado[i] && pctFiltrado[i] > LIMIAR_FECHAR) dedoFechado[i] = true;
    if (dedoFechado[i] && pctFiltrado[i] < LIMIAR_ABRIR)   dedoFechado[i] = false;
    pctFinal[i] = dedoFechado[i] ? 100 : 0;
#else
    // Analogico: converte para % e suaviza (media movel exponencial)
    int bruto = paraPorcento(i, leituraCrua[i]);
    pctFiltrado[i] += ALFA * (bruto - pctFiltrado[i]);
    pctFinal[i] = (int)(pctFiltrado[i] + 0.5);
#endif

    atualizarServo(i, pctFinal[i]);
  }

  // Mostra os numeros 5 vezes por segundo
  if ((long)(millis() - proximoPrint) >= 0) {
    proximoPrint = millis() + 200;
    imprimirDepuracao();
  }
}
