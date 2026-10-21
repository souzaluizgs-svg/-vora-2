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

  Ao ligar (CALIBRACAO, so no modo SS49E):
    1) LED azul (GPIO2) piscando DEVAGAR: mao ABERTA (~3 s)
    2) LED piscando RAPIDO: punho FECHADO (~3 s)
    3) Depois o LED pisca curtinho de vez em quando = enviando.
    Para calibrar de novo, aperte o botao EN (reset) do ESP32.

  Placa na IDE: "ESP32 Dev Module" (ou "DOIT ESP32 DEVKIT V1").
  Monitor Serial: 115200.
  ==========================================================================
*/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

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

// ---------------- Pinos (ordem: polegar, indicador, medio, anelar, minimo)
const uint8_t PINO_SENSOR[5] = { 36, 34, 35, 32, 33 };
const char*   NOME_DEDO[5]   = { "polegar", "indicador", "medio", "anelar", "minimo" };
const uint8_t PINO_LED = 2;          // LED azul da placa

// ---------------- Ajustes ----------------
const uint8_t CANAL_WIFI = 1;        // os dois ESP32 precisam usar o MESMO canal
const unsigned long PERIODO_MS = 20; // 50 envios por segundo
const uint8_t LEITURAS_MEDIA = 8;    // quantas leituras do ADC para fazer a media
const float ALFA = 0.3;              // filtro: perto de 0 = mais suave, 1 = sem filtro
const unsigned long TEMPO_CALIBRACAO_MS = 3000; // use 2000 ou mais (o 1o segundo e ignorado)
const int   DIFERENCA_MINIMA = 120;  // em leituras do ADC (0..4095); nunca 0
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
int   valorFechado[5];
bool  sensorOk[5];
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

void calibrarSensores() {
  Serial.println("CALIBRACAO 1/2: deixe a mao ABERTA (LED piscando devagar)...");
  medirMedia(valorAberto, 400);
  Serial.println("CALIBRACAO 2/2: FECHE o punho (LED piscando rapido)...");
  medirMedia(valorFechado, 80);

  Serial.println("Resultado:");
  for (uint8_t i = 0; i < 5; i++) {
    int diferenca = abs(valorFechado[i] - valorAberto[i]);
    sensorOk[i] = (diferenca >= DIFERENCA_MINIMA && diferenca > 0);  // > 0: evita dividir por zero
    Serial.printf("  %-9s aberto=%4d fechado=%4d  %s\n", NOME_DEDO[i],
                  valorAberto[i], valorFechado[i],
                  sensorOk[i] ? "OK" : "POUCA DIFERENCA! Aproxime o ima. Dedo vai ficar parado.");
  }
}

// Leitura -> 0..100 %. Funciona com o ima de qualquer lado.
int paraPorcento(uint8_t i, int leitura) {
  if (!sensorOk[i]) return 0;
  long p = (long)(leitura - valorAberto[i]) * 100L / (valorFechado[i] - valorAberto[i]);
  return constrain(p, 0, 100);
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
  calibrarSensores();
#endif

  Serial.println("Pronto! Enviando...");
  proximoEnvio = millis();
}

void loop() {
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
    int bruto = paraPorcento(i, leituraCrua[i]);
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
}
