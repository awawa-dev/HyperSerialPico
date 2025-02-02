#pragma once

/* leds.h
*
*  MIT License
*
*  Copyright (c) 2023-2025 awawa-dev
*
*  https://github.com/awawa-dev/HyperSerialPico
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  The above copyright notice and this permission notice shall be included in all
*  copies or substantial portions of the Software.

*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
 */

/*
	HyperSerialPico led (aka PicoLada) library features:
	- neopixel (rgb: ws2812b, ws2813..., rgbw: sk6812b) and dotstar (rgb: apa102, hd107, sk9822...) led strip support
	- single and up to 8 lines parallel (neopixel) mode
	- DMA
	- PIO neopixel hardware processing
	- using LUT tables for preparing PIO DMA parallel buffer
	- SPI dotstar hardware support
	- non-blocking rendering (check isReady if it's finished)

	Usage for sk6812 rgbw single lane:
	ledStrip1 = new sk6812(ledsNumber, DATA_PIN);
	ledStrip1->SetPixel(index, ColorGrbw(255));
	ledStrip1->renderSingleLane();

	Usage for ws2812 rgb single lane:
	ledStrip1 = new ws2812(ledsNumber, DATA_PIN);
	ledStrip1->SetPixel(index, ColorGrb32(255));
	ledStrip1->renderSingleLane();

	Usage for sk6812 rgbw multi lanes:
	ledStrip1 = new sk6812p(ledsNumber, DATA_PIN); // using DATA_PIN output
	ledStrip2 = new sk6812p(ledsNumber, DATA_PIN); // using DATA_PIN + 1 output
	ledStrip1->SetPixel(index, ColorGrbw(255));
	ledStrip2->SetPixel(index, ColorGrbw(255));
	ledStrip1->renderAllLanes(); // renders ledStrip1 and ledStrip2 simoultaneusly

	Usage for ws2812 rgb multi lanes:
	ledStrip1 = new ws2812p(ledsNumber, DATA_PIN); // using DATA_PIN output
	ledStrip2 = new ws2812p(ledsNumber, DATA_PIN); // using DATA_PIN + 1 output
	ledStrip1->SetPixel(index, ColorGrb(255));
	ledStrip2->SetPixel(index, ColorGrb(255));
	ledStrip1->renderAllLanes(); // renders ledStrip1 and ledStrip2 simoultaneusly

	Usage for dotstar rgb single line:
	ledStrip1 = new apa102(ledsNumber, DATA_PIN, CLOCK_PIN);
	ledStrip1->SetPixel(index, ColorDotstartBgr(255));
	ledStrip1->renderSingleLane();
*/

#include <hardware/spi.h>
#include <hardware/dma.h>
#include <hardware/clocks.h>
#include <neopixel.pio.h>
#include <neopixel_ws2812b.pio.h>
#include <pico/stdlib.h>
#include <pico/binary_info.h>
#include <algorithm>
#include <string.h>

struct ColorGrb32
{
	uint8_t notUsed;
	uint8_t B;
	uint8_t R;
	uint8_t G;

	ColorGrb32(uint8_t gray) :
		R(gray), G(gray), B(gray)
	{
	};

	ColorGrb32() : R(0), G(0), B(0)
	{
	};

	static bool isAlignedTo24()
	{
		return true;
	};
};

struct ColorGrb
{
	uint8_t B;
	uint8_t R;
	uint8_t G;


	ColorGrb(uint8_t gray) :
		R(gray), G(gray), B(gray)
	{
	};

	ColorGrb() : R(0), G(0), B(0)
	{
	};
};

struct ColorGrbw
{
	uint8_t W;
	uint8_t B;
	uint8_t R;
	uint8_t G;

	ColorGrbw(uint8_t gray) :
		R(gray), G(gray), B(gray), W(gray)
	{
	};

	ColorGrbw() : R(0), G(0), B(0), W(0)
	{
	};

	static bool isAlignedTo24()
	{
		return false;
	};
};

struct ColorDotstartBgr
{
	uint8_t brightness;
	uint8_t B;
	uint8_t G;
	uint8_t R;

	ColorDotstartBgr(uint8_t gray) :
		R(gray), G(gray), B(gray), brightness(gray | 0b11100000)
	{
	};

	ColorDotstartBgr() : R(0), G(0), B(0), brightness(0xff)
	{
	};
};

class LedDriver
{
	protected:

	int ledsNumber;
	int pin;
	int clockPin;
	int dmaSize;
	uint8_t* buffer;
	uint8_t* dma;

