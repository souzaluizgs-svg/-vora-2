/*
  ==========================================================================
  MAO ROBOTICA - VERSAO 2 (SEM FIO) - MAO  -  Arduino Uno
  ==========================================================================
  !!! ATENCAO: DESCONECTE O FIO DO PINO D0 ANTES DE GRAVAR ESTE PROGRAMA !!!
  (O D0 e o mesmo pino que a USB usa para gravar. Com o ESP32 ligado nele,
   a gravacao falha. Grave, depois ligue o fio de novo.)

  O que faz:
    Recebe do ESP32 receptor (pelo pino D0) quanto cada dedo da luva esta
    fechado (0 a 100 %) e move os servos da mao robotica.
    Se ficar 500 ms sem receber nada certo, ABRE a mao (seguranca).

  Ligacoes:
    ESP32 receptor GPIO17 (TX2) -> D0 (RX) do Uno  (fio direto, SEM resistor
                                   e SEM divisor de tensao nesse fio)
    ESP32 GND                   -> GND do Uno (OBRIGATORIO)
    NUNCA ligue o D1 (TX) do Uno no ESP32: os 5 V do Uno queimam o ESP32.
    Nao deixe o fio no D0 com o ESP32 desligado (ligue os dois juntos).
    Servos SG90 (sinal):  polegar = D3, indicador = D5, medio = D6,
                          anelar = D9, minimo = D10
    Pack 4 pilhas AA (4,8 V): + -> vermelho dos servos,
                              - -> marrom dos servos E GND do Uno.

  Quadro esperado (7 bytes, 115200 baud):
    0xFF, p0, p1, p2, p3, p4, xor(p0..p4)    (p = 0..100)

  LED do pino 13: aceso = chegando dados bons | apagado = sem sinal |
                  piscando = modo demonstracao.

  MODO DEMONSTRACAO (plano B se o radio falhar na apresentacao):
    botao entre D2 e GND (opcional). Segure 2 s para ligar ou desligar.
    A mao faz gestos sozinha e ignora a luva.
  O Monitor Serial (115200) mostra o que chegou. NAO digite nada nele
  enquanto o ESP32 estiver ligado no D0.
  ==========================================================================
*/

#include <Servo.h>

// ---------------- Pinos (ordem: polegar, indicador, medio, anelar, minimo)
const uint8_t PINO_SERVO[5] = { 3, 5, 6, 9, 10 };
const uint8_t PINO_LED = 13;
const uint8_t PINO_BOTAO = 2;   // opcional: botao entre D2 e GND

// ---------------- Calibracao dos servos (mesmos numeros da versao 1) -----
// Copie do "teste_servos"/"mao_com_fio". Sempre entre 0 e 180.
int ANG_ABERTO[5]  = { 20, 20, 20, 20, 20 };     // dedo esticado
int ANG_FECHADO[5] = { 160, 160, 160, 160, 160 }; // dedo dobrado

// ---------------- Ajustes finos ----------------
const unsigned long PERIODO_MS = 20;        // 50 vezes por segundo
const int   ZONA_MORTA = 2;                 // graus
const int   PASSO_MAX  = 6;                 // graus por ciclo (limite de velocidade)
const unsigned long TEMPO_FAILSAFE_MS = 500; // sem dados por 500 ms -> abre a mao
const unsigned long APERTO_LONGO_MS = 2000;  // segurar o botao 2 s = modo demonstracao

