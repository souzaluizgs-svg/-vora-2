/*
  ==========================================================================
  MAO ROBOTICA - VERSAO 2 (SEM FIO) - LUVA  -  ESP32 DevKit V1 (transmissor)
  ==========================================================================
  O que faz:
    Le os 5 sensores Hall da luva, calcula quanto cada dedo esta fechado
    (0 a 100 %) e manda pelo ar (ESP-NOW) cerca de 50 vezes por segundo
    para o outro ESP32, que fica junto da mao robotica.

  Ligacoes (SS49E):  VCC -> 3V3 do ESP32 | GND -> GND | OUT -> pino abaixo
      polegar = GPIO36 (no DevKit aparece como "VP"), indicador = GPIO34,
      medio   = GPIO35, anelar = GPIO32, minimo = GPIO33
    (nessa ordem eles ficam em sequencia na fileira de pinos: VP, D34, D35,
     D32, D33; o VN, entre VP e D34, fica sem uso)
    Esses pinos sao do ADC1, que funciona junto com o Wi-Fi.
    (Os pinos do ADC2 NAO funcionam enquanto o Wi-Fi/ESP-NOW esta ligado.)
    !!! Na luva o SS49E vai no 3V3, NUNCA no 5V/VIN: com 5 V a saida dele
        passa de 3,3 V e pode queimar o pino do ESP32. Pelo mesmo motivo,
        NAO use modulos KY-024/KY-003 alimentados com 5 V aqui. !!!

  Este codigo e para o ESP32 "classico" (DevKit V1, modulo ESP-WROOM-32).
  ESP32-S3/C3/C6 tem outros numeros de pino.

  CALIBRACAO (so no modo SS49E). Siga o LED azul (GPIO2):
    1) piscando DEVAGAR : mao ABERTA, dedos esticados (~3 s)
    2) piscando MEDIO   : dedos dobrados SO NA BASE, ate a metade, como um C
    3) piscando RAPIDO  : PUNHO fechado (~3 s)
    4) depois pisca curtinho de vez em quando = enviando.
    A calibracao fica GUARDADA na memoria do ESP32: ao ligar de novo, ele
    usa a ultima. Para calibrar de novo, aperte o botao BOOT da placa
    (rapidinho, com o programa ja rodando) ou digite c no Monitor Serial.
    Calibre de novo sempre que vestir a luva.
    (Nao segure o BOOT enquanto liga a placa: isso entra no modo gravacao.)

  Placa na IDE: "ESP32 Dev Module" (ou "DOIT ESP32 DEVKIT V1").
  Monitor Serial: 115200.
  ==========================================================================
*/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>

// ---------------- Tipo de sensor ----------------
// 0 = SS49E (analogico, recomendado)   1 = A3144 (digital)
// ATENCAO A3144: precisa de VCC >= 4,5 V -> ligue o VCC dele no pino VIN
// (5 V da USB). So funciona com o ESP32 alimentado pela USB/power bank;
// com bateria ligada no 3V3 o VIN fica sem 5 V e o A3144 nao liga.
// A saida dele e "coletor aberto": coloque um resistor de
// 10 k entre cada saida e o 3V3. NAO ligue o resistor no 5 V!
// (Os GPIO34, 35 e 36 nao tem pull-up interno, por isso o resistor.)
// Use o A3144 avulso: o modulo KY-003 ja tem resistor para o 5 V.
#define USAR_A3144 0

// 1 = calibracao em 3 posicoes (aberta, meio, fechada): movimento mais "reto".
// 0 = so aberta e fechada.
#define CALIBRAR_MEIO 1

// ---------------- Pinos (ordem: polegar, indicador, medio, anelar, minimo)
const uint8_t PINO_SENSOR[5] = { 36, 34, 35, 32, 33 };
const char*   NOME_DEDO[5]   = { "polegar", "indicador", "medio", "anelar", "minimo" };
const uint8_t PINO_LED = 2;          // LED azul da placa
const uint8_t PINO_BOTAO = 0;        // botao BOOT da placa (ja tem pull-up)

