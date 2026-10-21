/*
  ==========================================================================
  TESTE DOS SERVOS  -  ferramenta de calibracao (Arduino Uno)
  ==========================================================================
  Para que serve:
    Descobrir, dedo por dedo, qual angulo deixa o dedo ABERTO (esticado)
    e qual angulo deixa o dedo FECHADO (dobrado). Esses numeros vao para
    as tabelas ANG_ABERTO e ANG_FECHADO dos programas "mao_com_fio" e
    "mao_sem_fio".

  Como usar:
    1. Grave este programa no Uno.
    2. Abra o Monitor Serial (lupa no canto da IDE) em 115200 baud.
    3. Digite comandos e aperte Enter:
         2 90    -> move o servo 2 (indicador) para 90 graus
         a       -> leva todos os dedos para ANG_ABERTO (repouso)
         f       -> leva todos os dedos para ANG_FECHADO
         ?       -> mostra a ajuda de novo
    4. Va mudando o angulo aos poucos (de 5 em 5 graus). Quando o dedo
       ficar bem esticado, anote: esse e o ANG_ABERTO. Quando ficar bem
       dobrado SEM o servo forcar (sem zumbido alto), anote: ANG_FECHADO.

  Numeracao dos servos (igual em todos os programas):
    1 = polegar  -> pino D3
    2 = indicador-> pino D5
    3 = medio    -> pino D6
    4 = anelar   -> pino D9
    5 = minimo   -> pino D10

  Ligacao: fio laranja/amarelo do servo = sinal (pino acima),
           vermelho = + do pack de pilhas (4,8 V), marrom = GND.
           O GND do pack TEM que estar ligado no GND do Uno.

  Dica de montagem: grave este programa ANTES de prender o "chifre"
  (a pecinha branca) no servo. Ao ligar, todos vao para 90 graus; prenda
  o chifre com o servo nessa posicao (meio do curso).
  ==========================================================================
*/

#include <Servo.h>

// ---------------- Tabelas iguais as dos outros programas ----------------
// Ordem: polegar, indicador, medio, anelar, minimo
const uint8_t PINO_SERVO[5]  = { 3, 5, 6, 9, 10 };
const char*   NOME_DEDO[5]   = { "polegar", "indicador", "medio", "anelar", "minimo" };

// Troque estes numeros pelos que voce anotar no teste.
int ANG_ABERTO[5]  = { 20, 20, 20, 20, 20 };     // dedo esticado
int ANG_FECHADO[5] = { 160, 160, 160, 160, 160 }; // dedo dobrado

const int ANG_INICIAL = 90;        // posicao ao ligar (meio do curso)
const int ATRASO_ENTRE_SERVOS = 300; // ms entre um servo e o proximo

Servo servo[5];
int angAtual[5];

char linha[24];        // texto digitado
uint8_t tamLinha = 0;
unsigned long ultimoCaractere = 0;

// Move os 5 servos, UM DE CADA VEZ, para os angulos da tabela.
// Mover todos juntos puxa muita corrente de uma vez e pode reiniciar o Uno.
void moverTodos(const int angulos[5]) {
  for (uint8_t i = 0; i < 5; i++) {
    servo[i].write(angulos[i]);
    angAtual[i] = angulos[i];
    delay(ATRASO_ENTRE_SERVOS);
  }
}

void mostrarAjuda() {
  Serial.println();
  Serial.println(F("=== TESTE DOS SERVOS - Mao Robotica ==="));
  Serial.println(F("Comandos (digite e aperte Enter):"));
  Serial.println(F("  N ANGULO  -> ex.: 2 90  (servo 1 a 5, angulo 0 a 180)"));
  Serial.println(F("  a         -> todos para ANG_ABERTO (repouso)"));
  Serial.println(F("  f         -> todos para ANG_FECHADO (use depois de ajustar a tabela)"));
  Serial.println(F("  ?         -> esta ajuda"));
  Serial.println(F("Servos: 1=polegar(D3) 2=indicador(D5) 3=medio(D6) 4=anelar(D9) 5=minimo(D10)"));
  Serial.println(F("Va de 5 em 5 graus. Se o servo zumbir forte, volte um pouco!"));
  Serial.println();
}

void mostrarAngulos() {
  Serial.print(F("Angulos agora: "));
  for (uint8_t i = 0; i < 5; i++) {
    Serial.print(NOME_DEDO[i]);
    Serial.print('=');
    Serial.print(angAtual[i]);
    Serial.print(i < 4 ? F("  ") : F("\n"));
  }
}

// Interpreta o que foi digitado
void executarComando(char* txt) {
  // pula espacos no comeco
  while (*txt == ' ') txt++;
  if (*txt == '\0') return;

  if (*txt == '?' || *txt == 'h' || *txt == 'H') {
    mostrarAjuda();
    return;
  }
  if (*txt == 'a' || *txt == 'A') {
    Serial.println(F("Abrindo todos (ANG_ABERTO)..."));
    moverTodos(ANG_ABERTO);
    mostrarAngulos();
    return;
  }
  if (*txt == 'f' || *txt == 'F') {
    Serial.println(F("Fechando todos (ANG_FECHADO)..."));
    moverTodos(ANG_FECHADO);
    mostrarAngulos();
    return;
  }

  // Formato "N ANGULO"
  char* resto;
  long n = strtol(txt, &resto, 10);
  if (resto == txt) {
    Serial.println(F("Nao entendi. Digite ? para ajuda."));
    return;
  }
  char* fim;
  long ang = strtol(resto, &fim, 10);
  if (fim == resto) {
    Serial.println(F("Falta o angulo. Exemplo: 2 90"));
    return;
  }
  if (n < 1 || n > 5) {
    Serial.println(F("Servo invalido: use 1 a 5."));
    return;
  }
  if (ang < 0 || ang > 180) {
    Serial.println(F("Angulo invalido: use 0 a 180."));
    return;
  }

  uint8_t i = n - 1;
  servo[i].write((int)ang);
  angAtual[i] = ang;
  Serial.print(F("Servo "));
  Serial.print(n);
  Serial.print(F(" ("));
  Serial.print(NOME_DEDO[i]);
  Serial.print(F(") -> "));
  Serial.print(ang);
  Serial.println(F(" graus"));
  mostrarAngulos();
}

void setup() {
  Serial.begin(115200);

  // Liga os servos um por um, ja em 90 graus, com uma pausa entre eles.
  // (write antes de attach = o primeiro pulso ja sai no angulo certo)
  for (uint8_t i = 0; i < 5; i++) {
    servo[i].write(ANG_INICIAL);
    servo[i].attach(PINO_SERVO[i]);
    angAtual[i] = ANG_INICIAL;
    delay(ATRASO_ENTRE_SERVOS);
  }

  mostrarAjuda();
  mostrarAngulos();
}

void loop() {
  // Junta as letras digitadas ate chegar o Enter.
  while (Serial.available() > 0) {
    char c = Serial.read();
    ultimoCaractere = millis();
    if (c == '\n' || c == '\r') {
      linha[tamLinha] = '\0';
      executarComando(linha);
      tamLinha = 0;
    } else if (tamLinha < sizeof(linha) - 1) {
      linha[tamLinha++] = c;
    }
  }

  // Se o Monitor estiver em "Nenhum final de linha", nao chega Enter.
  // Entao, depois de 200 ms sem letras novas, executa assim mesmo.
  if (tamLinha > 0 && millis() - ultimoCaractere > 200) {
    linha[tamLinha] = '\0';
    executarComando(linha);
    tamLinha = 0;
  }
}
