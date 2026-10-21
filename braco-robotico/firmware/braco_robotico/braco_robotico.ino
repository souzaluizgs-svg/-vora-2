/*
  ==========================================================================
  BRACO ROBOTICO (Mark 2) - joysticks + GRAVAR E REPETIR  -  Arduino Uno
  ==========================================================================
  O que faz:
    - Voce move o braco com os 2 joysticks. Quanto mais inclina, mais rapido
      a junta anda; soltou, ela PARA onde esta (controle por velocidade).
    - Clique no joystick 1 = GRAVA a posicao atual (ate 40 posicoes).
    - Clique no joystick 2 = o braco REPETE sozinho as posicoes gravadas,
      em loop. Clique de novo (ou mexa um joystick) para parar.
    - A gravacao fica na memoria (EEPROM): sobrevive a desligar o Uno.

  Comandos nos botoes dos joysticks:
    Joystick 1, clique ............ grava a posicao atual
    Joystick 1, segurar 2 s ....... APAGA todas as posicoes gravadas
    Joystick 2, clique ............ repete / para
    Joystick 2, segurar 2 s ....... volta para a posicao inicial
    (No Monitor Serial, 115200: g grava, r repete/para, l apaga,
     i posicao inicial, p lista as posicoes, ? ajuda.)

  Ligacoes (CONFIRA COM O MANUAL DO KIT; se for diferente, mude os numeros
  em "Pinos" logo abaixo):
    Servos na shield V5 (fileira S-V-G de cada pino):
      base = D3 | ombro = D5 | cotovelo = D6 | garra = D9
    Joystick 1: VRx -> A0 (base)     VRy -> A1 (ombro)     SW -> D2
    Joystick 2: VRx -> A2 (garra)    VRy -> A3 (cotovelo)  SW -> D4
    Joysticks: +5V -> V, GND -> G da shield.

  ENERGIA (importante):
    Fonte 5 V 2 A do kit -> borne azul da shield (use o borne P4 femea).
    TIRE o jumper "SEL" da shield: assim os servos usam so a fonte.
    O Uno fica na USB (notebook ou carregador de celular).
    NUNCA ligue a fonte de 5 V no conector P4 do proprio Uno: ali ele
    precisa de uns 7 V ou mais.

  Ao ligar: NAO toque nos joysticks por 1 segundo (o programa mede o centro
  de cada um). Depois os servos ligam um de cada vez.
  ==========================================================================
*/

#include <Servo.h>
#include <EEPROM.h>

// ---------------- Pinos (ordem: base, ombro, cotovelo, garra) ----------------
const uint8_t PINO_SERVO[4]    = { 3, 5, 6, 9 };
const uint8_t PINO_EIXO[4]     = { A0, A1, A3, A2 };   // qual eixo mexe cada junta
const char*   NOME_JUNTA[4]    = { "base", "ombro", "cotovelo", "garra" };
const uint8_t PINO_BOTAO_GRAVAR = 2;   // SW do joystick 1
const uint8_t PINO_BOTAO_TOCAR  = 4;   // SW do joystick 2
const uint8_t PINO_LED = 13;

// Se uma junta anda para o lado contrario do joystick, troque false por true.
const bool INVERTER[4] = { false, false, false, false };

// ---------------- Limites de cada junta (em graus, 0 a 180) ----------------
// Mexa a junta devagar com o joystick e veja o angulo no Monitor Serial.
// Anote onde ela comeca a forcar ou bater e coloque aqui, com 5 graus de folga.
// Isso protege o servo e as pecas impressas.
int ANG_MIN[4]     = {  10,  30,  30,  60 };
int ANG_MAX[4]     = { 170, 150, 150, 120 };
int ANG_INICIAL[4] = {  90,  90,  90,  90 };

