/*
  ==========================================================================
  MAO ROBOTICA - VERSAO 1 (COM FIO)  -  Arduino Uno
  ==========================================================================
  O que faz:
    Le 5 sensores Hall da luva (um por dedo). Cada sensor fica nas costas
    da mao, logo antes do no do dedo, e o ima fica na falange logo depois.
    Quando o dedo dobra, o ima gira e se afasta, e o dedo da mao robotica
    fecha na mesma proporcao. A calibracao funciona com o sinal subindo ou
    descendo, entao nao importa o lado do ima.

  Ligacoes:
    Sensores Hall (SS49E):  VCC -> 5V do Uno | GND -> GND | OUT -> pino abaixo
      polegar = A0, indicador = A1, medio = A2, anelar = A3, minimo = A4
    Servos SG90 (sinal):    polegar = D3, indicador = D5, medio = D6,
                            anelar = D9, minimo = D10
    Botao (OPCIONAL):       uma perna no D2, a outra no GND. Sem resistor.
    Alimentacao dos servos: pack 4 pilhas AA recarregaveis (4,8 V).
                            + do pack -> fio vermelho dos servos
                            - do pack -> fio marrom dos servos E GND do Uno
    O Uno fica ligado na USB do computador (ou num power bank).
    ATENCAO: os sensores usam o 5V do UNO (nao o pack). Nunca ligue o +
    do pack no pino 5V do Uno: so o GND (-) e compartilhado.

  CALIBRACAO (so no modo SS49E). Siga o LED do pino 13:
    1) piscando DEVAGAR : mao ABERTA, dedos esticados (~3 s)
    2) piscando MEDIO   : dedos dobrados SO NA BASE, ate a metade,
                          como a letra "C" ou segurando uma lata (~3 s)
    3) piscando RAPIDO  : PUNHO fechado (~3 s)
    4) apagado          : pronto!
    A calibracao fica GUARDADA na memoria (EEPROM): ao ligar de novo, o Uno
    usa a ultima. Para calibrar de novo: aperte o botao rapidinho, ou
    digite c no Monitor Serial, ou ligue o Uno com o botao apertado.
    Calibre de novo sempre que vestir a luva.

  MODO DEMONSTRACAO (mao faz gestos sozinha, sem a luva):
    segure o botao por 2 s, ou digite d no Monitor Serial. Repita para sair.
    Use para testar a mecanica e como plano B na apresentacao.

  Monitor Serial em 115200. Digite ? para ver os comandos.
  Serial Plotter: digite p para ver a curva de cada dedo (0 a 100 %).
  ==========================================================================
*/

#include <Servo.h>
#include <EEPROM.h>

// ---------------- Tipo de sensor ----------------
// 0 = SS49E (analogico, recomendado)   1 = A3144 (digital, liga/desliga)
// A3144: VCC -> 5V, GND -> GND, OUT -> A0..A4. Usa o pull-up interno.
// Ele so liga com o polo SUL do ima virado para a face com escrita.
#define USAR_A3144 0

// 1 = calibracao em 3 posicoes (aberta, meio, fechada): deixa o movimento
//     mais "reto", porque o sinal do ima muda mais rapido no comeco da dobra.
// 0 = so aberta e fechada (mais rapido de calibrar).
#define CALIBRAR_MEIO 1

// ---------------- Pinos (ordem: polegar, indicador, medio, anelar, minimo)
const uint8_t PINO_SENSOR[5] = { A0, A1, A2, A3, A4 };
const uint8_t PINO_SERVO[5]  = { 3, 5, 6, 9, 10 };
const char*   NOME_DEDO[5]   = { "polegar", "indicador", "medio", "anelar", "minimo" };
const uint8_t PINO_LED   = 13;
const uint8_t PINO_BOTAO = 2;   // opcional: botao entre D2 e GND

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
const unsigned long APERTO_LONGO_MS = 2000; // segurar o botao por 2 s = modo demonstracao
// O SS49E nunca chega perto de 0 V nem de 5 V. Leitura nas pontas = fio solto.
const int   LEITURA_MIN_VALIDA = 20;
const int   LEITURA_MAX_VALIDA = 1003;

// ---------------- Gestos do modo demonstracao ----------------
// Fechamento de cada dedo em % (polegar, indicador, medio, anelar, minimo)
// e quanto tempo o gesto fica parado. Pode inventar os seus!
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

// ---------------- Calibracao guardada na EEPROM ----------------
const uint16_t ASSINATURA = 0x4D32;   // "M2": muda se o formato mudar
struct Calibracao {
  uint16_t assinatura;
  int aberto[5];
  int meio[5];
  int fechado[5];
  uint8_t conferencia;                 // soma dos bytes, para saber se esta inteira
};

