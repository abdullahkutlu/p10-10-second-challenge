// ============================================================
// 10.00 SANİYE OYUNU
// Arduino UNO + P10 32x16
// DP4536 + 8x DP5125
//
// KAZANMA ARALIĞI: 9.95 - 10.05 saniye
//
// BUTON:
// D2 ---- BUTON ---- GND
//
// AKTIF BUZZER:
// D3 ---- (+) BUZZER (-) ---- GND
//
// P10:
// A     -> D6
// B     -> D7
// LAT   -> D8
// OE    -> D9
// DATA  -> D11
// CLK   -> D13
//
// Panel harici 5V güç kaynağından beslenecek.
// Arduino ve panel GND ortak olacak.
// ============================================================


// ===================== PINLER ================================

#define PIN_BUTTON 2
#define PIN_BUZZER 3

#define PIN_A     6
#define PIN_B     7
#define PIN_LAT   8
#define PIN_OE    9
#define PIN_DATA  11
#define PIN_CLK   13


// ===================== OYUN DURUMLARI ========================

enum GameState {

  READY,          // İlk açılış: 00.00
  RUNNING,        // Kronometre çalışıyor
  RESULT_FACE,    // Mutlu / üzgün yüz gösteriliyor
  RESULT_TIME     // Sonuç süresi ekranda bekliyor
};

GameState state = READY;


// ===================== ZAMAN AYARLARI ========================

// Maksimum oyun süresi
const unsigned long MAX_TIME = 20000;

// Kazanma aralığı
const unsigned long WIN_MIN = 9950;
const unsigned long WIN_MAX = 10050;

// Yüzün ekranda kalma süresi
const unsigned long FACE_TIME = 2000;


unsigned long startTime = 0;
unsigned long stoppedTime = 0;
unsigned long faceStartTime = 0;


// ===================== BUTON DEBOUNCE ========================

bool lastRawButton = HIGH;
bool stableButton = HIGH;

unsigned long debounceTime = 0;

const unsigned long DEBOUNCE_MS = 30;


// ===================== BUZZER ================================

bool buzzerRunning = false;
bool buzzerState = false;

unsigned long buzzerTimer = 0;

byte buzzerStep = 0;
byte soundType = 0;


// ===================== FRAME BUFFER ==========================
//
// 16 satır × 32 sütun
//

bool frame[16][32];


// ===================== 5x7 SAYI FONTU ========================

const byte digitFont[10][5] = {

  {0x3E,0x51,0x49,0x45,0x3E}, // 0
  {0x00,0x42,0x7F,0x40,0x00}, // 1
  {0x42,0x61,0x51,0x49,0x46}, // 2
  {0x21,0x41,0x45,0x4B,0x31}, // 3
  {0x18,0x14,0x12,0x7F,0x10}, // 4
  {0x27,0x45,0x45,0x45,0x39}, // 5
  {0x3C,0x4A,0x49,0x49,0x30}, // 6
  {0x01,0x71,0x09,0x05,0x03}, // 7
  {0x36,0x49,0x49,0x49,0x36}, // 8
  {0x06,0x49,0x49,0x29,0x1E}  // 9
};


// ============================================================
// FRAME BUFFER TEMİZLE
// ============================================================

void clearFrame() {

  for (byte y = 0; y < 16; y++) {

    for (byte x = 0; x < 32; x++) {

      frame[y][x] = false;
    }
  }
}


// ============================================================
// TEK PİKSEL
// ============================================================

void pixel(int x, int y, bool on = true) {

  if (x < 0 || x >= 32)
    return;

  if (y < 0 || y >= 16)
    return;

  frame[y][x] = on;
}


// ============================================================
// RAKAM ÇİZ
// ============================================================

void drawDigit(byte number, int x, int y) {

  if (number > 9)
    return;


  for (byte col = 0; col < 5; col++) {

    byte data = digitFont[number][col];


    for (byte row = 0; row < 7; row++) {

      if (bitRead(data, row)) {

        pixel(x + col, y + row);
      }
    }
  }
}


