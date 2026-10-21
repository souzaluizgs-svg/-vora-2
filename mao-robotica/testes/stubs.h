// Stubs minimos para testar a logica dos sketches do Uno no PC
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
typedef uint8_t byte;
#define F(x) (x)
#define HIGH 1
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define INPUT_PULLUP 2
#define A0 14
#define A1 15
#define A2 16
#define A3 17
#define A4 18
static unsigned long g_millis = 0;
inline unsigned long millis() { return g_millis; }
inline void delay(unsigned long ms) { g_millis += ms; }
inline void pinMode(int, int) {}
static int g_led = 0;
inline void digitalWrite(int p, int v) { if (p == 13) g_led = v; }
static int g_analog[19];
static int (*g_analogFn)(int) = nullptr;
inline int analogRead(int p) { return g_analogFn ? g_analogFn(p) : g_analog[p]; }
static int g_dig[19];
static int g_botao = 1;  // D2 com pull-up: 1 = solto
inline int digitalRead(int p) { return (p == 2) ? g_botao : g_dig[p]; }
inline long map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }
template <class T, class U, class V> T constrain(T x, U a, V b) { return x < a ? a : (x > b ? b : x); }
struct FakeSerial {
  std::deque<uint8_t> rx; std::string out;
  void begin(long) {}
  int available() { return (int)rx.size(); }
  int read() { if (rx.empty()) return -1; int b = rx.front(); rx.pop_front(); return b; }
  template <class T> void print(T v) { out += std::to_string(v); }
  void print(const char* s) { out += s; }
  void print(char c) { out += c; }
  void print(int v) { out += std::to_string(v); }
  template <class T> void println(T v) { print(v); out += "\n"; }
  void println() { out += "\n"; }
} Serial;
struct Servo {
  int pin = -1, val = 90;
  void attach(int p) { pin = p; }
  void write(int v) { val = v; }
};

struct FakeEEPROM {
  uint8_t mem[1024];
  FakeEEPROM() { memset(mem, 0xFF, sizeof(mem)); }
  template <class T> void put(int a, const T& v) { memcpy(mem + a, &v, sizeof(T)); }
  template <class T> void get(int a, T& v) { memcpy(&v, mem + a, sizeof(T)); }
} EEPROM;
