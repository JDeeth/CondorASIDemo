#include <Wire.h>
#include <U8g2lib.h>

class Display {
  public:
    void setup() {
      i2c.begin();
    }

    void print_msg(String msg, uint8_t row = 0) {
      i2c.clearBuffer();
      i2c.setFont(u8g2_font_5x8_tf);
      i2c.setCursor(1, 8 * (row+1));
      i2c.print(msg);
      i2c.sendBuffer();
    }

    void print_3dig_roll(float val) {
      i2c.clearBuffer();
      i2c.setFont(u8g2_font_logisoso24_tf);


      while(val < 0) {
        val += 1000;
      }

      const auto char_width = 16;
      val = fmod(val, 1000);
      auto ones = fmod(val, 10);
      auto one_digit = static_cast<int>(ones);
      uint8_t one_offset = 32 * fmod(val, 1);
      i2c.setCursor(1 + char_width * 2, 28 - one_offset);
      i2c.print(one_digit);
      i2c.setCursor(1 + char_width * 2, 60 - one_offset);
      i2c.print((one_digit + 1) % 10);

      auto ten_digit = (static_cast<int>(val) / 10) % 10;
      uint8_t ten_offset = (one_digit == 9) ? one_offset : 0;
      i2c.setCursor(1 + char_width * 1, 28 - ten_offset);
      i2c.print(ten_digit);
      i2c.setCursor(1 + char_width * 1, 60 - ten_offset);
      i2c.print((ten_digit + 1) % 10);

      auto hnd_digit = (static_cast<int>(val) / 100) % 10;
      uint8_t hnd_offset = (ten_digit == 9) ? ten_offset : 0;
      i2c.setCursor(1, 28 - hnd_offset);
      i2c.print(hnd_digit);
      i2c.setCursor(1, 60 - hnd_offset);
      i2c.print((hnd_digit + 1) % 10);

      i2c.setFont(u8g2_font_5x8_tf);
      i2c.drawStr(59, 10, "I");
      i2c.drawStr(59, 20, "A");
      i2c.drawStr(59, 30, "S");

      i2c.sendBuffer();
    }

  private:
    U8G2_SSD1306_64X32_1F_F_HW_I2C i2c = U8G2_SSD1306_64X32_1F_F_HW_I2C(U8G2_R0, U8X8_PIN_NONE);
};