// ---------------- Ajustes ----------------
const uint8_t CANAL_WIFI = 1;        // os dois ESP32 precisam usar o MESMO canal
const unsigned long PERIODO_MS = 20; // 50 envios por segundo
const uint8_t LEITURAS_MEDIA = 8;    // quantas leituras do ADC para fazer a media
const float ALFA = 0.3;              // filtro: perto de 0 = mais suave, 1 = sem filtro
const unsigned long TEMPO_CALIBRACAO_MS = 3000; // use 2000 ou mais (o 1o segundo e ignorado)
const int   DIFERENCA_MINIMA = 120;  // em leituras do ADC (0..4095); nunca 0
// Com 3,3 V o SS49E fica entre ~0,9 e ~2,4 V. Leitura nas pontas = fio solto
// (ou um clone de 49E que nao funciona em 3,3 V).
const int   LEITURA_MIN_VALIDA = 60;
const int   LEITURA_MAX_VALIDA = 4030;
const float LIMIAR_FECHAR = 70;      // so A3144 (histerese)
const float LIMIAR_ABRIR  = 30;

// ---------------- Pacote enviado (IGUAL no receptor!) ----------------
// Outra mao robotica com este programa por perto? Troque MAGIC_2 (ex.: 'X')
// aqui E no receptor_esp32, senao uma luva mexe a mao da outra.
const uint8_t MAGIC_1 = 'M';
const uint8_t MAGIC_2 = 'R';
typedef struct __attribute__((packed)) {
  uint8_t magic[2];   // 'M','R' = "Mao Robotica" (para reconhecer o pacote)
  uint8_t seq;        // numero do pacote (vai de 0 a 255 e volta)
  uint8_t pct[5];     // fechamento de cada dedo, 0..100 %
} PacoteMao;

uint8_t ENDERECO_TODOS[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; // broadcast

PacoteMao pacote;
int   leituraCrua[5];
int   valorAberto[5];
int   valorMeio[5];
int   valorFechado[5];
bool  sensorOk[5];
bool  meioOk[5];
bool  fioSolto[5];
Preferences memoria;
bool  botaoAntes = false;
unsigned long inicioAperto = 0;
unsigned long proximoAvisoFio = 0;
float pctFiltrado[5];
bool  dedoFechado[5];
unsigned long proximoEnvio = 0;
unsigned long proximoPrint = 0;
unsigned long errosEnvio = 0;

int lerSensor(uint8_t i) {
#if USAR_A3144
  return (digitalRead(PINO_SENSOR[i]) == LOW) ? 1 : 0;   // LOW = ima perto
#else
  long soma = 0;
  for (uint8_t k = 0; k < LEITURAS_MEDIA; k++) soma += analogRead(PINO_SENSOR[i]);
  return soma / LEITURAS_MEDIA;                          // 0..4095
#endif
}

// Media dos sensores durante TEMPO_CALIBRACAO_MS, piscando o LED.
// O primeiro 1 segundo e ignorado (tempo para posicionar a mao).
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
  if (n == 0) n = 1;  // protecao: TEMPO_CALIBRACAO_MS curto demais (no ESP32, dividir por 0 trava a placa)
  for (uint8_t i = 0; i < 5; i++) resultado[i] = soma[i] / n;
  digitalWrite(PINO_LED, LOW);
}

bool leituraValida(int leitura) {
  return leitura >= LEITURA_MIN_VALIDA && leitura <= LEITURA_MAX_VALIDA;
}

// Confere cada dedo depois de calibrar (ou de carregar da memoria).
void conferirCalibracao() {
  for (uint8_t i = 0; i < 5; i++) {
    int diferenca = abs(valorFechado[i] - valorAberto[i]);
    bool valida = leituraValida(valorAberto[i]) && leituraValida(valorFechado[i]);
    sensorOk[i] = valida && diferenca >= DIFERENCA_MINIMA && diferenca > 0;  // > 0: evita dividir por zero
    long lado = (valorFechado[i] > valorAberto[i]) ? 1 : -1;
    long m = (long)(valorMeio[i] - valorAberto[i]) * lado;
    long f = (long)(valorFechado[i] - valorAberto[i]) * lado;
    meioOk[i] = CALIBRAR_MEIO && sensorOk[i] && m > f / 8 && m < f - f / 8;
    const char* situacao = !valida ? "FIO SOLTO? (ou 49E que nao funciona em 3,3 V)"
                         : !sensorOk[i] ? "POUCA DIFERENCA! Aproxime o ima. Dedo vai ficar parado."
                         : (CALIBRAR_MEIO && !meioOk[i]) ? "OK (meio fora da faixa: usando so aberto/fechado)"
                         : "OK";
    Serial.printf("  %-9s aberto=%4d meio=%4d fechado=%4d  %s\n", NOME_DEDO[i],
                  valorAberto[i], valorMeio[i], valorFechado[i], situacao);
  }
}

