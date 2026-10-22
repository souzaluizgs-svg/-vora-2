# Prática 05: Controle de saída PWM (SENAI)

Atividade prática Arduino 05 do Técnico em Automação Industrial (Circuitos Microcontrolados).

| Sketch | O que faz |
|---|---|
| `pratica_pwm` | Sketch da folha: o potenciômetro em A0 controla o brilho do LED no D9 (PWM) e mostra os valores no Monitor Serial |
| `desafio_4_leds` | Desafio: 4 LEDs acendem em barra conforme o sinal de 0 a 5 V no A0 |

Os dois compilam sem avisos para o Arduino Uno (Arduino AVR 1.8.6).

## Ligações do desafio

Além do material da prática, separe mais **3 LEDs e 3 resistores de 330 Ω**: cada LED precisa do seu resistor.

| Componente | Ligação |
|---|---|
| Potenciômetro | uma ponta no 5V, a outra no GND, perna do meio no A0 (igual à prática) |
| LED 1 (25%) | D9 → perna longa (+); perna curta (−) → resistor 330 Ω → GND |
| LED 2 (50%) | D10 → perna longa (+); perna curta (−) → resistor 330 Ω → GND |
| LED 3 (75%) | D11 → perna longa (+); perna curta (−) → resistor 330 Ω → GND |
| LED 4 (100%) | D12 → perna longa (+); perna curta (−) → resistor 330 Ω → GND |

O LED 1 é o mesmo da prática, que já está no D9: é só acrescentar os outros três.

Não use um resistor só para os 4 LEDs: o brilho cai à medida que mais LEDs acendem, e LEDs de cores diferentes dividem a corrente de forma desigual.

## Faixas

| Sinal | Leitura (`analogRead`) | LEDs acesos |
|---|---|---|
| abaixo de 25% (< 1,25 V) | 0 a 255 | nenhum |
| 25% (1,25 V) | 256 a 511 | 1 |
| 50% (2,50 V) | 512 a 767 | 1 e 2 |
| 75% (3,75 V) | 768 a 1002 | 1, 2 e 3 |
| 100% (fim de curso, ≥ 4,90 V) | 1003 a 1023 | os 4 |

O LED 4 acende a partir de 98% de propósito: no fim do curso, o potenciômetro pode ler um pouco menos que 1023 (por exemplo 1020), e com 100% o LED 4 nunca acenderia. Se o seu ler 1023 no Monitor Serial, troque `nivel100` para `100`.

## Como testar

1. Envie o `desafio_4_leds` e abra o Monitor Serial em 9600.
2. Gire o potenciômetro devagar: o monitor mostra a leitura, a porcentagem e a tensão, e os LEDs acendem um a um nas faixas acima.
3. Gire até o fim e confira se os 4 LEDs estão acesos.

Se você parar o potenciômetro exatamente em cima de um limite, o LED daquele limite pode piscar, porque a leitura oscila ±1. Isso é ruído normal do conversor analógico.
