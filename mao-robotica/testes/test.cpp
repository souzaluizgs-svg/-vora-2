#include "com_fio_host.cpp"
#include <cassert>
// leituras por dedo em cada postura (A0..A4 = pinos 14..18)
static int ab[5] = {600, 400, 512, 700, 300};
static int me[5] = {450, 520, 514, 560, 500};   // dedo 2: quase nao muda
static int fe[5] = {400, 700, 520, 300, 800};
static int postura = 0;      // 0 aberto, 1 meio, 2 fechado, 3 = valor livre
static int livre[5];
static int fake(int p) {
  int i = p - 14;
  if (postura == 3) return livre[i];
  return postura == 0 ? ab[i] : postura == 1 ? me[i] : fe[i];
}
// troca a postura conforme a mensagem de calibracao que apareceu por ultimo
static unsigned long t0;
static int fakeCal(int p) {
  int i = p - 14;
  size_t a = Serial.out.rfind("1/3"), m = Serial.out.rfind("2/3"), f = Serial.out.rfind("3/3");
  long ia = a == std::string::npos ? -1 : (long)a, im = m == std::string::npos ? -1 : (long)m, iff = f == std::string::npos ? -1 : (long)f;
  if (iff > im && iff > ia) return fe[i];
  if (im > ia) return me[i];
  return ab[i];
}
static void roda(int ms) { for (int t = 0; t < ms; t++) { loop(); g_millis++; } }
int main() {
  // 1) EEPROM vazia -> calibra em 3 etapas e grava
  g_analogFn = fakeCal;
  setup();
  for (int i = 0; i < 5; i++) printf("d%d ab=%d me=%d fe=%d ok=%d meio=%d\n", i, valorAberto[i], valorMeio[i], valorFechado[i], sensorOk[i], meioOk[i]);
  for (int i = 0; i < 5; i++) { assert(valorAberto[i] == ab[i]); assert(valorMeio[i] == me[i]); assert(valorFechado[i] == fe[i]); }
  assert(sensorOk[0] && sensorOk[1] && !sensorOk[2] && sensorOk[3] && sensorOk[4]);
  assert(meioOk[0] && meioOk[1] && meioOk[3] && meioOk[4]);
  assert(Serial.out.find("guardada") != std::string::npos);
  // 2) mapeamento em duas retas
  assert(paraPorcento(0, 600) == 0 && paraPorcento(0, 450) == 50 && paraPorcento(0, 400) == 100);
  assert(paraPorcento(0, 525) == 25 && paraPorcento(0, 425) == 75);
  assert(paraPorcento(1, 460) == 25 && paraPorcento(1, 610) == 75);
  assert(paraPorcento(0, 900) == 0 && paraPorcento(0, 100) == 100);   // fora da faixa: limita
  // 3) uso normal: postura meio -> 50 %
  g_analogFn = fake; postura = 1; roda(2000);
  for (int i = 0; i < 5; i++) printf("uso meio d%d pct=%d ang=%d\n", i, pctFinal[i], angAtual[i]);
  assert(pctFinal[0] == 50 && pctFinal[1] == 50 && pctFinal[2] == 0 && pctFinal[3] == 50 && pctFinal[4] == 50);
  assert(abs(angAtual[0] - 90) <= ZONA_MORTA);
  // 4) fio solto no indicador -> abre e avisa
  postura = 3; for (int i = 0; i < 5; i++) livre[i] = fe[i]; livre[1] = 0;
  Serial.out.clear(); roda(3000);
  assert(pctFinal[1] == 0 && pctFinal[0] == 100 && pctFinal[3] == 100);
  assert(Serial.out.find("FIO SOLTO? sensor do indicador") != std::string::npos);
  // 5) "reboot": carrega da EEPROM sem calibrar
  unsigned long antes = g_millis; Serial.out.clear();
  setup();
  assert(Serial.out.find("Usando a calibracao guardada") != std::string::npos);
  assert(g_millis - antes < 2500);   // so o tempo de ligar os servos
  for (int i = 0; i < 5; i++) assert(valorMeio[i] == me[i]);
  // 6) EEPROM corrompida -> calibra de novo
  EEPROM.mem[5] ^= 0x55; Serial.out.clear(); g_analogFn = fakeCal;
  setup();
  assert(Serial.out.find("CALIBRACAO 1/3") != std::string::npos);
  // 7) comando d -> demonstracao segue os gestos
  g_analogFn = fake; postura = 0;
  Serial.rx.push_back('d'); roda(100);
  assert(modoDemo);
  roda(2100);   // passa "aberta" (1,5 s) e entra no "punho"
  printf("demo gesto=%d pct0=%d ang0=%d\n", gestoAtual, pctFinal[0], angAtual[0]);
  assert(gestoAtual == 1 && pctFinal[0] == 100);
  roda(800);
  assert(angAtual[0] == ANG_FECHADO[0]);
  unsigned long ciclo = 0; for (int g = 0; g < NUM_GESTOS; g++) ciclo += GESTOS[g].ms;
  printf("ciclo da demonstracao = %lu ms, %d gestos\n", ciclo, NUM_GESTOS);
  Serial.rx.push_back('d'); roda(100); assert(!modoDemo);
  // 8) botao: segurar 2 s -> demo; aperto curto -> calibra
  g_botao = 0; roda(2100); g_botao = 1; roda(50);
  assert(modoDemo);
  g_botao = 0; roda(2100); g_botao = 1; roda(50);
  assert(!modoDemo);
  Serial.out.clear(); g_analogFn = fakeCal;
  g_botao = 0; roda(200); g_botao = 1; roda(1);
  assert(Serial.out.find("CALIBRACAO 1/3") != std::string::npos);
  // 9) ruido no botao (< 40 ms) nao faz nada
  Serial.out.clear(); g_analogFn = fake;
  g_botao = 0; roda(10); g_botao = 1; roda(50);
  assert(Serial.out.find("CALIBRACAO") == std::string::npos);
  // 10) meio fora da faixa -> usa so aberto/fechado
  valorMeio[0] = 700; conferirCalibracao(false); assert(!meioOk[0] && sensorOk[0]);
  assert(paraPorcento(0, 500) == 50);
  // 11) plotter
  Serial.out.clear(); Serial.rx.push_back('p'); roda(200);
  assert(Serial.out.find("polegar:") != std::string::npos && Serial.out.find(",minimo:") != std::string::npos);
  printf("TODOS OS TESTES do mao_com_fio PASSARAM\n");
}