	public:

	LedDriver(int _ledsNumber, int _pin, int _dmaSize): LedDriver(_ledsNumber, _pin, 0, _dmaSize)
	{

	}

	LedDriver(int _ledsNumber, int _pin, int _clockPin, int _dmaSize)
	{
		LedDriverDmaReceiver = this;
		ledsNumber = _ledsNumber;
		pin = _pin;
		clockPin = _clockPin;
		dmaSize = _dmaSize;
		if (dmaSize % 4)
			dmaSize += (4 - (_dmaSize % 4));
		buffer = reinterpret_cast<uint8_t*>(calloc(dmaSize, 1));
		dma = reinterpret_cast<uint8_t*>(calloc(dmaSize, 1));
	}

	~LedDriver()
	{
		free(buffer);
		free(dma);
		if (LedDriverDmaReceiver == this)
			LedDriverDmaReceiver = nullptr;
	}

	static LedDriver* LedDriverDmaReceiver;
};

LedDriver* LedDriver::LedDriverDmaReceiver = nullptr;

class DmaClient
{
	protected:

	PIO selectedPIO;
	uint stateIndex;

	static uint PICO_DMA_CHANNEL;
	static volatile uint64_t lastRenderTime;
	static volatile bool isDmaBusy;


	DmaClient()
	{
		PICO_DMA_CHANNEL = dma_claim_unused_channel(true);
		isDmaBusy = false;
		lastRenderTime = 0;
	};

	~DmaClient()
	{
		for(int i = 0; i < 10 && isDmaBusy; i++)
			busy_wait_us(500);

		dma_channel_abort(PICO_DMA_CHANNEL);
		dma_channel_set_irq0_enabled(PICO_DMA_CHANNEL, false);
		irq_set_enabled(DMA_IRQ_0, false);

		dma_channel_unclaim(PICO_DMA_CHANNEL);
	};

	void dmaConfigure(PIO _selectedPIO, uint _sm)
	{
		selectedPIO = _selectedPIO;
		stateIndex = _sm;
	};

	void initDmaPio(uint dataLenDword32)
	{
		dma_channel_config dmaConfig = dma_channel_get_default_config(PICO_DMA_CHANNEL);
		channel_config_set_dreq(&dmaConfig, pio_get_dreq(selectedPIO, stateIndex, true));
		channel_config_set_transfer_data_size(&dmaConfig, DMA_SIZE_32);
		channel_config_set_read_increment(&dmaConfig, true);
		dma_channel_configure(PICO_DMA_CHANNEL, &dmaConfig, &selectedPIO->txf[stateIndex], NULL, dataLenDword32, false);

		assignDmaIrq();
	};

	void initDmaSpi(spi_inst_t* _spi, uint dataLenByte8)
	{
		dma_channel_config dmaConfig = dma_channel_get_default_config(PICO_DMA_CHANNEL);
		channel_config_set_transfer_data_size(&dmaConfig, DMA_SIZE_8);
		channel_config_set_dreq(&dmaConfig, spi_get_dreq(_spi, true));
		dma_channel_configure(PICO_DMA_CHANNEL, &dmaConfig,&spi_get_hw(_spi)->dr, NULL, dataLenByte8, false);

		assignDmaIrq();
	};

	void assignDmaIrq()
	{
		irq_set_exclusive_handler(DMA_IRQ_0, dmaFinishReceiver);
		dma_channel_set_irq0_enabled(PICO_DMA_CHANNEL, true);
		irq_set_enabled(DMA_IRQ_0, true);
	};

	public:

	bool isReadyBlocking()
	{
		int wait = 200;
		while(isDmaBusy && wait-- > 0)
			busy_wait_us(50);

		return !isDmaBusy;
	}

	bool isReady()
	{
		return !isDmaBusy;
	}

	static void dmaFinishReceiver()
	{
		if (dma_hw->ints0 & (1u<<DmaClient::PICO_DMA_CHANNEL))
		{
			dma_hw->ints0 = (1u<<DmaClient::PICO_DMA_CHANNEL);

			lastRenderTime = time_us_64();
			isDmaBusy = false;
		}
	}
};

enum class NeopixelSubtype {ws2812b, sk6812};

class Neopixel : public LedDriver, public DmaClient
{

	uint64_t resetTime;

	friend class NeopixelParallel;

