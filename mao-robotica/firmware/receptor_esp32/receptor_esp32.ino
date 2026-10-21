/*
  ==========================================================================
  MAO ROBOTICA - VERSAO 2 (SEM FIO) - RECEPTOR  -  ESP32 DevKit V1
  ==========================================================================
  O que faz:
    Fica junto da mao robotica. Recebe pelo ar (ESP-NOW) os pacotes da
    luva e repassa para o Arduino Uno por um fio (serial), 50 vezes por
    segundo. Quem mexe os servos continua sendo o Uno.

  Ligacoes:
    ESP32 TX2 (GPIO17)  ->  pino D0 (RX) do Uno (fio direto, SEM resistor
                            e SEM divisor de tensao)
    ESP32 GND           ->  GND do Uno          (OBRIGATORIO!)
    Alimente este ESP32 pela USB (ou 5 V no pino VIN, mas nunca USB e VIN
    ao mesmo tempo). NAO alimente pelo pack de pilhas dos servos: quando os
    servos puxam corrente a tensao cai e o ESP32 reinicia.
    So existe um fio de dados: o ESP32 fala e o Uno so escuta.
    (3,3 V do ESP32 e suficiente para o Uno entender como "1".)
    NUNCA ligue o D1 (TX) do Uno aqui: 5 V queimam o ESP32.

  Este codigo e para o ESP32 "classico" (DevKit V1, modulo ESP-WROOM-32).
  ESP32-S3/C3/C6 tem outros pinos. Em placas WROVER (com PSRAM) o GPIO16 e
  o GPIO17 sao usados pela memoria: troque PINO_TX_UNO para 25, por exemplo.

  Quadro enviado ao Uno (7 bytes):
    0xFF, p0, p1, p2, p3, p4, xor(p0..p4)
    p0..p4 = fechamento 0..100 % (polegar, indicador, medio, anelar, minimo)
    Como p sempre e menor que 128, o 0xFF so aparece no comeco do quadro.

  LED azul (GPIO2): aceso = recebendo da luva | apagado = sem sinal.
  Se a luva parar de mandar por 300 ms, este ESP32 para de mandar para o
  Uno, e o Uno abre a mao sozinho (seguranca).

  Placa na IDE: "ESP32 Dev Module" (ou "DOIT ESP32 DEVKIT V1").
  ==========================================================================
*/

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_arduino_version.h>

const uint8_t CANAL_WIFI = 1;          // MESMO canal da luva
const uint8_t PINO_LED = 2;
const int     PINO_TX_UNO = 17;        // TX2 -> D0 do Uno
const int     PINO_RX_NAO_USADO = 16;  // RX2 (nao precisa ligar nada)
const unsigned long PERIODO_MS = 20;   // 50 quadros por segundo
const unsigned long TEMPO_SEM_SINAL_MS = 300;

// ---------------- Pacote recebido (IGUAL na luva!) ----------------
// Se houver OUTRA mao robotica com este mesmo programa por perto (feira de
// ciencias), uma luva vai mexer a mao da outra! Para evitar, troque a letra
// de MAGIC_2 (ex.: 'X') AQUI e na luva_esp32 (e, se quiser, o canal).
const uint8_t MAGIC_1 = 'M';
const uint8_t MAGIC_2 = 'R';
typedef struct __attribute__((packed)) {
  uint8_t magic[2];   // 'M','R'
  uint8_t seq;
  uint8_t pct[5];
} PacoteMao;

// Dados compartilhados entre a funcao de recepcao (roda na tarefa do
// Wi-Fi) e o loop(). A "trava" (spinlock) impede ler no meio de uma copia.
portMUX_TYPE trava = portMUX_INITIALIZER_UNLOCKED;
PacoteMao ultimoPacote;
unsigned long momentoUltimoPacote = 0;
bool jaRecebeu = false;
unsigned long pacotesRecebidos = 0;

unsigned long proximoEnvio = 0;
unsigned long proximoPrint = 0;