// ============================================================
// KRONOMETREYİ ÇİZ
//
// FORMAT:
// SS.CC
//
// Örnek:
//
// 09.95
// 10.00
// 10.05
//
// CC = salise
// 1 salise = 10 ms
// ============================================================

void drawTime(unsigned long elapsed) {

  clearFrame();


  if (elapsed > MAX_TIME)
    elapsed = MAX_TIME;


  // Milisaniyeyi saliseye çevir

  unsigned int centiseconds = elapsed / 10;


  byte seconds = centiseconds / 100;

  byte cents = centiseconds % 100;


  byte s1 = seconds / 10;

  byte s2 = seconds % 10;

  byte c1 = cents / 10;

  byte c2 = cents % 10;


  // Ekrana ortala

  int x = 2;

  int y = 4;


  // İlk saniye rakamı

  drawDigit(s1, x, y);

  x += 6;


  // İkinci saniye rakamı

  drawDigit(s2, x, y);

  x += 6;


  // NOKTA

  pixel(x, y + 6);

  pixel(x + 1, y + 6);

  x += 3;


  // İlk salise rakamı

  drawDigit(c1, x, y);

  x += 6;


  // İkinci salise rakamı

  drawDigit(c2, x, y);
}


// ============================================================
// MUTLU YÜZ :)
// ============================================================

void drawHappy() {

  clearFrame();


  // GÖZLER

  pixel(9,5);
  pixel(10,5);

  pixel(21,5);
  pixel(22,5);


  // GÜLÜMSEME

  pixel(8,9);
  pixel(9,10);

  pixel(10,11);
  pixel(11,11);

  pixel(12,12);
  pixel(13,12);
  pixel(14,12);
  pixel(15,12);
  pixel(16,12);
  pixel(17,12);
  pixel(18,12);
  pixel(19,12);

  pixel(20,11);
  pixel(21,11);

  pixel(22,10);
  pixel(23,9);
}


// ============================================================
// ÜZGÜN YÜZ :(
// ============================================================

void drawSad() {

  clearFrame();


  // GÖZLER

  pixel(9,5);
  pixel(10,5);

  pixel(21,5);
  pixel(22,5);


  // ÜZGÜN AĞIZ

  pixel(8,12);
  pixel(9,11);

  pixel(10,10);
  pixel(11,9);

  pixel(12,9);

  pixel(13,8);
  pixel(14,8);
  pixel(15,8);
  pixel(16,8);
  pixel(17,8);
  pixel(18,8);

  pixel(19,9);
  pixel(20,9);

  pixel(21,10);
  pixel(22,11);
  pixel(23,12);
}


// ============================================================
// P10 PANEL REFRESH
//
// Bu mapping bizim panel üzerinde deneysel olarak çıkarıldı.
//
// PANEL:
// DP4536 + 8x DP5125
//
// DATA:
// LOW  = LED ON
// HIGH = LED OFF
//
// OE:
// HIGH = DISPLAY ON
// LOW  = DISPLAY OFF
// ============================================================

void refreshScan(byte scan) {


  // Görüntüyü kapat

  digitalWrite(PIN_OE, LOW);


  // 1/4 SCAN adresi

  digitalWrite(PIN_A, scan & 1);

  digitalWrite(PIN_B, (scan >> 1) & 1);


  // Her scan için 128 bit gönderiyoruz

  for (byte block = 0; block < 4; block++) {


    for (byte rowGroup = 0; rowGroup < 4; rowGroup++) {


      int y =
        (3 - scan) + rowGroup * 4;


      for (byte p = 0; p < 8; p++) {


        int x =
          31 - block * 8 - p;


        bool led =
          frame[y][x];


        // Panel DATA polaritesi ters

        digitalWrite(
          PIN_DATA,
          led ? LOW : HIGH
        );


        // CLOCK PULSE

        digitalWrite(PIN_CLK, HIGH);

        digitalWrite(PIN_CLK, LOW);
      }
    }
  }


  // LATCH

  digitalWrite(PIN_LAT, HIGH);

  delayMicroseconds(1);

  digitalWrite(PIN_LAT, LOW);


  // Görüntüyü aç

  digitalWrite(PIN_OE, HIGH);


  // Scan satırını kısa süre göster

  delayMicroseconds(500);


  // Tekrar kapat

  digitalWrite(PIN_OE, LOW);
}


