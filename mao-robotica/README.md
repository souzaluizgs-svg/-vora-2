# Mão robótica com luva de sensores Hall

Projeto escolar: mão impressa em 3D com 5 servos SG90 controlada por um Arduino Uno
e por uma luva com sensores Hall SS49E e ímãs. Tem uma versão opcional sem fio com dois ESP32 (ESP-NOW).

- `Guia-Mao-Robotica.pdf`: o guia completo (críticas ao plano, compras, energia, ligações, código e calendário).
- `guia/index.html`: o mesmo guia em página web, com as imagens em `guia/img/`.
- `firmware/`: os 5 sketches da Arduino IDE.

| Sketch | Placa | Para quê |
|---|---|---|
| `teste_servos` | Arduino Uno | Calibrar os ângulos de cada dedo pelo Monitor Serial |
| `teste_sensores` | Arduino Uno | Ver os 5 sensores no Serial Plotter para achar a posição do ímã |
| `mao_com_fio` | Arduino Uno | Versão 1: sensores em A0–A4, servos em D3, D5, D6, D9, D10, botão opcional no D2; calibração em 3 posições guardada na EEPROM, modo demonstração, Serial Plotter, detecção de fio solto |
| `luva_esp32` | ESP32 DevKit V1 | Versão 2: lê os sensores (GPIO 36, 34, 35, 32, 33) e envia por ESP-NOW |
| `receptor_esp32` | ESP32 DevKit V1 | Versão 2: recebe e repassa ao Uno pelo TX2 (GPIO17) |
| `mao_sem_fio` | Arduino Uno | Versão 2: lê os quadros no D0 e move os servos, com failsafe |

`testes/`: testes da lógica no computador (veja `testes/README.md`).

Todos compilam sem avisos: Uno com Arduino AVR 1.8.6 e Servo 1.2.2, ESP32 com os cores 2.0.17 e 3.3.12.