// Chamada automaticamente quando chega um pacote ESP-NOW.
// Tem que ser RAPIDA: so confere e copia. Nada de delay ou Serial aqui.
// O formato mudou no core 3.x do ESP32, por isso o #if.
#if ESP_ARDUINO_VERSION_MAJOR >= 3
void aoReceber(const esp_now_recv_info_t* info, const uint8_t* dados, int tamanho) {
#else
void aoReceber(const uint8_t* mac, const uint8_t* dados, int tamanho) {
#endif
  if (tamanho != sizeof(PacoteMao)) return;          // tamanho errado: ignora
  if (dados[0] != MAGIC_1 || dados[1] != MAGIC_2) return; // nao e da nossa luva
  portENTER_CRITICAL(&trava);
  memcpy(&ultimoPacote, dados, sizeof(PacoteMao));
  momentoUltimoPacote = millis();
  jaRecebeu = true;
  pacotesRecebidos++;
  portEXIT_CRITICAL(&trava);
}

void setup() {
  Serial.begin(115200);                                          // USB (depuracao)
  // Para o Uno. Deixe os pinos escritos aqui: no core 3.x o Serial2 sem
  // pinos usa outros GPIO. 115200 tem que ser IGUAL ao Serial.begin do Uno.
  Serial2.begin(115200, SERIAL_8N1, PINO_RX_NAO_USADO, PINO_TX_UNO);
  pinMode(PINO_LED, OUTPUT);
  delay(300);
  Serial.println("\n=== Mao Robotica - Receptor (ESP32) ===");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_wifi_set_channel(CANAL_WIFI, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    Serial.println("ERRO: nao consegui mudar o canal do Wi-Fi!");
  }
  uint8_t canal = 0;
  wifi_second_chan_t canal2;
  esp_wifi_get_channel(&canal, &canal2);
  Serial.printf("Canal Wi-Fi: %u (tem que ser igual ao da luva)\n", canal);

  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  Serial.printf("MAC deste receptor: %02X:%02X:%02X:%02X:%02X:%02X\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERRO: nao consegui iniciar o ESP-NOW. Reiniciando...");
    delay(2000);
    ESP.restart();
  }
  esp_now_register_recv_cb(aoReceber);

  Serial.println("Esperando a luva...");
  proximoEnvio = millis();
}

void loop() {
  if ((long)(millis() - proximoEnvio) < 0) return;
  proximoEnvio += PERIODO_MS;

  // Copia os dados com a trava fechada (rapidinho)
  PacoteMao p;
  unsigned long momento;
  bool recebeu;
  portENTER_CRITICAL(&trava);
  p = ultimoPacote;
  momento = momentoUltimoPacote;
  recebeu = jaRecebeu;
  unsigned long total = pacotesRecebidos;
  portEXIT_CRITICAL(&trava);

  bool linkOk = recebeu && (millis() - momento < TEMPO_SEM_SINAL_MS);
  digitalWrite(PINO_LED, linkOk ? HIGH : LOW);

  if (linkOk) {
    uint8_t quadro[7];
    uint8_t x = 0;
    quadro[0] = 0xFF;                      // cabecalho
    for (uint8_t i = 0; i < 5; i++) {
      uint8_t v = p.pct[i] > 100 ? 100 : p.pct[i];
      quadro[1 + i] = v;
      x ^= v;                              // "ou exclusivo" para conferir
    }
    quadro[6] = x;
    Serial2.write(quadro, sizeof(quadro));
  }
  // Sem sinal: nao manda nada. O Uno percebe e abre a mao.

  if ((long)(millis() - proximoPrint) >= 0) {
    proximoPrint = millis() + 500;
    if (linkOk) {
      Serial.printf("OK  seq %3u | %%: %3u %3u %3u %3u %3u | pacotes %lu\n",
                    p.seq, p.pct[0], p.pct[1], p.pct[2], p.pct[3], p.pct[4], total);
    } else {
      Serial.println("SEM SINAL da luva (nao estou mandando nada ao Uno)");
    }
  }
}
