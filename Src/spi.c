/*
  spi.c - SPI support for SD card & Trinamic plugins

  Part of grblHAL driver for STM32F7xx

  Copyright (c) 2020-2021 Terje Io

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "main.h"
#include "driver.h"

#define SPIport(p) SPIportI(p)
#define SPIportI(p) SPI ## p

#define SPIPORT SPIport(SPI_PORT)

static SPI_HandleTypeDef spi_port = {
    .Instance = SPIPORT,
    .Init.Mode = SPI_MODE_MASTER,
    .Init.Direction = SPI_DIRECTION_2LINES,
    .Init.DataSize = SPI_DATASIZE_8BIT,
    .Init.CLKPolarity = SPI_POLARITY_LOW,
    .Init.CLKPhase = SPI_PHASE_1EDGE,
    .Init.NSS = SPI_NSS_SOFT,
    .Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256,
    .Init.FirstBit = SPI_FIRSTBIT_MSB,
    .Init.TIMode = SPI_TIMODE_DISABLE,
    .Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE,
    .Init.CRCPolynomial = 10
};

void spi_init (void)
{
    static bool init = false;

    // Pin order: SCK|MISO|MOSI

    if(!init) {

#if SPI_PORT == 1
        __HAL_RCC_SPI1_CLK_ENABLE();

        GPIO_InitTypeDef GPIO_InitStruct = {
            .Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
            .Alternate = GPIO_AF5_SPI1,
        };
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        static const periph_pin_t sck = {
            .function = Output_SCK,
            .group = PinGroup_SPI,
            .port = GPIOA,
            .pin = 5,
            .mode = { .mask = PINMODE_OUTPUT }
        };

        static const periph_pin_t sdo = {
            .function = Output_MOSI,
            .group = PinGroup_SPI,
            .port = GPIOA,
            .pin = 6,
            .mode = { .mask = PINMODE_NONE }
        };

        static const periph_pin_t sdi = {
            .function = Input_MISO,
            .group = PinGroup_SPI,
            .port = GPIOA,
            .pin = 7,
            .mode = { .mask = PINMODE_NONE }
        };
#endif
#if SPI_PORT == 2
        __HAL_RCC_SPI2_CLK_ENABLE();

        GPIO_InitTypeDef GPIO_InitStruct = {
            .Pin = GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
            .Alternate = GPIO_AF5_SPI2,
        };
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        static const periph_pin_t sck = {
            .function = Output_SCK,
            .group = PinGroup_SPI,
            .port = GPIOB,
            .pin = 13,
            .mode = { .mask = PINMODE_OUTPUT }
        };

        static const periph_pin_t sdo = {
            .function = Output_MOSI,
            .group = PinGroup_SPI,
            .port = GPIOB,
            .pin = 14,
            .mode = { .mask = PINMODE_NONE }
        };

        static const periph_pin_t sdi = {
            .function = Input_MISO,
            .group = PinGroup_SPI,
            .port = GPIOB,
            .pin = 15,
            .mode = { .mask = PINMODE_NONE }
        };
#endif
#if SPI_PORT == 3
        __HAL_RCC_SPI3_CLK_ENABLE();

        GPIO_InitTypeDef GPIO_InitStruct = {
            .Pin = GPIO_PIN_10|GPIO_PIN_11|GPIO_PIN_12,
            .Mode = GPIO_MODE_AF_PP,
            .Pull = GPIO_NOPULL,
            .Speed = GPIO_SPEED_FREQ_VERY_HIGH,
            .Alternate = GPIO_AF6_SPI3
        };
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        static const periph_pin_t sck = {
            .function = Output_SCK,
            .group = PinGroup_SPI,
            .port = GPIOC,
            .pin = 10,
            .mode = { .mask = PINMODE_OUTPUT }
        };

        static const periph_pin_t sdo = {
            .function = Output_MOSI,
            .group = PinGroup_SPI,
            .port = GPIOC,
            .pin = 11,
            .mode = { .mask = PINMODE_NONE }
        };

        static const periph_pin_t sdi = {
            .function = Input_MISO,
            .group = PinGroup_SPI,
            .port = GPIOC,
            .pin = 12,
            .mode = { .mask = PINMODE_NONE }
        };
#endif

        HAL_SPI_Init(&spi_port);
        __HAL_SPI_ENABLE(&spi_port);

        hal.delay_ms(2, NULL);

        hal.periph_port.register_pin(&sck);
        hal.periph_port.register_pin(&sdo);
        hal.periph_port.register_pin(&sdi);

        init = true;
    }
}

// set the SSI speed to the max setting
void spi_set_max_speed (void)
{
    spi_port.Instance->CR1 &= ~SPI_BAUDRATEPRESCALER_256;
    spi_port.Instance->CR1 |= SPI_BAUDRATEPRESCALER_16; // should be able to go to 12Mhz...
}

uint32_t spi_set_speed (uint32_t prescaler)
{
    uint32_t cur = spi_port.Instance->CR1 & SPI_BAUDRATEPRESCALER_256;

    spi_port.Instance->CR1 &= ~SPI_BAUDRATEPRESCALER_256;
    spi_port.Instance->CR1 |= prescaler;

    return cur;
}

uint8_t spi_get_byte (void)
{
    *((__IO uint8_t *)&SPIPORT->DR) = 0xFF; // Writing dummy data into Data register

    while(!__HAL_SPI_GET_FLAG(&spi_port, SPI_FLAG_RXNE));

    return (uint8_t)spi_port.Instance->DR;
}

uint8_t spi_put_byte (uint8_t byte)
{
    *((__IO uint8_t *)&SPIPORT->DR) = byte;

    while(!__HAL_SPI_GET_FLAG(&spi_port, SPI_FLAG_TXE));
    while(!__HAL_SPI_GET_FLAG(&spi_port, SPI_FLAG_RXNE));

    __HAL_SPI_CLEAR_OVRFLAG(&spi_port);

    return (uint8_t)spi_port.Instance->DR;
}