// ---------------- Ajustes finos ----------------
const unsigned long PERIODO_MS = 20;       // 50 vezes por segundo
const int   ZONA_MORTA = 70;               // folga do joystick em torno do centro (0..512)
const float VEL_MAX[4] = { 90, 70, 70, 120 }; // graus por segundo com o joystick no fim
const float VEL_REPETIR = 60;              // graus por segundo ao repetir
const unsigned long PAUSA_POSICAO_MS = 400; // parada em cada posicao gravada
const unsigned long APERTO_LONGO_MS = 2000;
const uint8_t MAX_POSICOES = 40;

// ---------------- Gravacao na EEPROM ----------------
// Endereco 0: assinatura (2 bytes) e quantidade (1 byte). Depois, 4 bytes por posicao.
const uint16_t ASSINATURA = 0x4252;        // "BR"
uint8_t posicoes[MAX_POSICOES][4];
uint8_t numPosicoes = 0;

// ---------------- Variaveis ----------------
Servo servo[4];
float angulo[4];          // angulo atual (com casas decimais, para andar suave)
int   anguloEscrito[4];   // ultimo valor enviado ao servo
int   centro[4];          // leitura do joystick parado

bool  repetindo = false;
uint8_t alvoAtual = 0;
bool  esperando = false;
unsigned long inicioEspera = 0;

bool  indoParaInicio = false;

struct Botao {
  uint8_t pino;
  bool antes;
  unsigned long inicio;
  bool longoFeito;
};
Botao botaoGravar = { PINO_BOTAO_GRAVAR, false, 0, true };
Botao botaoTocar  = { PINO_BOTAO_TOCAR,  false, 0, true };

unsigned long proximoCiclo = 0;
unsigned long proximoPrint = 0;

// ======================= EEPROM =======================

void salvarPosicoes() {
  EEPROM.put(0, ASSINATURA);
  EEPROM.update(2, numPosicoes);
  for (uint8_t p = 0; p < numPosicoes; p++)
    for (uint8_t j = 0; j < 4; j++) EEPROM.update(3 + p * 4 + j, posicoes[p][j]);
}

void carregarPosicoes() {
  uint16_t a;
  EEPROM.get(0, a);
  uint8_t n = EEPROM.read(2);
  if (a != ASSINATURA || n > MAX_POSICOES) { numPosicoes = 0; return; }
  numPosicoes = n;
  for (uint8_t p = 0; p < numPosicoes; p++)
    for (uint8_t j = 0; j < 4; j++) posicoes[p][j] = EEPROM.read(3 + p * 4 + j);
}

// ======================= SERVOS =======================

void escreverServo(uint8_t j) {
  int a = (int)(angulo[j] + 0.5);
  if (a != anguloEscrito[j]) {
    servo[j].write(a);
    anguloEscrito[j] = a;
  }
}

// Leva a junta j ate o alvo, no maximo "passo" graus neste ciclo.
// Devolve true quando chegou.
bool andarAte(uint8_t j, float alvo, float passo) {
  float falta = alvo - angulo[j];
  if (falta > passo) angulo[j] += passo;
  else if (falta < -passo) angulo[j] -= passo;
  else angulo[j] = alvo;
  escreverServo(j);
  return angulo[j] == alvo;
}

// ======================= JOYSTICKS =======================

// Velocidade pedida pelo joystick da junta j, em graus por segundo.
float velocidadeJoystick(uint8_t j) {
  int desvio = analogRead(PINO_EIXO[j]) - centro[j];
  if (INVERTER[j]) desvio = -desvio;
  if (abs(desvio) < ZONA_MORTA) return 0;
  // de ZONA_MORTA ate 512 -> de 0 ate VEL_MAX (com o sinal da direcao)
  float fracao = (float)(abs(desvio) - ZONA_MORTA) / (512 - ZONA_MORTA);
  if (fracao > 1) fracao = 1;
  return (desvio > 0 ? 1 : -1) * fracao * VEL_MAX[j];
}

bool algumJoystickMexido() {
  for (uint8_t j = 0; j < 4; j++) if (velocidadeJoystick(j) != 0) return true;
  return false;
}