// ============================================================
// BUTON OKUMA
//
// INPUT_PULLUP:
// HIGH = basılmıyor
// LOW  = basılıyor
// ============================================================

bool buttonPressed() {

  bool raw =
    digitalRead(PIN_BUTTON);


  // Buton durumunda değişiklik oldu

  if (raw != lastRawButton) {

    debounceTime = millis();

    lastRawButton = raw;
  }


  // Sinyal 30 ms stabil kaldı mı?

  if (millis() - debounceTime > DEBOUNCE_MS) {


    if (raw != stableButton) {


      stableButton = raw;


      // Butona yeni basıldı

      if (stableButton == LOW) {

        return true;
      }
    }
  }


  return false;
}


// ============================================================
// KAZANMA SESİ
//
// Aktif buzzer:
// bip bip bip - biiip
// ============================================================

void startWinSound() {

  soundType = 1;

  buzzerRunning = true;

  buzzerState = true;

  buzzerStep = 0;

  buzzerTimer = millis();


  digitalWrite(PIN_BUZZER, HIGH);
}


// ============================================================
// KAYBETME SESİ
//
// biiip - biiip - biiiiip
// ============================================================

void startLoseSound() {

  soundType = 2;

  buzzerRunning = true;

  buzzerState = true;

  buzzerStep = 0;

  buzzerTimer = millis();


  digitalWrite(PIN_BUZZER, HIGH);
}


// ============================================================
// BUZZER UPDATE
//
// delay() YOK.
// Böylece panel refresh durmuyor.
// ============================================================

void updateBuzzer() {

  if (!buzzerRunning)
    return;


  unsigned long now =
    millis();


  // ================= KAZANMA =================

  if (soundType == 1) {


    const unsigned int durations[] = {

      100, 70,
      100, 70,
      100, 100,
      500
    };


    if (
      now - buzzerTimer
      >= durations[buzzerStep]
    ) {


      buzzerTimer = now;

      buzzerStep++;


      if (buzzerStep >= 7) {

        digitalWrite(
          PIN_BUZZER,
          LOW
        );

        buzzerRunning = false;

        return;
      }


      buzzerState =
        !buzzerState;


      digitalWrite(
        PIN_BUZZER,
        buzzerState
      );
    }
  }


  // ================= KAYBETME =================

  else if (soundType == 2) {


    const unsigned int durations[] = {

      350, 180,
      350, 180,
      700
    };


    if (
      now - buzzerTimer
      >= durations[buzzerStep]
    ) {


      buzzerTimer = now;

      buzzerStep++;


      if (buzzerStep >= 5) {

        digitalWrite(
          PIN_BUZZER,
          LOW
        );

        buzzerRunning = false;

        return;
      }


      buzzerState =
        !buzzerState;


      digitalWrite(
        PIN_BUZZER,
        buzzerState
      );
    }
  }
}


// ============================================================
// YENİ OYUN BAŞLAT
// ============================================================

void startGame() {

  // Eski sonucu temizle

  stoppedTime = 0;


  // Kronometre başlangıcı

  startTime = millis();


  state = RUNNING;


  drawTime(0);
}


// ============================================================
// OYUNU BİTİR
// ============================================================

