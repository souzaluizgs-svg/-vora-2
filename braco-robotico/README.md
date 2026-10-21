# Braço robótico Mark 2 com gravar e repetir

Projeto escolar: braço robótico MakerQuest Mark 2 (4 servos SG90, Arduino Uno R3, Shield V5, 2 joysticks).
O diferencial é o programa: controle por velocidade nos joysticks, limites por junta e gravar e repetir até
40 posições guardadas na EEPROM.

- `Guia-Braco-Robotico.pdf` e `guia/index.html`: guia de energia, ligações, código, calendário e apresentação.
- `firmware/braco_robotico/braco_robotico.ino`: o programa (Arduino IDE, sem bibliotecas extras).
- `testes/`: teste da lógica no computador.

```
cd braco-robotico/testes
{ echo '#include "stubs.h"'; sed -e 's/#include <Servo.h>//' -e 's/#include <EEPROM.h>//' ../firmware/braco_robotico/braco_robotico.ino; } > braco_host.cpp
g++ -std=c++17 -o t test.cpp && ./t
```

Compila no Uno com 8854 bytes de programa (27 %) e 547 bytes de RAM (26 %).
O projeto anterior, a mão robótica com luva, está em `mao-robotica/`.