// ---------------- Gestos do modo demonstracao (iguais aos da versao 1) ----
// Fechamento de cada dedo em % (polegar, indicador, medio, anelar, minimo).
struct Gesto {
  uint8_t pct[5];
  uint16_t ms;
};
const Gesto GESTOS[] = {
  { {   0,   0,   0,   0,   0 }, 1500 },  // mao aberta
  { { 100, 100, 100, 100, 100 }, 1500 },  // punho
  { { 100,   0, 100, 100, 100 }, 1000 },  // conta 1
  { { 100,   0,   0, 100, 100 }, 1000 },  // conta 2
  { { 100,   0,   0,   0, 100 }, 1000 },  // conta 3
  { { 100,   0,   0,   0,   0 }, 1000 },  // conta 4
  { {   0,   0,   0,   0,   0 }, 1200 },  // conta 5
  { {   0, 100, 100, 100, 100 }, 1800 },  // joinha
  { { 100,   0, 100, 100,   0 }, 1800 },  // rock
  { {  70,  70,   0,   0,   0 }, 1800 },  // pinca / "ok"
  { {   0,   0,   0,   0, 100 },  500 },  // onda: fecha do minimo ao polegar...
  { {   0,   0,   0, 100, 100 },  500 },
  { {   0,   0, 100, 100, 100 },  500 },
  { {   0, 100, 100, 100, 100 },  500 },
  { { 100, 100, 100, 100, 100 },  800 },
  { { 100, 100, 100, 100,   0 },  500 },  // ...e abre na mesma ordem
  { { 100, 100, 100,   0,   0 },  500 },
  { { 100, 100,   0,   0,   0 },  500 },
  { { 100,   0,   0,   0,   0 },  500 },
};
const uint8_t NUM_GESTOS = sizeof(GESTOS) / sizeof(GESTOS[0]);

// ---------------- Variaveis ----------------
Servo servo[5];
int angAtual[5];
int alvo[5];
int pctRecebido[5];             // ultimo quadro valido
unsigned long momentoUltimoQuadro = 0;
bool jaRecebeu = false;
unsigned long quadrosBons = 0;
unsigned long quadrosRuins = 0;

// Maquina de estados do leitor de quadros
uint8_t estado = 0;             // 0 = esperando 0xFF, 1 = lendo dedos, 2 = lendo xor
uint8_t bufferDedos[5];
uint8_t posicao = 0;

bool modoDemo = false;
uint8_t gestoAtual = 0;
unsigned long inicioGesto = 0;
bool botaoAntes = false;
unsigned long inicioAperto = 0;
bool apertoLongoFeito = false;

unsigned long proximoCiclo = 0;
unsigned long proximoPrint = 0;

// Le os bytes que chegaram e monta o quadro. Retorna true se fechou um quadro bom.
bool lerQuadros() {
  bool chegouQuadro = false;
  while (Serial.available() > 0) {
    uint8_t b = Serial.read();

    if (b == 0xFF) {            // 0xFF sempre e comeco de quadro
      if (estado != 0) quadrosRuins++;   // quadro anterior ficou pela metade
      estado = 1;
      posicao = 0;
      continue;
    }

    if (estado == 1) {          // lendo os 5 dedos
      if (b > 100) {            // valor impossivel: descarta
        quadrosRuins++;
        estado = 0;
        continue;
      }
      bufferDedos[posicao++] = b;
      if (posicao == 5) estado = 2;
    } else if (estado == 2) {   // byte de conferencia (xor)
      uint8_t x = 0;
      for (uint8_t i = 0; i < 5; i++) x ^= bufferDedos[i];
      if (x == b) {
        for (uint8_t i = 0; i < 5; i++) pctRecebido[i] = bufferDedos[i];
        momentoUltimoQuadro = millis();
        jaRecebeu = true;
        quadrosBons++;
        chegouQuadro = true;
      } else {
        quadrosRuins++;
      }
      estado = 0;
    }
    // estado 0 e byte diferente de 0xFF: lixo, ignora
  }
  return chegouQuadro;
}

// Leva o servo i na direcao do fechamento pct (0..100 %),
// com zona morta e limite de velocidade. (Igual a versao 1.)
void atualizarServo(uint8_t i, int pct) {
  int alvoNovo = map(pct, 0, 100, ANG_ABERTO[i], ANG_FECHADO[i]);
  bool extremo = (pct == 0 || pct == 100);
  if (abs(alvoNovo - alvo[i]) > ZONA_MORTA || extremo) {
    alvo[i] = alvoNovo;
  }
  int falta = alvo[i] - angAtual[i];
  if (falta != 0) {
    angAtual[i] += constrain(falta, -PASSO_MAX, PASSO_MAX);
    servo[i].write(angAtual[i]);
  }
}

