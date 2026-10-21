# Testes da lógica do firmware no computador

Simulam o Arduino (millis, analogRead, Serial, Servo, EEPROM) para testar a lógica dos sketches do Uno sem hardware.

```
cd mao-robotica/testes
{ echo '#include "stubs.h"'; echo '#include <cstddef>'; sed -e 's/#include <Servo.h>//' -e 's/#include <EEPROM.h>//' ../firmware/mao_com_fio/mao_com_fio.ino; } > com_fio_host.cpp
{ echo '#include "stubs.h"'; sed 's/#include <Servo.h>//' ../firmware/mao_sem_fio/mao_sem_fio.ino; } > sem_fio_host.cpp
g++ -std=c++17 -o t1 test.cpp && ./t1
g++ -std=c++17 -o t2 test_sem_fio.cpp && ./t2
g++ -std=c++17 -o t3 test_sem_fio_demo.cpp && ./t3
```
