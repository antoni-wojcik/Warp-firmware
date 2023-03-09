#include <stdint.h>

/*
 *	config.h needs to come first
 */
#include "config.h"

#include "fsl_spi_master_driver.h"
#include "fsl_port_hal.h"

#include "SEGGER_RTT.h"
#include "gpio_pins.h"
#include "warp.h"
#include "devSSD1331.h"

#define CHAR_WIDTH	 7
#define CHAR_HEIGHT  9

volatile uint8_t	inBuffer[32];
volatile uint8_t	payloadBytes[32];


/*
 *	Override Warp firmware's use of these pins and define new aliases.
 */
enum
{
	kSSD1331PinMOSI		= GPIO_MAKE_PIN(HW_GPIOA, 8),
	kSSD1331PinSCK		= GPIO_MAKE_PIN(HW_GPIOA, 9),
	kSSD1331PinCSn		= GPIO_MAKE_PIN(HW_GPIOB, 13),
	kSSD1331PinDC		= GPIO_MAKE_PIN(HW_GPIOA, 12),
	kSSD1331PinRST		= GPIO_MAKE_PIN(HW_GPIOB, 0),
};

static int
writeCommand(uint8_t commandByte)
{
	spi_status_t status;

	/*
	 *	Drive /CS low.
	 *
	 *	Make sure there is a high-to-low transition by first driving high, delay, then drive low.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);
	OSA_TimeDelay(10);
	GPIO_DRV_ClearPinOutput(kSSD1331PinCSn);

	/*
	 *	Drive DC low (command).
	 */
	GPIO_DRV_ClearPinOutput(kSSD1331PinDC);

	payloadBytes[0] = commandByte;
	status = SPI_DRV_MasterTransferBlocking(0	/* master instance */,
					NULL		/* spi_master_user_config_t */,
					(const uint8_t * restrict)&payloadBytes[0],
					(uint8_t * restrict)&inBuffer[0],
					1		/* transfer size */,
					1000		/* timeout in microseconds (unlike I2C which is ms) */);

	/*
	 *	Drive /CS high
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinCSn);

	return status;
}

void
clearScreen(void)
{
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(0x00);
    writeCommand(0x00);
    writeCommand(0x5F);
    writeCommand(0x3F);
}

void
clearRegion(uint8_t s_x, uint8_t s_y, uint8_t w, uint8_t h)
{
	writeCommand(kSSD1331CommandCLEAR);
	writeCommand(s_x);
    writeCommand(s_y);
    writeCommand(s_x + w);
    writeCommand(s_y + h);
}

void
drawLine(uint8_t s_x, uint8_t s_y, uint8_t e_x, uint8_t e_y, uint8_t g_x, uint8_t g_y, SSD1331Colors color)
{
	uint8_t col_r = (color >> 16) & 0xFF;
	uint8_t col_g = (color >>  8) & 0xFF;
	uint8_t col_b = (color      ) & 0xFF;

	writeCommand(kSSD1331CommandDRAWLINE);
	writeCommand(g_x + s_x);
    writeCommand(g_y + s_y);
    writeCommand(g_x + e_x);
    writeCommand(g_y + e_y);
    writeCommand(col_r);
    writeCommand(col_g);
    writeCommand(col_b);
}

/* 7 x 9 characters */
void
drawDigit(uint8_t digit, uint8_t x, uint8_t y, SSD1331Colors color)
{
	//clearRegion(x, y, CHAR_WIDTH - 1, CHAR_HEIGHT - 1);

	switch(digit)
	{
		case 0:
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			drawLine(0, 0, 6, 8, x, y, color);
			break;
		}
		case 1:
		{
			drawLine(3, 0, 3, 8, x, y, color);
			break;
		}
		case 2:
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(6, 0, 6, 4, x, y, color);
			drawLine(6, 4, 0, 4, x, y, color);
			drawLine(0, 4, 0, 8, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			break;
		}
		case 3:
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			drawLine(0, 4, 6, 4, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			break;
		}
		case 4:
		{
			drawLine(0, 0, 0, 4, x, y, color);
			drawLine(0, 4, 6, 4, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			break;
		}
		case 5:
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 0, 0, 4, x, y, color);
			drawLine(0, 4, 6, 4, x, y, color);
			drawLine(6, 4, 6, 8, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			break;
		}
		case 6:
		{
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(6, 4, 6, 8, x, y, color);
			drawLine(6, 4, 0, 4, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			break;
		}
		case 7:
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			break;
		}
		case 8:
		{
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			drawLine(0, 4, 6, 4, x, y, color);
			break;
		}
		case 9:
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			drawLine(6, 4, 0, 4, x, y, color);
			drawLine(0, 0, 0, 4, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			break;
		}
		default:
		{
			warpPrint("ERROR: SSD1331: drawDigit");
		}
	}
}

