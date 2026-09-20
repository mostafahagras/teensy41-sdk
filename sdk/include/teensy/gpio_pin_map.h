#ifndef TEENSY_GPIO_PIN_MAP_H
#define TEENSY_GPIO_PIN_MAP_H

/* Board pin, GPIO port, GPIO bit, mux register, pad register. */
#define TEENSY_GPIO_PIN_MAP(X)                                                 \
  X(0, 6, 3, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_03,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_03)                                       \
  X(1, 6, 2, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_02,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_02)                                       \
  X(2, 9, 4, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_04,                                \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_04)                                         \
  X(3, 9, 5, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_05,                                \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_05)                                         \
  X(4, 9, 6, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_06,                                \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_06)                                         \
  X(5, 9, 8, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_08,                                \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_08)                                         \
  X(6, 7, 10, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_10,                                \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_10)                                          \
  X(7, 7, 17, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_01,                                \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_01)                                          \
  X(8, 7, 16, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_00,                                \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_00)                                          \
  X(9, 7, 11, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_11,                                \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_11)                                          \
  X(10, 7, 0, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_00,                                \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_00)                                          \
  X(11, 7, 2, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_02,                                \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_02)                                          \
  X(12, 7, 1, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_01,                                \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_01)                                          \
  X(13, 7, 3, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_03,                                \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_03)                                          \
  X(14, 6, 18, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_02,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_02)                                       \
  X(15, 6, 19, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_03,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_03)                                       \
  X(16, 6, 23, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_07,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_07)                                       \
  X(17, 6, 22, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_06,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_06)                                       \
  X(18, 6, 17, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_01,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_01)                                       \
  X(19, 6, 16, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_00,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_00)                                       \
  X(20, 6, 26, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_10,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_10)                                       \
  X(21, 6, 27, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_11,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_11)                                       \
  X(22, 6, 24, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_08,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_08)                                       \
  X(23, 6, 25, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_09,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_09)                                       \
  X(24, 6, 12, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_12,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_12)                                       \
  X(25, 6, 13, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B0_13,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B0_13)                                       \
  X(26, 6, 30, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_14,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_14)                                       \
  X(27, 6, 31, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_15,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_15)                                       \
  X(28, 8, 18, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_32,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_32)                                         \
  X(29, 9, 31, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_31,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_31)                                         \
  X(30, 8, 23, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_37,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_37)                                         \
  X(31, 8, 22, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_36,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_36)                                         \
  X(32, 7, 12, IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_12,                               \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B0_12)                                          \
  X(33, 9, 7, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_07,                               \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_07)                                         \
  X(34, 7, 29, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_13,                               \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_13)                                          \
  X(35, 7, 28, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_12,                               \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_12)                                          \
  X(36, 7, 18, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_02,                               \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_02)                                          \
  X(37, 7, 19, IOMUXC_SW_MUX_CTL_PAD_GPIO_B1_03,                               \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_B1_03)                                          \
  X(38, 6, 28, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_12,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_12)                                       \
  X(39, 6, 29, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_13,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_13)                                       \
  X(40, 6, 20, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_04,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_04)                                       \
  X(41, 6, 21, IOMUXC_SW_MUX_CTL_PAD_GPIO_AD_B1_05,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_AD_B1_05)                                       \
  X(42, 8, 15, IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B0_03,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B0_03)                                       \
  X(43, 8, 14, IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B0_02,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B0_02)                                       \
  X(44, 8, 13, IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B0_01,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B0_01)                                       \
  X(45, 8, 12, IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B0_00,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B0_00)                                       \
  X(46, 8, 17, IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B0_05,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B0_05)                                       \
  X(47, 8, 16, IOMUXC_SW_MUX_CTL_PAD_GPIO_SD_B0_04,                            \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_SD_B0_04)                                       \
  X(48, 9, 24, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_24,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_24)                                         \
  X(49, 9, 27, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_27,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_27)                                         \
  X(50, 9, 28, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_28,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_28)                                         \
  X(51, 9, 22, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_22,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_22)                                         \
  X(52, 9, 26, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_26,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_26)                                         \
  X(53, 9, 25, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_25,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_25)                                         \
  X(54, 9, 29, IOMUXC_SW_MUX_CTL_PAD_GPIO_EMC_29,                              \
    IOMUXC_SW_PAD_CTL_PAD_GPIO_EMC_29)

#endif
