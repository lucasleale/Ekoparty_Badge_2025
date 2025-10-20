using namespace std;

class TouchSensor {
public:
  TouchSensor(int pin, int threshold = 1015)
    : pin(pin),
      threshold(threshold),
      f_s(250),  //sample rate
      f_c(60),   //notch freq
      f_n(2 * f_c / f_s),
      timer(std::round(1e6 / f_s)),
      filter1(simpleNotchFIR(f_n)),
      filter2(simpleNotchFIR(2 * f_n)) {}

  bool update() {
    // Retorna true cuando toca muestrear
    if (timer) {
      int raw = analogRead(pin);
      filtered = filter2(filter1(raw));
      checkTouch();
      return true;
    }
    return false;
  }

  float getFiltered() const {
    return filtered;
  }

  bool justPressed() {
    if (pressedEvent) {
      pressedEvent = false;
      return true;
    }
    return false;
  }

  bool justReleased() {
    if (releasedEvent) {
      releasedEvent = false;
      return true;
    }
    return false;
  }

  bool isPressed() const {
    return pressed;
  }

private:
  int pin;
  double f_s, f_c, f_n;
  int threshold;
  Timer<micros> timer;
  decltype(simpleNotchFIR(0.1)) filter1;
  decltype(simpleNotchFIR(0.1)) filter2;

  float filtered = 0;
  bool pressed = false;
  bool pressedEvent = false;
  bool releasedEvent = false;
  elapsedMillis bounce;

  void checkTouch() {
    
    
    const int debounceMs = 25;

    if (filtered < threshold && !pressed) {
      // presionado
      pressed = true;
      pressedEvent = true;
      bounce = 0;
    } else if (filtered >= threshold && pressed && bounce > debounceMs) {
      // liberado
      pressed = false;
      releasedEvent = true;
      bounce = 0;
    }
  }
};