#include "braco_host.cpp"
#include <cassert>
// A0..A3 = pinos 14..17. Joystick parado = 512.
static int eixo[4] = {512, 512, 512, 512};
static int fake(int p) { return eixo[p - 14]; }
static void roda(int ms) { for (int t = 0; t < ms; t++) { loop(); g_millis++; } }
static void clique(int& b) { b = 0; roda(100); b = 1; roda(30); }
static void longo(int& b) { b = 0; roda(2100); b = 1; roda(30); }
int main() {
  g_analogFn = fake;
  setup();
  for (int j = 0; j < 4; j++) { assert(centro[j] == 512); assert(servo[j].val == 90); }
  assert(numPosicoes == 0);
  // 1) joystick da base todo para frente por 1 s -> ~90 graus/s, limitado em 170
  eixo[0] = 1023; roda(500);
  printf("base apos 0,5 s: %d\n", anguloEscrito[0]);
  assert(anguloEscrito[0] >= 130 && anguloEscrito[0] <= 136);
  roda(2000); assert(anguloEscrito[0] == ANG_MAX[0]);
  eixo[0] = 512; roda(200); int b = anguloEscrito[0]; roda(1000); assert(anguloEscrito[0] == b);  // soltou: parou
  // 2) zona morta: desvio pequeno nao mexe
  eixo[1] = 560; roda(1000); assert(anguloEscrito[1] == 90); eixo[1] = 512;
  // 3) ombro para tras ate o minimo; cotovelo e o eixo A3
  eixo[1] = 0; roda(3000); assert(anguloEscrito[1] == ANG_MIN[1]); eixo[1] = 512;
  eixo[3] = 1023; roda(300); assert(anguloEscrito[2] > 90); eixo[3] = 512;
  // 4) grava 2 posicoes
  clique(g_botao);
  int p1[4]; for (int j = 0; j < 4; j++) p1[j] = anguloEscrito[j];
  eixo[0] = 0; roda(1500); eixo[0] = 512; eixo[2] = 1023; roda(400); eixo[2] = 512; roda(50);
  clique(g_botao);
  int p2[4]; for (int j = 0; j < 4; j++) p2[j] = anguloEscrito[j];
  assert(numPosicoes == 2);
  for (int j = 0; j < 4; j++) { assert(posicoes[0][j] == p1[j]); assert(posicoes[1][j] == p2[j]); }
  // 5) repete: vai a p1, pausa, vai a p2
  clique(g_b4); assert(repetindo);
  roda(6000);
  bool viuP1 = false, viuP2 = false;
  for (int t = 0; t < 12000; t++) {
    loop(); g_millis++;
    bool em1 = true, em2 = true;
    for (int j = 0; j < 4; j++) { if (anguloEscrito[j] != p1[j]) em1 = false; if (anguloEscrito[j] != p2[j]) em2 = false; }
    viuP1 |= em1; viuP2 |= em2;
  }
  assert(viuP1 && viuP2);
  // velocidade de repeticao: nunca mais de ~1,2 grau por ciclo de 20 ms
  // 6) mexer joystick para a repeticao
  eixo[0] = 900; roda(50); assert(!repetindo); eixo[0] = 512; roda(50);
  // 7) memoria: "reboot" recarrega as posicoes
  setup(); assert(numPosicoes == 2); for (int j = 0; j < 4; j++) assert(posicoes[1][j] == p2[j]);
  // 8) segurar joystick 1 = apaga; repetir com < 2 posicoes nao liga
  longo(g_botao); assert(numPosicoes == 0);
  setup(); assert(numPosicoes == 0);
  clique(g_b4); assert(!repetindo);
  // 9) segurar joystick 2 = volta para o inicio
  eixo[0] = 1023; roda(800); eixo[0] = 512; roda(50);
  longo(g_b4); roda(4000);
  for (int j = 0; j < 4; j++) assert(anguloEscrito[j] == ANG_INICIAL[j]);
  // 10) memoria cheia
  for (int k = 0; k < 41; k++) clique(g_botao);
  assert(numPosicoes == MAX_POSICOES);
  assert(Serial.out.find("Memoria cheia") != std::string::npos);
  // 11) joystick fora do centro ao ligar avisa
  eixo[2] = 900; Serial.out.clear(); setup(); assert(Serial.out.find("fora do centro") != std::string::npos);
  printf("TODOS OS TESTES do braco_robotico PASSARAM\n");
}