// ---------------- Variaveis ----------------
Servo servo[5];
int   angAtual[5];      // angulo que o servo esta agora
int   alvo[5];          // angulo para onde ele quer ir
int   leituraCrua[5];   // ultimo valor lido do sensor
int   valorAberto[5];   // leitura com a mao aberta (calibracao)
int   valorMeio[5];     // leitura com os dedos ate a metade (calibracao)
int   valorFechado[5];  // leitura com o punho fechado (calibracao)
bool  sensorOk[5];      // false = sensor quase nao mudou na calibracao
bool  meioOk[5];        // false = ponto do meio nao serve, usa so aberto/fechado
bool  fioSolto[5];      // leitura impossivel: fio solto ou sensor queimado
float pctFiltrado[5];   // fechamento filtrado, 0 a 100 %
bool  dedoFechado[5];   // so modo A3144
int   pctFinal[5];      // fechamento usado nos servos, 0 a 100 %

bool  modoDemo = false;
uint8_t gestoAtual = 0;
unsigned long inicioGesto = 0;
bool  modoPlotter = false;

bool  botaoAntes = false;
unsigned long inicioAperto = 0;
bool  apertoLongoFeito = false;

unsigned long proximoCiclo = 0;
unsigned long proximoPrint = 0;
unsigned long proximoAvisoFio = 0;

// ======================= SENSORES =======================

void atualizarServo(uint8_t i, int pct);   // fica la embaixo, na parte dos servos

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

