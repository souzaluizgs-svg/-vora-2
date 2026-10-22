/* Desafio - Prática 05
   Controle analógico: lê um sinal de 0 a 5 V no A0 (potenciômetro)
   e acende 4 LEDs como uma barra de nível:
     a partir de 25%  -> LED 1
     a partir de 50%  -> LED 1 e 2
     a partir de 75%  -> LED 1, 2 e 3
     100%             -> os 4 LEDs                                  */

// Definindo os pinos como constantes
const int pot  = A0;   // Entrada analógica (0 a 5 V)
const int led1 = 9;    // LED 1 (25%)
const int led2 = 10;   // LED 2 (50%)
const int led3 = 11;   // LED 3 (75%)
const int led4 = 12;   // LED 4 (100%)

// Nível em que o LED 4 acende. No fim do curso, o potenciômetro pode ler um
// pouco menos que 1023 (ex.: 1020), e com 100 o LED 4 poderia nunca acender.
// Com 98 ele acende a partir de 1003 (4,90 V). Se o seu potenciômetro
// chega a 1023 no Monitor Serial, pode trocar para 100.
const int nivel100 = 98;

// Variáveis para armazenar as leituras
int valor = 0;         // Guarda a leitura analógica (0 a 1023)
int porcentagem = 0;   // Guarda o sinal em porcentagem (0 a 100)

void setup() {
  pinMode(led1, OUTPUT);   // Configura os pinos dos LEDs como saída
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  Serial.begin(9600);      // Inicializa a comunicação serial com o computador
}

void loop() {
  // 1. Leitura e Conversão
  valor = analogRead(pot);                     // Lê o sinal (0 a 1023 = 0 a 5 V)
  porcentagem = map(valor, 0, 1023, 0, 100);   // Converte para porcentagem (0 a 100)

  // 2. Ação: cada LED acende quando o sinal atinge a sua faixa
  if (porcentagem >= 25) {         // 25% = leitura 256 = 1,25 V
    digitalWrite(led1, HIGH);
  } else {
    digitalWrite(led1, LOW);
  }

  if (porcentagem >= 50) {         // 50% = leitura 512 = 2,50 V
    digitalWrite(led2, HIGH);
  } else {
    digitalWrite(led2, LOW);
  }

  if (porcentagem >= 75) {         // 75% = leitura 768 = 3,75 V
    digitalWrite(led3, HIGH);
  } else {
    digitalWrite(led3, LOW);
  }

  if (porcentagem >= nivel100) {   // 100% = fim de curso (ver nivel100)
    digitalWrite(led4, HIGH);
  } else {
    digitalWrite(led4, LOW);
  }

  // 3. Monitoramento Serial Organizado
  Serial.print("Potenciometro = ");
  Serial.print(valor);                      // Valor lido (0 a 1023)
  Serial.print("\t Sinal = ");
  Serial.print(porcentagem);                // Porcentagem (0 a 100)
  Serial.print("%\t Tensao = ");
  Serial.print(valor * 5.0 / 1023, 2);      // Tensão em volts, com 2 casas
  Serial.println(" V");

  delay(100); // 10 leituras por segundo: dá para ler os números no monitor
}
