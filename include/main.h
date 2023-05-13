/* main.h
*
*  MIT License
*
*  Copyright (c) 2023 awawa-dev
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

#ifndef MAIN_H
#define MAIN_H

#define MAX_BUFFER (3013 * 3 + 1)
#define HELLO_MESSAGE "\r\nWelcome!\r\nAwa driver 9.\r\n"

#include "calibration.h"
#include "statistics.h"
#include "base.h"
#include "framestate.h"

void updateMainStatistics(unsigned long currentTime, unsigned long deltaTime, bool hasData)
{
	if (hasData && deltaTime >= 1000 && deltaTime <= 1025 && statistics.getGoodFrames() > 3)
		statistics.update(currentTime);
	else if (deltaTime > 1025)
		statistics.lightReset(currentTime, hasData);
}

/**
 * @brief process received data on core 0
 *
 */
void processData()
{
	// update and print statistics
	unsigned long currentTime = millis();
	unsigned long deltaTime = currentTime - statistics.getStartTime();

	updateMainStatistics(currentTime, deltaTime, base.queueCurrent != base.queueEnd);

	// render waiting frame if available
	if (base.hasLateFrameToRender())
		base.renderLeds(false);

	// process received data
	while (base.queueCurrent != base.queueEnd)
	{
		uint8_t input = base.buffer[base.queueCurrent++];

		if (base.queueCurrent >= MAX_BUFFER)
		{
			base.queueCurrent = 0;
			yield();
		}

		switch (frameState.getState())
		{
		case AwaProtocol::HEADER_A:
			if (input == 'A')
				frameState.setState(AwaProtocol::HEADER_d);
			break;

		case AwaProtocol::HEADER_d:
			if (input == 'd')
				frameState.setState(AwaProtocol::HEADER_a);
			else
				frameState.setState(AwaProtocol::HEADER_A);
			break;

		case AwaProtocol::HEADER_a:
			if (input == 'a')
				frameState.setState(AwaProtocol::HEADER_HI);
			else
				frameState.setState(AwaProtocol::HEADER_A);
			break;

		case AwaProtocol::HEADER_HI:
			statistics.increaseTotal();
			frameState.init(input);
			frameState.setState(AwaProtocol::HEADER_LO);
			break;

		case AwaProtocol::HEADER_LO:
			frameState.computeCRC(input);
			frameState.setState(AwaProtocol::HEADER_CRC);
			break;

		case AwaProtocol::HEADER_CRC:
			// verify CRC and create/update LED driver if neccesery
			if (frameState.getCRC() == input)
			{
				uint16_t ledSize = frameState.getCount() + 1;

				// sanity check
				if (ledSize > 4096)
					frameState.setState(AwaProtocol::HEADER_A);
				else
				{
					if (ledSize != base.getLedsNumber())
						base.initLedStrip(ledSize);

					frameState.setState(AwaProtocol::RED);
				}
			}
			else
				frameState.setState(AwaProtocol::HEADER_A);
			break;

		case AwaProtocol::RED:
			frameState.color.R = input;
			frameState.addFletcher(input);

			frameState.setState(AwaProtocol::GREEN);
			break;

		case AwaProtocol::GREEN:
			frameState.color.G = input;
			frameState.addFletcher(input);

			frameState.setState(AwaProtocol::BLUE);
			break;

		case AwaProtocol::BLUE:
			frameState.color.B = input;
			frameState.addFletcher(input);

			#ifdef NEOPIXEL_RGBW
				// calculate RGBW from RGB using provided calibration data
				frameState.rgb2rgbw();
			#endif

			// set pixel, increase the index and check if it was the last LED color to come
			if (base.setStripPixel(frameState.getCurrentLedIndex(), frameState.color))
			{
				frameState.setState(AwaProtocol::RED);
			}
			else
			{
				frameState.setState(AwaProtocol::FINAL);
			}

			break;

		case AwaProtocol::FINAL:
			statistics.increaseGood();

			base.renderLeds(true);


			currentTime = millis();
			deltaTime = currentTime - statistics.getStartTime();
			updateMainStatistics(currentTime, deltaTime, true);

			yield();

			frameState.setState(AwaProtocol::HEADER_A);
			break;
		}
	}
}

#endif