void salvarCalibracao() {
  int dados[15];
  for (uint8_t i = 0; i < 5; i++) { dados[i] = valorAberto[i]; dados[5 + i] = valorMeio[i]; dados[10 + i] = valorFechado[i]; }
  memoria.putBytes("cal", dados, sizeof(dados));
}

bool carregarCalibracao() {
  int dados[15];
  if (memoria.getBytes("cal", dados, sizeof(dados)) != sizeof(dados)) return false;
  for (uint8_t i = 0; i < 5; i++) { valorAberto[i] = dados[i]; valorMeio[i] = dados[5 + i]; valorFechado[i] = dados[10 + i]; }
  return true;
}

void calibrarSensores() {
  // Durante a calibracao nada e enviado: o Uno abre a mao (failsafe).
#if CALIBRAR_MEIO
  Serial.println("CALIBRACAO 1/3: mao ABERTA, dedos esticados (LED devagar)...");
  medirMedia(valorAberto, 400);
  Serial.println("CALIBRACAO 2/3: dobre so a BASE dos dedos ate a METADE, como um C (LED medio)...");
  medirMedia(valorMeio, 180);
  Serial.println("CALIBRACAO 3/3: FECHE o punho (LED rapido)...");
  medirMedia(valorFechado, 70);
#else
  Serial.println("CALIBRACAO 1/2: mao ABERTA, dedos esticados (LED devagar)...");
  medirMedia(valorAberto, 400);
  Serial.println("CALIBRACAO 2/2: FECHE o punho (LED rapido)...");
  medirMedia(valorFechado, 70);
  for (uint8_t i = 0; i < 5; i++) valorMeio[i] = (valorAberto[i] + valorFechado[i]) / 2;
#endif
  Serial.println("Resultado:");
  conferirCalibracao();
  salvarCalibracao();
  Serial.println("Calibracao guardada na memoria.");
  for (uint8_t i = 0; i < 5; i++) pctFiltrado[i] = 0;
}

// Leitura -> 0..100 %. Funciona com o ima de qualquer lado.
// Com o ponto do meio sao duas retas (0..50 % e 50..100 %), que corrigem a curva do ima.
int paraPorcento(uint8_t i, int leitura) {
  if (!sensorOk[i]) return 0;
  long lado = (valorFechado[i] > valorAberto[i]) ? 1 : -1;
  long v = (long)(leitura - valorAberto[i]) * lado;
  long f = (long)(valorFechado[i] - valorAberto[i]) * lado;
  long p;
  if (meioOk[i]) {
    long m = (long)(valorMeio[i] - valorAberto[i]) * lado;
    if (v <= m) p = v * 50L / m;
    else        p = 50L + (v - m) * 50L / (f - m);
  } else {
    p = v * 100L / f;
  }
  return constrain(p, 0, 100);
}

// Botao BOOT (GPIO0, apertado = LOW) ou letra c no Monitor: calibra de novo.
bool pediuCalibracao() {
  bool pediu = false;
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'c' || c == 'C') pediu = true;
  }
  bool apertado = (digitalRead(PINO_BOTAO) == LOW);
  if (apertado && !botaoAntes) inicioAperto = millis();
  if (!apertado && botaoAntes && millis() - inicioAperto >= 40) pediu = true;  // 40 ms ignora ruido
  botaoAntes = apertado;
  return pediu;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  pinMode(PINO_LED, OUTPUT);
  Serial.println("\n=== Mao Robotica - Luva (ESP32 transmissor) ===");

  // ---- Liga o Wi-Fi so para o ESP-NOW (nao conecta em roteador) ----
  // Fica ANTES da calibracao: com o radio ligado o consumo muda e as
  // leituras do ADC mudam um pouco; calibrar nas mesmas condicoes do uso.
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_wifi_set_channel(CANAL_WIFI, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    Serial.println("ERRO: nao consegui mudar o canal do Wi-Fi!");
  }
  uint8_t canal = 0;
  wifi_second_chan_t canal2;
  esp_wifi_get_channel(&canal, &canal2);
  Serial.printf("Canal Wi-Fi: %u (tem que ser igual ao do receptor)\n", canal);

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  Serial.printf("MAC desta luva: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERRO: nao consegui iniciar o ESP-NOW. Reiniciando...");
    delay(2000);
    ESP.restart();
  }

  // Cadastra o endereco "broadcast" (todo mundo) como destino
  esp_now_peer_info_t destino = {};
  memcpy(destino.peer_addr, ENDERECO_TODOS, 6);
  destino.channel = 0;              // 0 = usa o canal atual
  destino.ifidx = WIFI_IF_STA;
  destino.encrypt = false;
  if (esp_now_add_peer(&destino) != ESP_OK) {
    Serial.println("ERRO: nao consegui adicionar o destino broadcast.");
  }

  pacote.magic[0] = MAGIC_1;
  pacote.magic[1] = MAGIC_2;
  pacote.seq = 0;
  for (uint8_t i = 0; i < 5; i++) {
    pacote.pct[i] = 0;
    pctFiltrado[i] = 0;
    dedoFechado[i] = false;
  }