/* 7 x 9 characters */
void
drawChar(char character, uint8_t x, uint8_t y, SSD1331Colors color)
{
	//clearRegion(x, y, CHAR_WIDTH - 1, CHAR_HEIGHT - 1);

	switch(character)
	{
		case 'a':
		case 'A':
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(0, 4, 6, 4, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			break;
		}
		case 'b':
		case 'B':
		{
        	drawLine(0, 0, 4, 0, x, y, color);
        	drawLine(4, 0, 4, 3, x, y, color);
        	drawLine(0, 0, 0, 8, x, y, color);
        	drawLine(0, 3, 6, 3, x, y, color);
        	drawLine(6, 3, 6, 8, x, y, color);
        	drawLine(0, 8, 6, 8, x, y, color);
			break;
		}
		case 'c':
		case 'C':
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			break;
		}
		case 'd':
		case 'D':
		{
			drawLine(0, 0, 5, 0, x, y, color);
			drawLine(0, 8, 5, 8, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(6, 1, 6, 7, x, y, color);
			break;
		}
		case 'e':
		case 'E':
		{
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			drawLine(0, 4, 4, 4, x, y, color);
			break;
		}
		case 'f':
		case 'F':
		{
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 4, 4, 4, x, y, color);
			break;
		}		
		case 'g':
		case 'G':
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			drawLine(6, 4, 6, 8, x, y, color);
			drawLine(3, 4, 6, 4, x, y, color);
			break;
		}
		case 'h':
		case 'H':
		{
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(0, 4, 6, 4, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			break;
		}
		case 'i':
		case 'I':
		{
			drawLine(3, 0, 3, 8, x, y, color);
			break;
		}
		case 'j':
		case 'J':
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 6, 0, 8, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			break;
		}
		case 'k':
		case 'K':
		{
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(1, 4, 6, 0, x, y, color);
			drawLine(1, 4, 6, 8, x, y, color);
			break;
		}
		case 'l':
		case 'L':
		{
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			break;
		}
		case 'm':
		case 'M':
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			drawLine(3, 0, 3, 8, x, y, color);
			break;
		}
		case 'n':
		case 'N':
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			break;
		}
		case 'o':
		case 'O':
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			break;
		}
		case 'p':
		case 'P':
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(6, 4, 0, 4, x, y, color);
			drawLine(6, 0, 6, 4, x, y, color);
			break;
		}
		case 'q':
		case 'Q':
		{
			drawLine(0, 0, 5, 0, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			drawLine(5, 0, 5, 8, x, y, color);
			break;
		}
		case 'r':
		case 'R':
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(6, 4, 0, 4, x, y, color);
			drawLine(6, 0, 6, 4, x, y, color);
			drawLine(1, 5, 6, 8, x, y, color);
			break;
		}
		case 's':
		case 'S':
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(0, 0, 0, 4, x, y, color);
			drawLine(0, 4, 6, 4, x, y, color);
			drawLine(6, 4, 6, 8, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			break;
		}
		case 't':
		case 'T':
		{
			drawLine(3, 0, 3, 8, x, y, color);
			drawLine(0, 0, 6, 0, x, y, color);
			break;
		}
		case 'u':
		case 'U':
		{
			drawLine(0, 8, 6, 8, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			break;
		}
		case 'w':
		case 'W':
		{
			drawLine(0, 8, 6, 8, x, y, color);
			drawLine(0, 0, 0, 8, x, y, color);
			drawLine(6, 0, 6, 8, x, y, color);
			drawLine(3, 0, 3, 8, x, y, color);
			break;
		}
		case 'x':
		case 'X':
		{
			drawLine(0, 0, 6, 8, x, y, color);
			drawLine(6, 0, 0, 8, x, y, color);
			break;
		}
		case 'y':
		case 'Y':
		{
			drawLine(0, 0, 3, 5, x, y, color);
			drawLine(6, 0, 3, 5, x, y, color);
			drawLine(3, 5, 3, 8, x, y, color);
			break;
		}
		case 'z':
		case 'Z':
		{
			drawLine(0, 0, 6, 0, x, y, color);
			drawLine(6, 1, 0, 7, x, y, color);
			drawLine(0, 8, 6, 8, x, y, color);
			break;
		}
		case '.':
		{
			drawLine(0, 8, 0, 8, x, y, color);
		}
		case ':':
		{
			drawLine(0, 3, 0, 3, x, y, color);
			drawLine(0, 5, 0, 5, x, y, color);
			break;
		}
		case '%':
		{
			drawLine(0, 0, 0, 0, x, y, color);
			drawLine(0, 0, 6, 8, x, y, color);
			drawLine(6, 0, 0, 8, x, y, color);
			break;
		}
		case ' ':
		{
			break;
		}
		default:
		{
			warpPrint("ERROR: SSD1331: drawChar");
		}
	}
}

uint8_t
drawText(const char* text, uint8_t size, uint8_t x, uint8_t y, SSD1331Colors color)
{
	uint8_t x_offset = 0;

	for(uint8_t i = 0; i < size; i++)
	{
		drawChar(text[i], x + x_offset, y, color);

		switch(text[i])
		{
			case '.':
			case ':':
			{
				x_offset += 2;
			}
			default:
			{
				x_offset += CHAR_WIDTH + 1;
			}
		}
	}

	return x_offset;
}

