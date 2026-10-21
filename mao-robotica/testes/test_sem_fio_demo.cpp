#include "sem_fio_host.cpp"
#include <cassert>
static void roda(int ms) { for (int t = 0; t < ms; t++) { loop(); g_millis++; } }
int main() {
  setup();
  roda(1000);                       // sem sinal: mao aberta
  assert(angAtual[0] == ANG_ABERTO[0]);
  g_botao = 0; roda(2100); g_botao = 1; roda(10);
  assert(modoDemo);
  roda(1500 + 900);                 // gesto 1 = punho
  assert(gestoAtual == 1 && angAtual[0] == ANG_FECHADO[0] && angAtual[4] == ANG_FECHADO[4]);
  // dados chegando durante a demo sao ignorados
  uint8_t q[7] = {0xFF, 0, 0, 0, 0, 0, 0}; for (int k = 0; k < 7; k++) Serial.rx.push_back(q[k]);
  roda(100); assert(angAtual[0] == ANG_FECHADO[0]);
  g_botao = 0; roda(2100); g_botao = 1; roda(10);
  assert(!modoDemo);
  roda(1000); assert(angAtual[0] == ANG_ABERTO[0]);   // sem sinal valido recente: abre
  printf("TESTES DA DEMONSTRACAO do mao_sem_fio PASSARAM\n");
}