	public:
	Neopixel(NeopixelSubtype timingType, int lanes, uint64_t _resetTime, int _ledsNumber, int _pin, int _dmaSize, bool alignTo24 = false):
			LedDriver(_ledsNumber, _pin, _dmaSize)
	{
		pio_sm_config smConfig;
		uint programAddress;

		dmaConfigure(pio0, 0);
		resetTime = _resetTime;

		if (lanes >= 1)
		{
			programAddress = (timingType == NeopixelSubtype::ws2812b) ?
				pio_add_program(selectedPIO,  &neopixel_ws2812b_parallel_program) : pio_add_program(selectedPIO,  &neopixel_parallel_program);

			for(uint i=_pin; i<_pin + lanes; i++){
				pio_gpio_init(selectedPIO, i);
			}

			smConfig = (timingType == NeopixelSubtype::ws2812b) ?
				neopixel_ws2812b_parallel_program_get_default_config(programAddress) : neopixel_parallel_program_get_default_config(programAddress);

			sm_config_set_out_pins(&smConfig, _pin, lanes);
			sm_config_set_set_pins(&smConfig, _pin, lanes);
		}
		else
		{
			programAddress = (timingType == NeopixelSubtype::ws2812b) ?
				pio_add_program(selectedPIO,  &neopixel_ws2812b_program) : pio_add_program(selectedPIO,  &neopixel_program);

			pio_gpio_init(selectedPIO, _pin);

			smConfig = (timingType == NeopixelSubtype::ws2812b) ?
				neopixel_ws2812b_program_get_default_config(programAddress) : neopixel_program_get_default_config(programAddress);

			sm_config_set_sideset_pins(&smConfig, _pin);
		}

		pio_sm_set_consecutive_pindirs(selectedPIO, stateIndex, _pin, std::max(lanes, 1), true);
		sm_config_set_out_shift(&smConfig, false, true, (alignTo24) ? 24: 32);
		sm_config_set_fifo_join(&smConfig, PIO_FIFO_JOIN_TX);
		float div = clock_get_hz(clk_sys) / (800000 * 12);
		sm_config_set_clkdiv(&smConfig, div);
		pio_sm_init(selectedPIO, stateIndex, programAddress, &smConfig);
		pio_sm_set_enabled(selectedPIO, stateIndex, true);

		initDmaPio(dmaSize / 4);
	}

	uint8_t* getBufferMemory()
	{
		return buffer;
	}

	protected:

	void renderDma(bool resetBuffer)
	{
		if (isDmaBusy)
			return;

		isDmaBusy = true;

		uint64_t currentTime = time_us_64();
		if (currentTime < resetTime + lastRenderTime)
			busy_wait_us(std::min(resetTime + lastRenderTime - currentTime, resetTime));

		memcpy(dma, buffer, dmaSize);

		dma_channel_set_read_addr(PICO_DMA_CHANNEL, dma, true);

		if (resetBuffer)
			memset(buffer, 0, dmaSize);
	}
};

template<NeopixelSubtype _type, int RESET_TIME, typename colorData>
class NeopixelType : public Neopixel
{
	public:

	NeopixelType(int _ledsNumber, int _pin) :
		Neopixel(_type, 0, RESET_TIME, _ledsNumber, _pin, _ledsNumber * sizeof(colorData), colorData::isAlignedTo24())
	{
	}

	void SetPixel(int index, colorData color)
	{
		if (index >= ledsNumber)
			return;

		*(reinterpret_cast<colorData*>(buffer)+index) = color;
	}

	void renderSingleLane()
	{
		renderDma(false);
	}
};


class NeopixelParallel
{
	static Neopixel *muxer;
	static int instances;

	protected:
	static int maxLeds;
	const uint8_t myLaneMask;
	static uint8_t* buffer;

	public:

	NeopixelParallel(NeopixelSubtype _type, size_t pixelSize, uint64_t _resetTime, int _ledsNumber, int _pin):
					myLaneMask(1 << (instances++))
	{
		maxLeds = std::max(maxLeds, _ledsNumber);

		delete muxer;
		muxer = new Neopixel(_type, instances, _resetTime, maxLeds, _pin, maxLeds * 8 * pixelSize );
		buffer = muxer->getBufferMemory();
	}

	~NeopixelParallel()
	{
		if (instances > 0)
			instances--;

		if (instances == 0)
		{
			delete muxer;
			muxer = nullptr;
			buffer = nullptr;
			maxLeds = 0;
		}
	}

	bool isReadyBlocking()
	{
		return muxer->isReadyBlocking();
	}