bool leituraValida(int leitura) {
  return leitura >= LEITURA_MIN_VALIDA && leitura <= LEITURA_MAX_VALIDA;
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

// Confere cada dedo depois de calibrar (ou de carregar da EEPROM).
void conferirCalibracao(bool mostrar) {
  for (uint8_t i = 0; i < 5; i++) {
    int diferenca = abs(valorFechado[i] - valorAberto[i]);
    bool valida = leituraValida(valorAberto[i]) && leituraValida(valorFechado[i]);
    sensorOk[i] = valida && diferenca >= DIFERENCA_MINIMA && diferenca > 0;  // > 0: evita dividir por zero

    // O meio tem que ficar ENTRE aberto e fechado, com folga dos dois lados.
    long lado = (valorFechado[i] > valorAberto[i]) ? 1 : -1;
    long m = (long)(valorMeio[i] - valorAberto[i]) * lado;
    long f = (long)(valorFechado[i] - valorAberto[i]) * lado;
    long folga = f / 8;
    meioOk[i] = CALIBRAR_MEIO && sensorOk[i] && m > folga && m < f - folga;

    if (!mostrar) continue;
    Serial.print(F("  "));
    Serial.print(NOME_DEDO[i]);
    Serial.print(F(": aberto="));
    Serial.print(valorAberto[i]);
#if CALIBRAR_MEIO
    Serial.print(F(" meio="));
    Serial.print(valorMeio[i]);
#endif
    Serial.print(F(" fechado="));
    Serial.print(valorFechado[i]);
    if (!valida) {
      Serial.println(F("  FIO SOLTO? Leitura impossivel. Confira VCC, GND e OUT deste sensor."));
    } else if (!sensorOk[i]) {
      Serial.println(F("  POUCA DIFERENCA! Aproxime o ima do sensor. Este dedo vai ficar parado."));
    } else if (CALIBRAR_MEIO && !meioOk[i]) {
      Serial.println(F("  OK (o meio nao ficou entre aberto e fechado: usando so os dois)"));
    } else {
      Serial.println(F("  OK"));
    }
  }
}

uint8_t somaBytes(const Calibracao& c) {
  const uint8_t* p = (const uint8_t*)&c;
  uint8_t s = 0;
  for (uint8_t k = 0; k < offsetof(Calibracao, conferencia); k++) s += p[k];
  return s;
}

void salvarCalibracao() {
  Calibracao c;
  c.assinatura = ASSINATURA;
  for (uint8_t i = 0; i < 5; i++) {
    c.aberto[i] = valorAberto[i];
    c.meio[i] = valorMeio[i];
    c.fechado[i] = valorFechado[i];
  }
  c.conferencia = somaBytes(c);
  EEPROM.put(0, c);   // put so grava os bytes que mudaram (poupa a EEPROM)
}

bool carregarCalibracao() {
  Calibracao c;
  EEPROM.get(0, c);
  if (c.assinatura != ASSINATURA || c.conferencia != somaBytes(c)) return false;
  for (uint8_t i = 0; i < 5; i++) {
    valorAberto[i] = c.aberto[i];
    valorMeio[i] = c.meio[i];
    valorFechado[i] = c.fechado[i];
  }
  return true;
}

// Abre a mao devagar (respeitando PASSO_MAX) e espera chegar.
void abrirMao() {
  for (uint8_t t = 0; t < 40; t++) {
    for (uint8_t i = 0; i < 5; i++) atualizarServo(i, 0);
    delay(PERIODO_MS);
  }
  for (uint8_t i = 0; i < 5; i++) pctFinal[i] = 0;
}

void calibrarSensores() {
  // Enquanto calibra, a mao robotica fica aberta e parada.
  abrirMao();
#if CALIBRAR_MEIO
  Serial.println(F("CALIBRACAO 1/3: mao ABERTA, dedos esticados (LED devagar)..."));
  medirMedia(valorAberto, 400);
  Serial.println(F("CALIBRACAO 2/3: dobre so a BASE dos dedos ate a METADE, como um C (LED medio)..."));
  medirMedia(valorMeio, 180);
  Serial.println(F("CALIBRACAO 3/3: FECHE o punho (LED rapido)..."));
  medirMedia(valorFechado, 70);
#else
  Serial.println(F("CALIBRACAO 1/2: mao ABERTA, dedos esticados (LED devagar)..."));
  medirMedia(valorAberto, 400);
  Serial.println(F("CALIBRACAO 2/2: FECHE o punho (LED rapido)..."));
  medirMedia(valorFechado, 70);
  for (uint8_t i = 0; i < 5; i++) valorMeio[i] = (valorAberto[i] + valorFechado[i]) / 2;
#endif
  Serial.println(F("Resultado:"));
  conferirCalibracao(true);
  salvarCalibracao();
  Serial.println(F("Calibracao guardada na memoria."));
  for (uint8_t i = 0; i < 5; i++) pctFiltrado[i] = 0;
}

// Converte a leitura do sensor em fechamento de 0 a 100 %.
// Funciona com o ima de qualquer lado (a leitura pode subir ou descer).
// Com o ponto do meio, sao duas retas: aberto->meio = 0..50 % e
// meio->fechado = 50..100 %. Isso corrige a curva do ima.
int paraPorcento(uint8_t i, int leitura) {
  if (!sensorOk[i]) return 0;
  long lado = (valorFechado[i] > valorAberto[i]) ? 1 : -1;
  long v = (long)(leitura - valorAberto[i]) * lado;          // 0 = aberto
  long f = (long)(valorFechado[i] - valorAberto[i]) * lado;  // > 0
  long p;
  if (meioOk[i]) {
    long m = (long)(valorMeio[i] - valorAberto[i]) * lado;   // entre 0 e f
    if (v <= m) p = v * 50L / m;
    else        p = 50L + (v - m) * 50L / (f - m);
  } else {
    p = v * 100L / f;
  }
  return constrain(p, 0, 100);
}

// ======================= SERVOS =======================

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

// ======================= MODOS E COMANDOS =======================

void ligarDemo(bool ligar) {
  modoDemo = ligar;
  gestoAtual = 0;
  inicioGesto = millis();
  Serial.println(ligar ? F(">> MODO DEMONSTRACAO ligado (a luva e ignorada)")
                       : F(">> MODO DEMONSTRACAO desligado (voltou para a luva)"));
  for (uint8_t i = 0; i < 5; i++) pctFiltrado[i] = 0;
}

void mostrarAjuda() {
  Serial.println(F("Comandos (digite e aperte Enter):"));
  Serial.println(F("  c = calibrar de novo   d = liga/desliga demonstracao"));
  Serial.println(F("  p = liga/desliga modo Serial Plotter   ? = esta ajuda"));
  Serial.println(F("Botao no D2: aperto curto = calibrar | segurar 2 s = demonstracao"));
}

void recalibrar() {
#if USAR_A3144
  Serial.println(F("Modo A3144: nao precisa calibrar."));
#else
  if (modoDemo) ligarDemo(false);
  calibrarSensores();
  proximoCiclo = millis();
#endif
}

void lerComandos() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'c' || c == 'C') recalibrar();
    else if (c == 'd' || c == 'D') ligarDemo(!modoDemo);
    else if (c == 'p' || c == 'P') {
      modoPlotter = !modoPlotter;
      if (!modoPlotter) Serial.println(F(">> modo texto"));
    }
    else if (c == '?') mostrarAjuda();
  }
}

// Botao entre D2 e GND (INPUT_PULLUP: apertado = LOW).
// Sem botao ligado, o pino fica em HIGH e nada acontece.
void lerBotao() {
  bool apertado = (digitalRead(PINO_BOTAO) == LOW);
  unsigned long agora = millis();
  if (apertado && !botaoAntes) {
    inicioAperto = agora;
    apertoLongoFeito = false;
  }
  if (apertado && !apertoLongoFeito && agora - inicioAperto >= APERTO_LONGO_MS) {
    apertoLongoFeito = true;
    ligarDemo(!modoDemo);
  }
  if (!apertado && botaoAntes && !apertoLongoFeito && agora - inicioAperto >= 40) {
    recalibrar();   // aperto curto (40 ms ignora ruido do contato)
  }
  botaoAntes = apertado;
}