#if USAR_A3144
  for (uint8_t i = 0; i < 5; i++) pinMode(PINO_SENSOR[i], INPUT);  // pull-up externo 10k no 3V3
  Serial.println("Sensor: A3144 (digital). Sem calibracao.");
#else
  analogReadResolution(12);           // 0..4095
  analogSetAttenuation(ADC_11db);     // le de 0 ate ~3,1 V
  Serial.println("Sensor: SS49E (analogico).");
  memoria.begin("mao", false);
  if (carregarCalibracao()) {
    Serial.println("Usando a calibracao guardada:");
    conferirCalibracao();
    Serial.println("(Vestiu a luva de novo? Aperte BOOT ou digite c para calibrar.)");
  } else {
    calibrarSensores();
  }
#endif
  pinMode(PINO_BOTAO, INPUT_PULLUP);

  Serial.println("Pronto! Enviando...");
  proximoEnvio = millis();
}

void loop() {
#if !USAR_A3144
  if (pediuCalibracao()) {
    calibrarSensores();
    proximoEnvio = millis();
  }
#endif
  if ((long)(millis() - proximoEnvio) < 0) return;
  proximoEnvio += PERIODO_MS;

  for (uint8_t i = 0; i < 5; i++) {
    leituraCrua[i] = lerSensor(i);
#if USAR_A3144
    float bruto = leituraCrua[i] ? 100.0 : 0.0;
    pctFiltrado[i] += ALFA * (bruto - pctFiltrado[i]);
    if (!dedoFechado[i] && pctFiltrado[i] > LIMIAR_FECHAR) dedoFechado[i] = true;
    if (dedoFechado[i] && pctFiltrado[i] < LIMIAR_ABRIR)   dedoFechado[i] = false;
    pacote.pct[i] = dedoFechado[i] ? 100 : 0;
#else
    fioSolto[i] = !leituraValida(leituraCrua[i]);
    int bruto = fioSolto[i] ? 0 : paraPorcento(i, leituraCrua[i]);   // fio solto: dedo abre
    pctFiltrado[i] += ALFA * (bruto - pctFiltrado[i]);
    pacote.pct[i] = (uint8_t)(pctFiltrado[i] + 0.5);
#endif
  }

  pacote.seq++;
  esp_err_t r = esp_now_send(ENDERECO_TODOS, (uint8_t*)&pacote, sizeof(pacote));
  if (r != ESP_OK) errosEnvio++;

  // LED: pisca curtinho a cada 16 pacotes (~3 vezes por segundo) = transmitindo
  digitalWrite(PINO_LED, (pacote.seq & 0x0F) == 0);

  // Depuracao 5 vezes por segundo
  if ((long)(millis() - proximoPrint) >= 0) {
    proximoPrint = millis() + 200;
    Serial.printf("cru: %4d %4d %4d %4d %4d | %%: %3d %3d %3d %3d %3d | seq %3u | erros %lu\n",
                  leituraCrua[0], leituraCrua[1], leituraCrua[2], leituraCrua[3], leituraCrua[4],
                  pacote.pct[0], pacote.pct[1], pacote.pct[2], pacote.pct[3], pacote.pct[4],
                  pacote.seq, errosEnvio);
  }
#if !USAR_A3144
  if ((long)(millis() - proximoAvisoFio) >= 0) {
    proximoAvisoFio = millis() + 2000;
    for (uint8_t i = 0; i < 5; i++) {
      if (fioSolto[i]) Serial.printf("!! FIO SOLTO? sensor do %s\n", NOME_DEDO[i]);
    }
  }
#endif
}