	bool isReady()
	{
		return muxer->isReady();
	}

	void renderAllLanes()
	{
		muxer->renderDma(true);
	}
};

template<NeopixelSubtype _type, int RESET_TIME, typename colorData>
class NeopixelParallelType : public NeopixelParallel
{
	uint32_t lut[16];

	public:

	NeopixelParallelType(int _ledsNumber, int _basePinForLanes) :
		NeopixelParallel(_type, sizeof(colorData), RESET_TIME, _ledsNumber, _basePinForLanes)
	{
		for (uint8_t a = 0; a < 16; a++)
		{
			uint8_t* target = reinterpret_cast<uint8_t*>(&(lut[a]));
			for (uint8_t b = 0; b < 4; b++)
				*(target++) = (uint8_t) ((a & (0b00000001 << b)) ? myLaneMask : 0);
		}
	}

	void SetPixel(int index, colorData color)
	{
		if (index >= maxLeds)
			return;

		uint8_t* source = reinterpret_cast<uint8_t*>(&color);
		uint32_t* target = reinterpret_cast<uint32_t*>(&(buffer[(index + 1) * 8 * sizeof(colorData)]));

		for(int i = 0; i < sizeof(colorData); i++)
		{
			*(--target) |= lut[ *(source) & 0b00001111];
			*(--target) |= lut[ *(source++) >> 4];
		}
	}
};

class Dotstar : public LedDriver, public DmaClient
{
	uint64_t resetTime;

	friend class NeopixelParallel;

	public:
	Dotstar(uint64_t _resetTime, int _ledsNumber, spi_inst_t* _spi, uint32_t _datapin, uint32_t _clockpin, int _dmaSize):
			LedDriver(_ledsNumber, _datapin, _clockpin, _dmaSize)
	{
		dmaConfigure(pio0, 0);
		resetTime = _resetTime;

		spi_init(_spi, 10000000);
		gpio_set_function(_clockpin, GPIO_FUNC_SPI);
		gpio_set_function(_datapin, GPIO_FUNC_SPI);
		bi_decl(bi_4pins_with_func(PICO_DEFAULT_SPI_RX_PIN, _datapin, _clockpin, PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI));

		initDmaSpi(_spi, _dmaSize);
	}

	uint8_t* getBufferMemory()
	{
		return buffer;
	}

	protected:

	void renderDma()
	{
		if (isDmaBusy)
			return;

		isDmaBusy = true;

		uint64_t currentTime = time_us_64();
		if (currentTime < resetTime + lastRenderTime)
			busy_wait_us(std::min(resetTime + lastRenderTime - currentTime, resetTime));

		memcpy(dma, buffer, dmaSize);

		dma_channel_set_read_addr(PICO_DMA_CHANNEL, dma, true);
	}
};

template<int RESET_TIME, typename colorData>
class DotstarType : public Dotstar
{
	public:

	DotstarType(int _ledsNumber, spi_inst_t* _spi, int _dataPin, int _clockPin) :
		Dotstar(RESET_TIME, _ledsNumber, _spi, _dataPin, _clockPin, (_ledsNumber + 2) * sizeof(colorData))
	{
	}

	void SetPixel(int index, colorData color)
	{
		if (index >= ledsNumber)
			return;

		*(reinterpret_cast<colorData*>(buffer)+index+1) = color;
	}

	void renderSingleLane()
	{
		memset(buffer,0 ,4);
		*(reinterpret_cast<colorData*>(buffer)+ledsNumber+1) = colorData(0xff);
		renderDma();
	}
};

Neopixel* NeopixelParallel::muxer = nullptr;
uint8_t* NeopixelParallel::buffer = nullptr;
int NeopixelParallel::instances = 0;
int NeopixelParallel::maxLeds = 0;
uint DmaClient::PICO_DMA_CHANNEL = 0;
volatile uint64_t DmaClient::lastRenderTime = 0;
volatile bool DmaClient::isDmaBusy = false;


// API classes
typedef NeopixelType<NeopixelSubtype::ws2812b, 650, ColorGrb32> ws2812;
typedef NeopixelType<NeopixelSubtype::sk6812, 450, ColorGrbw> sk6812;
typedef NeopixelParallelType<NeopixelSubtype::ws2812b, 300, ColorGrb> ws2812p;
typedef NeopixelParallelType<NeopixelSubtype::sk6812, 80, ColorGrbw> sk6812p;
typedef DotstarType<100, ColorDotstartBgr> apa102;