void medirCentros() {
  for (uint8_t j = 0; j < 4; j++) {
    long soma = 0;
    for (uint8_t k = 0; k < 32; k++) { soma += analogRead(PINO_EIXO[j]); delay(2); }
    centro[j] = soma / 32;
    if (abs(centro[j] - 512) > 150) {
      Serial.print(F("!! Joystick da junta "));
      Serial.print(NOME_JUNTA[j]);
      Serial.print(F(" fora do centro (leitura "));
      Serial.print(centro[j]);
      Serial.println(F("). Fio solto ou alguem mexeu ao ligar?"));
    }
  }
}

// ======================= ACOES =======================

void piscar(uint8_t vezes) {
  for (uint8_t k = 0; k < vezes; k++) {
    digitalWrite(PINO_LED, HIGH); delay(60);
    digitalWrite(PINO_LED, LOW);  delay(60);
  }
}

void gravarPosicao() {
  if (repetindo) return;
  if (numPosicoes >= MAX_POSICOES) {
    Serial.println(F("Memoria cheia (40 posicoes). Segure o joystick 1 por 2 s para apagar."));
    piscar(5);
    return;
  }
  for (uint8_t j = 0; j < 4; j++) posicoes[numPosicoes][j] = anguloEscrito[j];
  numPosicoes++;
  salvarPosicoes();
  Serial.print(F("Gravada a posicao "));
  Serial.println(numPosicoes);
  piscar(1);
}

void apagarPosicoes() {
  repetindo = false;
  numPosicoes = 0;
  salvarPosicoes();
  Serial.println(F("Todas as posicoes foram apagadas."));
  piscar(3);
}

void alternarRepeticao() {
  if (repetindo) {
    repetindo = false;
    Serial.println(F(">> Repeticao parada."));
    return;
  }
  if (numPosicoes < 2) {
    Serial.println(F("Grave pelo menos 2 posicoes (clique no joystick 1) antes de repetir."));
    piscar(2);
    return;
  }
  repetindo = true;
  indoParaInicio = false;
  alvoAtual = 0;
  esperando = false;
  Serial.println(F(">> Repetindo as posicoes gravadas..."));
}

void irParaInicio() {
  repetindo = false;
  indoParaInicio = true;
  Serial.println(F(">> Voltando para a posicao inicial."));
}

void listarPosicoes() {
  Serial.print(numPosicoes);
  Serial.println(F(" posicoes gravadas (base, ombro, cotovelo, garra):"));
  for (uint8_t p = 0; p < numPosicoes; p++) {
    Serial.print(F("  "));
    Serial.print(p + 1);
    Serial.print(F(": "));
    for (uint8_t j = 0; j < 4; j++) { Serial.print(posicoes[p][j]); Serial.print(j < 3 ? ' ' : '\n'); }
  }
}

void mostrarAjuda() {
  Serial.println(F("Joystick 1: clique = grava | segurar 2 s = apaga tudo"));
  Serial.println(F("Joystick 2: clique = repete/para | segurar 2 s = posicao inicial"));
  Serial.println(F("Monitor: g grava, r repete/para, l apaga, i inicial, p lista, ? ajuda"));
}

// Le um botao (INPUT_PULLUP: apertado = LOW). Devolve 1 = clique, 2 = aperto longo.
uint8_t lerBotao(Botao& b) {
  bool apertado = (digitalRead(b.pino) == LOW);
  unsigned long agora = millis();
  uint8_t evento = 0;
  if (apertado && !b.antes) { b.inicio = agora; b.longoFeito = false; }
  if (apertado && !b.longoFeito && agora - b.inicio >= APERTO_LONGO_MS) {
    b.longoFeito = true;
    evento = 2;
  }
  if (!apertado && b.antes && !b.longoFeito && agora - b.inicio >= 40) evento = 1;  // 40 ms ignora ruido
  b.antes = apertado;
  return evento;
}

void lerComandos() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'g' || c == 'G') gravarPosicao();
    else if (c == 'r' || c == 'R') alternarRepeticao();
    else if (c == 'l' || c == 'L') apagarPosicoes();
    else if (c == 'i' || c == 'I') irParaInicio();
    else if (c == 'p' || c == 'P') listarPosicoes();
    else if (c == '?') mostrarAjuda();
  }
}

