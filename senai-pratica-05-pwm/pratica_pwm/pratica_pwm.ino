/* Prática - Entrada Analógica e Saída PWM
   Controle de luminosidade de LED com Potenciômetro */

// Definindo os pinos como constantes
const int pot = A0;   // Entrada analógica do potenciômetro
const int led = 9;    // Saída PWM do LED

// Variáveis para armazenar as leituras
int valor = 0;    // Guarda a leitura analógica (0 a 1023)
int brilho = 0;   // Guarda o valor mapeado para o LED (0 a 255)

void setup() {
  pinMode(led, OUTPUT);   // Configura o pino do LED como saída
  Serial.begin(9600);     // Inicializa a comunicação serial com o computador
}

void loop() {
  // 1. Leitura e Conversão
  valor = analogRead(pot);                    // Lê a posição do potenciômetro (0 a 1023)
  brilho = map(valor, 0, 1023, 0, 255);       // Mapeia a escala para a saída PWM (0 a 255)

  // 2. Ação
  analogWrite(led, brilho);                   // Ajusta o brilho do LED

  // 3. Monitoramento Serial Organizado
  Serial.print("Potenciometro = ");           // Imprime o texto do potenciômetro
  Serial.print(valor);                        // Imprime o valor lido (0 a 1023)
  Serial.print("\t PWM (LED) = ");            // Usa '\t' para alinhar a coluna no monitor
  Serial.println(brilho);                     // Imprime o valor convertido (0 a 255) e pula linha

  delay(10); // Pausa suave de 10ms para estabilizar a exibição na tela
}