// ======================= SAIDA NO MONITOR =======================

void imprimirDepuracao() {
  if (modoPlotter) {
    // Formato do Serial Plotter da Arduino IDE 2: nome:valor,nome:valor
    for (uint8_t i = 0; i < 5; i++) {
      Serial.print(NOME_DEDO[i]);
      Serial.print(':');
      Serial.print(pctFinal[i]);
      Serial.print(i < 4 ? ',' : '\n');
    }
    return;
  }
  Serial.print(modoDemo ? F("DEMO ") : F("cru:"));
  if (!modoDemo) {
    for (uint8_t i = 0; i < 5; i++) { Serial.print(' '); Serial.print(leituraCrua[i]); }
  }
  Serial.print(F(" | %:"));
  for (uint8_t i = 0; i < 5; i++) { Serial.print(' '); Serial.print(pctFinal[i]); }
  Serial.print(F(" | ang:"));
  for (uint8_t i = 0; i < 5; i++) { Serial.print(' '); Serial.print(angAtual[i]); }
  Serial.println();
}

// ======================= SETUP E LOOP =======================

void setup() {
  Serial.begin(115200);
  pinMode(PINO_LED, OUTPUT);
  pinMode(PINO_BOTAO, INPUT_PULLUP);
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
    fioSolto[i] = false;
    delay(300);
  }

#if USAR_A3144
  for (uint8_t i = 0; i < 5; i++) pinMode(PINO_SENSOR[i], INPUT_PULLUP);
  Serial.println(F("Sensor: A3144 (digital). Sem calibracao."));
#else
  Serial.println(F("Sensor: SS49E (analogico)."));
  bool botaoNoBoot = (digitalRead(PINO_BOTAO) == LOW);
  if (!botaoNoBoot && carregarCalibracao()) {
    Serial.println(F("Usando a calibracao guardada:"));
    conferirCalibracao(true);
    Serial.println(F("(Vestiu a luva de novo? Aperte o botao ou digite c para calibrar.)"));
  } else {
    calibrarSensores();
  }
  botaoAntes = (digitalRead(PINO_BOTAO) == LOW);
  apertoLongoFeito = true;   // o aperto usado no boot nao conta como comando
#endif

  mostrarAjuda();
  Serial.println(F("Pronto! Mexa os dedos da luva."));
  proximoCiclo = millis();
}

void loop() {
  lerComandos();
  lerBotao();

  // Tudo acontece a cada PERIODO_MS (20 ms = 50 vezes por segundo)
  if ((long)(millis() - proximoCiclo) < 0) return;
  proximoCiclo += PERIODO_MS;

  if (modoDemo) {
    if (millis() - inicioGesto >= GESTOS[gestoAtual].ms) {
      gestoAtual = (gestoAtual + 1) % NUM_GESTOS;
      inicioGesto = millis();
    }
    for (uint8_t i = 0; i < 5; i++) {
      pctFinal[i] = GESTOS[gestoAtual].pct[i];
      atualizarServo(i, pctFinal[i]);
    }
  } else {
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
      // Fio solto: o dedo abre (mais seguro) e aparece um aviso.
      fioSolto[i] = !leituraValida(leituraCrua[i]);
      int bruto = fioSolto[i] ? 0 : paraPorcento(i, leituraCrua[i]);
      // Analogico: suaviza (media movel exponencial)
      pctFiltrado[i] += ALFA * (bruto - pctFiltrado[i]);
      pctFinal[i] = (int)(pctFiltrado[i] + 0.5);
#endif

      atualizarServo(i, pctFinal[i]);
    }
  }

  // Mostra os numeros 5 vezes por segundo (20 no modo Plotter)
  if ((long)(millis() - proximoPrint) >= 0) {
    proximoPrint = millis() + (modoPlotter ? 50 : 200);
    imprimirDepuracao();
  }

#if !USAR_A3144
  // Aviso de fio solto a cada 2 s (fora do modo Plotter, para nao sujar o grafico)
  if (!modoDemo && !modoPlotter && (long)(millis() - proximoAvisoFio) >= 0) {
    proximoAvisoFio = millis() + 2000;
    for (uint8_t i = 0; i < 5; i++) {
      if (fioSolto[i]) {
        Serial.print(F("!! FIO SOLTO? sensor do "));
        Serial.println(NOME_DEDO[i]);
      }
    }
  }
#endif
}