// ======================= SETUP E LOOP =======================

void setup() {
  Serial.begin(115200);
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_BOTAO_GRAVAR, INPUT_PULLUP);
  pinMode(PINO_BOTAO_TOCAR, INPUT_PULLUP);
  Serial.println(F("\n=== Braco Robotico - joysticks + gravar e repetir ==="));
  Serial.println(F("Nao toque nos joysticks..."));
  delay(1000);
  medirCentros();

  // Liga os servos UM DE CADA VEZ, ja na posicao inicial (write antes de attach).
  for (uint8_t j = 0; j < 4; j++) {
    angulo[j] = constrain(ANG_INICIAL[j], ANG_MIN[j], ANG_MAX[j]);
    anguloEscrito[j] = (int)angulo[j];
    servo[j].write(anguloEscrito[j]);
    servo[j].attach(PINO_SERVO[j]);
    delay(300);
  }

  carregarPosicoes();
  Serial.print(numPosicoes);
  Serial.println(F(" posicoes gravadas na memoria."));
  mostrarAjuda();
  proximoCiclo = millis();
}

void loop() {
  lerComandos();
  uint8_t eg = lerBotao(botaoGravar);
  uint8_t et = lerBotao(botaoTocar);
  if (eg == 1) gravarPosicao();
  if (eg == 2) apagarPosicoes();
  if (et == 1) alternarRepeticao();
  if (et == 2) irParaInicio();

  if ((long)(millis() - proximoCiclo) < 0) return;
  proximoCiclo += PERIODO_MS;
  const float dt = PERIODO_MS / 1000.0;

  // Mexer um joystick sempre tem prioridade: para a repeticao (seguranca).
  if ((repetindo || indoParaInicio) && algumJoystickMexido()) {
    repetindo = false;
    indoParaInicio = false;
    Serial.println(F(">> Joystick mexido: parei."));
  }

  if (repetindo) {
    digitalWrite(PINO_LED, (millis() / 250) % 2);   // pisca = repetindo
    if (esperando) {
      if (millis() - inicioEspera >= PAUSA_POSICAO_MS) {
        esperando = false;
        alvoAtual = (alvoAtual + 1) % numPosicoes;
      }
    } else {
      bool chegou = true;
      for (uint8_t j = 0; j < 4; j++) {
        float alvo = constrain(posicoes[alvoAtual][j], ANG_MIN[j], ANG_MAX[j]);
        if (!andarAte(j, alvo, VEL_REPETIR * dt)) chegou = false;
      }
      if (chegou) { esperando = true; inicioEspera = millis(); }
    }
  } else if (indoParaInicio) {
    digitalWrite(PINO_LED, LOW);
    bool chegou = true;
    for (uint8_t j = 0; j < 4; j++) {
      float alvo = constrain(ANG_INICIAL[j], ANG_MIN[j], ANG_MAX[j]);
      if (!andarAte(j, alvo, VEL_REPETIR * dt)) chegou = false;
    }
    if (chegou) indoParaInicio = false;
  } else {
    digitalWrite(PINO_LED, LOW);
    for (uint8_t j = 0; j < 4; j++) {
      angulo[j] += velocidadeJoystick(j) * dt;
      angulo[j] = constrain(angulo[j], ANG_MIN[j], ANG_MAX[j]);
      escreverServo(j);
    }
  }

  // Mostra os angulos 4 vezes por segundo (use para achar ANG_MIN e ANG_MAX)
  if ((long)(millis() - proximoPrint) >= 0) {
    proximoPrint = millis() + 250;
    for (uint8_t j = 0; j < 4; j++) {
      Serial.print(NOME_JUNTA[j]);
      Serial.print(':');
      Serial.print(anguloEscrito[j]);
      Serial.print(' ');
    }
    Serial.print(F("| gravadas:"));
    Serial.print(numPosicoes);
    Serial.println(repetindo ? F(" | REPETINDO") : F(""));
  }
}
