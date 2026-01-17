#define BOARD_NONE_PIN (-1)

#define BOARD_POWERON (gpio_num_t)(15)

#define BOARD_TFT_BL (38)
#define BOARD_TFT_DATA0 (39)
#define BOARD_TFT_DATA1 (40)
#define BOARD_TFT_DATA2 (41)
#define BOARD_TFT_DATA3 (42)
#define BOARD_TFT_DATA4 (45)
#define BOARD_TFT_DATA5 (46)
#define BOARD_TFT_DATA6 (47)
#define BOARD_TFT_DATA7 (48)
#define BOARD_TFT_RST (5)
#define BOARD_TFT_CS (6)
#define BOARD_TFT_DC (7)
#define BOARD_TFT_WR (8)
#define BOARD_TFT_RD (9)
#define BOARD_I2C_SCL (17)
#define BOARD_I2C_SDA (18)
#define BOARD_TOUCH_IRQ (16)
#define BOARD_TOUCH_RST (21)
#define AMOLED_WIDTH (170)
#define AMOLED_HEIGHT (320)

#define BOARD_HAS_TOUCH 1

#define DISPLAY_BUFFER_SIZE (AMOLED_WIDTH * 100)

#define DISPLAY_FULLRESH false

#define BOARD_HEATER_GPIO (10)
