#include "sem_fio_host.cpp"
#include <cassert>
static void sendFrame(const int p[5], bool corrupt = false) {
  uint8_t x = 0; Serial.rx.push_back(0xFF);
  for (int i = 0; i < 5; i++) { Serial.rx.push_back(p[i]); x ^= p[i]; }
  Serial.rx.push_back(corrupt ? (x ^ 1) : x);
}
static void runMs(int ms, const int* p = nullptr, bool corrupt = false) {
  for (int t = 0; t < ms; t++) {
    if (p && (g_millis % 20) == 0) sendFrame(p, corrupt);
    loop(); g_millis++;
  }
}
int main() {
  setup();
  for (int i = 0; i < 5; i++) assert(servo[i].val == 20 && servo[i].pin == PINO_SERVO[i]);
  int p1[5] = {0, 50, 100, 25, 75};
  runMs(1000, p1);
  int esperado[5] = {20, 90, 160, 55, 125};
  for (int i = 0; i < 5; i++) { printf("dedo %d pct=%d ang=%d\n", i, pctRecebido[i], angAtual[i]); assert(pctRecebido[i] == p1[i]); assert(angAtual[i] == esperado[i]); }
  assert(g_led == 1);
  unsigned long bons = quadrosBons; printf("bons=%lu ruins=%lu\n", quadrosBons, quadrosRuins); assert(quadrosRuins == 0);
  // quadros corrompidos (xor errado) nao podem mudar nada
  int p2[5] = {100, 100, 100, 100, 100};
  runMs(200, p2, true);
  for (int i = 0; i < 5; i++) assert(pctRecebido[i] == p1[i]);
  assert(quadrosBons == bons && quadrosRuins >= 9);
  // lixo + valor > 100 + quadro pela metade
  uint8_t lixo[] = {1, 2, 3, 0xFF, 10, 20, 200, 5, 0xFF, 1, 2, 0xFF};
  for (uint8_t b : lixo) Serial.rx.push_back(b);
  // depois um quadro bom chegando byte a byte
  int p3[5] = {10, 20, 30, 40, 50}; sendFrame(p3);
  while (!Serial.rx.empty()) { loop(); g_millis++; }
  for (int i = 0; i < 5; i++) assert(pctRecebido[i] == p3[i]);
  printf("depois do lixo: bons=%lu ruins=%lu\n", quadrosBons, quadrosRuins);
  // failsafe: para de mandar -> em ate ~500 ms + movimento, abre
  unsigned long t0 = g_millis; runMs(450); assert(g_led == 1);
  runMs(100); assert(g_led == 0);
  runMs(1000);
  for (int i = 0; i < 5; i++) { assert(angAtual[i] == ANG_ABERTO[i]); }
  printf("failsafe OK (t=%lu ms apos ultimo quadro)\n", g_millis - t0);
  // velocidade: de aberto a fechado, nunca mais que PASSO_MAX por ciclo
  int pf[5] = {100, 100, 100, 100, 100}; int antes = angAtual[0]; int maxPasso = 0;
  for (int t = 0; t < 1000; t++) { if (g_millis % 20 == 0) sendFrame(pf); loop(); g_millis++; int d = abs(angAtual[0] - antes); if (d > maxPasso) maxPasso = d; antes = angAtual[0]; }
  printf("maior passo por ciclo = %d (PASSO_MAX=%d), final=%d\n", maxPasso, PASSO_MAX, angAtual[0]);
  assert(maxPasso <= PASSO_MAX && angAtual[0] == 160);
  printf("TODOS OS TESTES mao_sem_fio PASSARAM\n");
}