// Botao entre D2 e GND (INPUT_PULLUP: apertado = LOW). Segurar 2 s liga
// ou desliga a demonstracao. Sem botao ligado, nada acontece.
void lerBotao() {
  bool apertado = (digitalRead(PINO_BOTAO) == LOW);
  unsigned long agora = millis();
  if (apertado && !botaoAntes) {
    inicioAperto = agora;
    apertoLongoFeito = false;
  }
  if (apertado && !apertoLongoFeito && agora - inicioAperto >= APERTO_LONGO_MS) {
    apertoLongoFeito = true;
    modoDemo = !modoDemo;
    gestoAtual = 0;
    inicioGesto = agora;
    Serial.println(modoDemo ? F(">> MODO DEMONSTRACAO ligado") : F(">> MODO DEMONSTRACAO desligado"));
  }
  botaoAntes = apertado;
}

void setup() {
  // 115200 = o MESMO valor do Serial2.begin do receptor_esp32.
  // (O Uno a 16 MHz gera 115200 com 2,1 % de erro; costuma funcionar. Se o
  //  contador "ruins" crescer sem parar, troque para 38400 AQUI, no
  //  receptor_esp32 E no Monitor Serial.)
  Serial.begin(115200);
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_BOTAO, INPUT_PULLUP);
  Serial.println(F("\n=== Mao Robotica - Versao 2 (sem fio) - Uno ==="));

  // Liga os servos UM DE CADA VEZ, ja com a mao aberta.
  for (uint8_t i = 0; i < 5; i++) {
    servo[i].write(ANG_ABERTO[i]);
    servo[i].attach(PINO_SERVO[i]);
    angAtual[i] = ANG_ABERTO[i];
    alvo[i] = ANG_ABERTO[i];
    pctRecebido[i] = 0;
    delay(300);
  }

  // Joga fora o que chegou enquanto ligava os servos
  while (Serial.available() > 0) Serial.read();

  Serial.println(F("Esperando dados do ESP32 no pino D0..."));
  proximoCiclo = millis();
}

void loop() {
  lerQuadros();    // le sempre, para o buffer da serial nao encher
  lerBotao();

  if ((long)(millis() - proximoCiclo) < 0) return;
  proximoCiclo += PERIODO_MS;

  bool sinalOk = jaRecebeu && (millis() - momentoUltimoQuadro < TEMPO_FAILSAFE_MS);

  if (modoDemo) {
    digitalWrite(PINO_LED, (millis() / 250) % 2);   // pisca = demonstracao
    if (millis() - inicioGesto >= GESTOS[gestoAtual].ms) {
      gestoAtual = (gestoAtual + 1) % NUM_GESTOS;
      inicioGesto = millis();
    }
    for (uint8_t i = 0; i < 5; i++) atualizarServo(i, GESTOS[gestoAtual].pct[i]);
  } else {
    digitalWrite(PINO_LED, sinalOk ? HIGH : LOW);
    for (uint8_t i = 0; i < 5; i++) {
      int pct = sinalOk ? pctRecebido[i] : 0;   // sem sinal = mao aberta
      atualizarServo(i, pct);
    }
  }

  // Mostra no Monitor Serial 4 vezes por segundo
  if ((long)(millis() - proximoPrint) >= 0) {
    proximoPrint = millis() + 250;
    if (modoDemo) {
      Serial.print(F("DEMO gesto "));
      Serial.print(gestoAtual);
    } else if (sinalOk) {
      Serial.print(F("OK  %:"));
      for (uint8_t i = 0; i < 5; i++) { Serial.print(' '); Serial.print(pctRecebido[i]); }
      Serial.print(F(" | ang:"));
      for (uint8_t i = 0; i < 5; i++) { Serial.print(' '); Serial.print(angAtual[i]); }
    } else {
      Serial.print(F("SEM SINAL -> mao aberta"));
    }
    Serial.print(F(" | bons "));
    Serial.print(quadrosBons);
    Serial.print(F(" ruins "));
    Serial.println(quadrosRuins);
  }
}