void finishGame(unsigned long elapsed) {


  stoppedTime = elapsed;


  // 20 saniyeyi aşmasın

  if (stoppedTime > MAX_TIME) {

    stoppedTime = MAX_TIME;
  }


  // ==========================================================
  // KAZANMA KONTROLÜ
  //
  // 9.950 saniye
  //       ↓
  //       KAZANMA BÖLGESİ
  //       ↓
  // 10.050 saniye
  //
  // SINIRLAR DAHİL
  // ==========================================================

  bool win =
    stoppedTime >= WIN_MIN &&
    stoppedTime <= WIN_MAX;


  if (win) {


    // ========================================================
    // KAZANDI
    //
    // Gerçekte örneğin:
    // 9.97
    // 10.02
    // 10.05
    //
    // yapmış olsa bile sonuç ekranında:
    //
    // 10.00
    //
    // gösterilecek.
    // ========================================================

    stoppedTime = 10000;


    drawHappy();


    startWinSound();
  }


  else {


    // ========================================================
    // KAYBETTİ
    //
    // Gerçek durdurduğu süre korunur.
    //
    // Örnek:
    // 9.84
    // 10.23
    // 14.52
    // ========================================================

    drawSad();


    startLoseSound();
  }


  // Yüzün gösterilmeye başladığı an

  faceStartTime = millis();


  state = RESULT_FACE;
}


// ============================================================
// SETUP
// ============================================================

void setup() {


  // ================= BUTON =================

  pinMode(
    PIN_BUTTON,
    INPUT_PULLUP
  );


  // ================= BUZZER =================

  pinMode(
    PIN_BUZZER,
    OUTPUT
  );


  digitalWrite(
    PIN_BUZZER,
    LOW
  );


  // ================= PANEL =================

  pinMode(PIN_A, OUTPUT);

  pinMode(PIN_B, OUTPUT);

  pinMode(PIN_LAT, OUTPUT);

  pinMode(PIN_OE, OUTPUT);

  pinMode(PIN_DATA, OUTPUT);

  pinMode(PIN_CLK, OUTPUT);


  digitalWrite(
    PIN_OE,
    LOW
  );


  digitalWrite(
    PIN_LAT,
    LOW
  );


  digitalWrite(
    PIN_CLK,
    LOW
  );


  // İlk açılış ekranı

  clearFrame();


  drawTime(0);
}


// ============================================================
// ANA PROGRAM
// ============================================================

void loop() {


  // ==========================================================
  // P10 REFRESH
  //
  // Sürekli çalışmak zorunda.
  // ==========================================================

  static byte scan = 0;


  refreshScan(scan);


  scan++;


  if (scan >= 4) {

    scan = 0;
  }


  // ==========================================================
  // BUZZER
  // ==========================================================

  updateBuzzer();


  // ==========================================================
  // BUTON
  // ==========================================================

  bool pressed =
    buttonPressed();


  // ==========================================================
  // READY
  //
  // Arduino ilk açıldığında:
  //
  // 00.00
  //
  // gösterilir.
  // ==========================================================

  if (state == READY) {


    if (pressed) {

      startGame();
    }
  }


  // ==========================================================
  // RUNNING
  //
  // Kronometre çalışıyor.
  // ==========================================================

  else if (state == RUNNING) {


    unsigned long elapsed =
      millis() - startTime;


    // --------------------------------------------------------
    // 20 saniye doldu
    // --------------------------------------------------------

    if (elapsed >= MAX_TIME) {


      // Otomatik kayıp

      finishGame(MAX_TIME);
    }


    // --------------------------------------------------------
    // Henüz 20 saniye olmadı
    // --------------------------------------------------------

    else {


      // Kronometreyi ekrana çiz

      drawTime(elapsed);


      // Oyuncu butona bastı

      if (pressed) {


        // Kronometreyi durdur

        finishGame(elapsed);
      }
    }
  }


  // ==========================================================
  // RESULT_FACE
  //
  // Kazandıysa :)
  // Kaybettiyse :(
  //
  // 2 saniye göster.
  // ==========================================================

  else if (state == RESULT_FACE) {


    if (
      millis() - faceStartTime
      >= FACE_TIME
    ) {


      // Yüzden sonra sonucu göster

      drawTime(stoppedTime);


      state = RESULT_TIME;
    }
  }


  // ==========================================================
  // RESULT_TIME
  //
  // KAZANDI:
  // 10.00
  //
  // KAYBETTİ:
  // gerçek süre
  //
  // Burada sonsuza kadar bekler.
  //
  // Tekrar basınca yeni oyun başlar.
  // ==========================================================

  else if (state == RESULT_TIME) {


    if (pressed) {


      // Yeni oyun

      startGame();
    }
  }
}