// draw porbabilty with the given precision. Assume prob <= 1
uint8_t
drawProb(double prob, uint8_t x, uint8_t y, SSD1331Colors color)
{
	uint8_t x_offset = 0;

	uint8_t precision = 3;

	uint8_t digit;

	for(uint8_t i = 0; i < precision; i++)
	{
		digit = (uint8_t)(prob) % 10;

		prob *= 10.0;

		if(i == 0 && digit != 1)
		{
			continue;
		}

		drawDigit(digit, x + x_offset, y, color);

		x_offset += CHAR_WIDTH + 1;

	}

	drawChar('%', x + x_offset, y, color);

	x_offset += CHAR_WIDTH + 1;

	return x_offset;
}

void
drawRect(void)
{
	writeCommand(kSSD1331CommandDRAWRECT);
	writeCommand(0x00); // set column address of start
	writeCommand(0x00); // set row address of start
	writeCommand(0x5F); // set column address of end
	writeCommand(0x3F); // set row address of end
	writeCommand(0x00); // R intensity - outline
	writeCommand(0xFF); // G intensity - outline
	writeCommand(0x00); // B intensity - outline
	writeCommand(0x00); // R intensity - fill
	writeCommand(0xFF); // G intensity - fill
	writeCommand(0x00); // B intensity - fill
}

int
devSSD1331init(void)
{
	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Re-configure SPI to be on PTA8 and PTA9 for MOSI and SCK respectively.
	 */
	PORT_HAL_SetMuxMode(PORTA_BASE, 8u, kPortMuxAlt3);
	PORT_HAL_SetMuxMode(PORTA_BASE, 9u, kPortMuxAlt3);

	warpEnableSPIpins();

	/*
	 *	Override Warp firmware's use of these pins.
	 *
	 *	Reconfigure to use as GPIO.
	 */
	PORT_HAL_SetMuxMode(PORTB_BASE, 13u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTA_BASE, 12u, kPortMuxAsGpio);
	PORT_HAL_SetMuxMode(PORTB_BASE, 0u, kPortMuxAsGpio);


	/*
	 *	RST high->low->high.
	 */
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_ClearPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);
	GPIO_DRV_SetPinOutput(kSSD1331PinRST);
	OSA_TimeDelay(100);

		/*
	 *	Initialization sequence, borrowed from https://github.com/adafruit/Adafruit-SSD1331-OLED-Driver-Library-for-Arduino
	 */
	writeCommand(kSSD1331CommandDISPLAYOFF);	// 0xAE
	writeCommand(kSSD1331CommandSETREMAP);		// 0xA0
	writeCommand(0x72);				// RGB Color
	writeCommand(kSSD1331CommandSTARTLINE);		// 0xA1
	writeCommand(0x0);
	writeCommand(kSSD1331CommandDISPLAYOFFSET);	// 0xA2
	writeCommand(0x0);
	writeCommand(kSSD1331CommandNORMALDISPLAY);	// 0xA4
	writeCommand(kSSD1331CommandSETMULTIPLEX);	// 0xA8
	writeCommand(0x3F);				// 0x3F 1/64 duty
	writeCommand(kSSD1331CommandSETMASTER);		// 0xAD
	writeCommand(0x8E);
	writeCommand(kSSD1331CommandPOWERMODE);		// 0xB0
	writeCommand(0x0B);
	writeCommand(kSSD1331CommandPRECHARGE);		// 0xB1
	writeCommand(0x31);
	writeCommand(kSSD1331CommandCLOCKDIV);		// 0xB3
	writeCommand(0xF0);				// 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8A
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGEB);	// 0x8B
	writeCommand(0x78);
	writeCommand(kSSD1331CommandPRECHARGEA);	// 0x8C
	writeCommand(0x64);
	writeCommand(kSSD1331CommandPRECHARGELEVEL);	// 0xBB
	writeCommand(0x3A);
	writeCommand(kSSD1331CommandVCOMH);		// 0xBE
	writeCommand(0x3E);
	writeCommand(kSSD1331CommandMASTERCURRENT);	// 0x87
	writeCommand(0x06);
	writeCommand(kSSD1331CommandCONTRASTA);		// 0x81
	writeCommand(0x91);
	writeCommand(kSSD1331CommandCONTRASTB);		// 0x82
	writeCommand(0x50);
	writeCommand(kSSD1331CommandCONTRASTC);		// 0x83
	writeCommand(0x7D);
	writeCommand(kSSD1331CommandDISPLAYON);		// Turn on oled panel
//	SEGGER_RTT_WriteString(0, "\r\n\tDone with initialization sequence...\n");

	/*
	 *	To use fill commands, you will have to issue a command to the display to enable them. See the manual.
	 */
	writeCommand(kSSD1331CommandFILL);
	writeCommand(0x01);
//	SEGGER_RTT_WriteString(0, "\r\n\tDone with enabling fill...\n");

	/*
	 *	Clear Screen
	 */
	clearScreen();

	drawRect();

	/*for(int i = 0; i <= 9; i++) {
		drawDigit(i, i * (CHAR_WIDTH + 1), 0, kSSD1331ColorWHITE);
	}*/

	


//	SEGGER_RTT_WriteString(0, "\r\n\tDone with draw rectangle...\n");



	return 0;
}
