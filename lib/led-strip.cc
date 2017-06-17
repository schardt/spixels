// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-
// SPI Pixels - Control SPI LED strips (spixels)
// Copyright (C) 2016 Henner Zeller <h.zeller@acm.org>
//
// This library is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "multi-spi.h"
#include "led-strip.h"

#define DO_CIE1931		FALSE


/*
typedef uint16_t CIEValue;

// Do CIE1931 luminance correction and scale to maximum expected output bits.
static CIEValue luminance_cie1931_internal(uint8_t c, uint8_t brightness) {
    const float out_factor = 0xFFFF;
    const float v = 100.0f * (brightness/255.0f) * (c/255.0f);
    return out_factor * ((v <= 8) ? v / 902.3 : pow((v + 16) / 116.0, 3));
}

static CIEValue *CreateCIE1931LookupTable() {
    CIEValue *result = new CIEValue[256 * 256];
    for (int v = 0; v < 256; ++v) {      // Value
        for (int b = 0; b < 256; ++b) {  // Brightness
            result[b * 256 + v] = luminance_cie1931_internal(v, b);
        }
    }
    return result;
}

// Return a CIE1931 corrected value from given desired lumninace value and
// brightness.
static CIEValue luminance_cie1931(uint8_t value, uint8_t bright) {
    static const CIEValue *const luminance_lookup = CreateCIE1931LookupTable();
    return luminance_lookup[bright * 256 + value];
}
*/

namespace spixels {
LEDStrip::LEDStrip(int pixelCount)
    : pixelCount_(pixelCount)
/*
    , values_(new RGBc[pixelCount])
*/
    , brightnessScale16_(0x10000)
{
}

LEDStrip::~LEDStrip()
{
//	delete values_;
}

/*
void LEDStrip::SetPixel(int pos, const RGBc& c) {
    if (pos < 0 || pos >= pixelCount()) return;
    values_[pos] = c;
    SetLinearValues(pos,
                    luminance_cie1931(c.r, brightness_),
                    luminance_cie1931(c.g, brightness_),
                    luminance_cie1931(c.b, brightness_));
}

void LEDStrip::SetBrightness(uint8_t new_brightness) {
    if (new_brightness == brightness_) return;
    brightness_ = new_brightness;
    for (int i = 0; i < pixelCount_; ++i) {
        SetPixel(i, values_[i]);  // Force recalculation.
    }
}
*/

namespace {


/////////////////////////////////////////////////
//#pragma mark - WS2801LedStrip:

class WS2801LedStrip : public LEDStrip {
public:
    WS2801LedStrip(MultiSPI *spi, MultiSPI::Pin pin, int pixelCount)
        : LEDStrip(pixelCount), spi_(spi), pin_(pin) {
        spi_->RegisterDataGPIO(pin, pixelCount * 3);
    }

    virtual void SetPixel8(int pixelIndex, uint8_t red, uint8_t green, uint8_t blue)
    {
    	uint32_t	const scale = brightnessScale16_;
    	
    	if (scale > 0x10000)
    	{
    		red = (uint8_t)std::min(red * scale / 0x10000, (uint32_t)0xFF);
    		green = (uint8_t)std::min(green * scale / 0x10000, (uint32_t)0xFF);
    		blue = (uint8_t)std::min(blue * scale / 0x10000, (uint32_t)0xFF);
    	}
    	else if (scale < 0x10000)
    	{
    		red = (uint8_t)(red * scale / 0x10000);
    		green = (uint8_t)(green * scale / 0x10000);
    		blue = (uint8_t)(blue * scale / 0x10000);
    	}
        spi_->SetBufferedByte(pin_, pixelIndex + 0, red);
        spi_->SetBufferedByte(pin_, pixelIndex + 1, green);
        spi_->SetBufferedByte(pin_, pixelIndex + 2, blue);
    }

private:
    MultiSPI *const spi_;
    const MultiSPI::Pin pin_;
};


/////////////////////////////////////////////////
//#pragma mark - LPD6803LedStrip:

class LPD6803LedStrip : public LEDStrip {
public:
    LPD6803LedStrip(MultiSPI *spi, MultiSPI::Pin pin, int pixelCount)
        : LEDStrip(pixelCount), spi_(spi), pin_(pin) {
        const size_t bytes_needed = 4 + 2 * pixelCount + 4;
        spi_->RegisterDataGPIO(pin, bytes_needed);

        // Four zero bytes as start-bytes for lpd6803
        spi_->SetBufferedByte(pin_, 0, 0x00);
        spi_->SetBufferedByte(pin_, 1, 0x00);
        spi_->SetBufferedByte(pin_, 2, 0x00);
        spi_->SetBufferedByte(pin_, 3, 0x00);
        for (int pos = 0; pos < pixelCount; ++pos)
        {
            SetPixel8(pos, 0, 0, 0);     // Initialize all top-bits.
        }
    }

    virtual void SetPixel8(int pixelIndex, uint8_t red, uint8_t green, uint8_t blue)
    {
    	uint32_t	const scale = brightnessScale16_;
    	
    	if (scale > 0x10000)
    	{
    		red = (uint8_t)std::min(red * scale / 0x10000, (uint32_t)0xFF);
    		green = (uint8_t)std::min(green * scale / 0x10000, (uint32_t)0xFF);
    		blue = (uint8_t)std::min(blue * scale / 0x10000, (uint32_t)0xFF);
    	}
    	else if (scale < 0x10000)
    	{
    		red = (uint8_t)(red * scale / 0x10000);
    		green = (uint8_t)(green * scale / 0x10000);
    		blue = (uint8_t)(blue * scale / 0x10000);
    	}
    	
        uint16_t		data;
        
        data = (1<<15);  // start bit
        data |= (red >> 3) << 10;
        data |= (green >> 3) <<  5;
        data |= (blue >> 3) <<  0;

		int				const offset = 4 + 2 * pixelIndex;
		
        spi_->SetBufferedByte(pin_, offset + 0, data >> 8);
        spi_->SetBufferedByte(pin_, offset + 1, data & 0xFF);
    }

private:
    MultiSPI *const spi_;
    const MultiSPI::Pin pin_;
};


/////////////////////////////////////////////////
//#pragma mark - APA102LedStrip:

class APA102LedStrip : public LEDStrip
{
public:
    APA102LedStrip(MultiSPI *spi, MultiSPI::Pin pin, int pixelCount)
	: LEDStrip(pixelCount)
	, spi_(spi)
	, pin_(pin)
	{
        const size_t startframe_size = 4;
        const size_t data_size = 4*pixelCount;
        const size_t endframe_size = (pixelCount+15) / 16;
        const size_t bytes_needed = startframe_size + data_size + endframe_size;

        spi_->RegisterDataGPIO(pin, bytes_needed);

        // Four zero bytes as start-bytes
        spi_->SetBufferedByte(pin_, 0, 0x00);
        spi_->SetBufferedByte(pin_, 1, 0x00);
        spi_->SetBufferedByte(pin_, 2, 0x00);
        spi_->SetBufferedByte(pin_, 3, 0x00);

        // Make sure the start bits are properly set.
        for (int i = 0; i < pixelCount; ++i)
        {
            SetPixel8(i, 0, 0, 0);
        }

		// TODO:  Add "reset frame" to support APA102 clone chip
		
        // We need a couple of more bits clocked at the end.
        for (size_t tail = startframe_size + data_size; tail < bytes_needed; ++tail)
        {
            spi_->SetBufferedByte(pin_, tail, 0xff);
        }

		uint32_t		index;
		
		index = 0;
		hardwareMultiplierInverseTable_[0] = 0;
		while (++index < 32)
		{
			hardwareMultiplierInverseTable_[index] = (0x1F << 8) / index;
		}
    }

    virtual void SetPixel8(int pixelIndex, uint8_t red, uint8_t green, uint8_t blue)
    {
    	uint32_t		scale;
   		uint32_t		hardwareMultiplier;
    	
    	scale = brightnessScale16_;
    	if (scale > 0x10000)
    	{
    		red = (uint8_t)std::min(red * scale / 0x10000, (uint32_t)0xFF);
    		green = (uint8_t)std::min(green * scale / 0x10000, (uint32_t)0xFF);
    		blue = (uint8_t)std::min(blue * scale / 0x10000, (uint32_t)0xFF);
    		hardwareMultiplier = 0x1F;
    	}
    	else if (scale < 0x10000)
    	{
    		hardwareMultiplier = (scale + 0x07FF) >> 11;
    		if (hardwareMultiplier < 0x01)
    		{
    			hardwareMultiplier = 0x01;
    		}
    		else if (hardwareMultiplier > 0x1F)
    		{
    			hardwareMultiplier = 0x1F;
    		}
    		scale = (scale * hardwareMultiplierInverseTable_[hardwareMultiplier]) >> 8;
    		red = (uint8_t)(red * scale / 0x10000);
    		green = (uint8_t)(green * scale / 0x10000);
    		blue = (uint8_t)(blue * scale / 0x10000);
    	}
    	else
    	{
    		hardwareMultiplier = 0x1F;
    	}

        int				const offset = 4 + 4 * pixelIndex;

		spi_->SetBufferedByte(pin_, offset + 0, (uint8_t)(0xE0 | hardwareMultiplier));
        spi_->SetBufferedByte(pin_, offset + 1, blue);
        spi_->SetBufferedByte(pin_, offset + 2, green);
        spi_->SetBufferedByte(pin_, offset + 3, red);
    }
    virtual void SetPixel16(int pixelIndex, uint16_t red, uint16_t green, uint16_t blue)
    {
    	uint32_t		const scale = brightnessScale16_;

    	if (scale > 0x10000)
    	{
    		red = (uint16_t)std::min(red * scale / 0x10000, (uint32_t)0xFFFF);
    		green = (uint16_t)std::min(green * scale / 0x10000, (uint32_t)0xFFFF);
    		blue = (uint16_t)std::min(blue * scale / 0x10000, (uint32_t)0xFFFF);
    	}
    	else if (scale < 0x10000)
    	{
    		red = (uint16_t)(red * scale / 0x10000);
    		green = (uint16_t)(green * scale / 0x10000);
    		blue = (uint16_t)(blue * scale / 0x10000);
    	}

		uint16_t		const maxHigh5Bits = std::max(std::max(red, green), blue) >> 11;
		uint16_t		const hardwareMultiplier = std::min(maxHigh5Bits + 1, 31);
		uint32_t		const inverse8 = hardwareMultiplierInverseTable_[hardwareMultiplier];
		
		red = (uint16_t)(red * inverse8 / 0x10000);
		green = (uint16_t)(green * inverse8 / 0x10000);
		blue = (uint16_t)(blue * inverse8 / 0x10000);
		
        int				const offset = 4 + 4 * pixelIndex;

		spi_->SetBufferedByte(pin_, offset + 0, (uint8_t)(0xE0 | hardwareMultiplier));
        spi_->SetBufferedByte(pin_, offset + 1, (uint8_t)blue);
        spi_->SetBufferedByte(pin_, offset + 2, (uint8_t)green);
        spi_->SetBufferedByte(pin_, offset + 3, (uint8_t)red);
    }

private:
    MultiSPI*		const spi_;
    MultiSPI::Pin	const pin_;
   	uint32_t		hardwareMultiplierInverseTable_[32];
};

}  // anonymous namespace

// Public interface
LEDStrip *CreateWS2801Strip(MultiSPI *spi, MultiSPI::Pin pin, int pixelCount) {
    return new WS2801LedStrip(spi, pin, pixelCount);
}
LEDStrip *CreateLPD6803Strip(MultiSPI *spi, MultiSPI::Pin pin, int pixelCount) {
    return new LPD6803LedStrip(spi, pin, pixelCount);
}
LEDStrip *CreateAPA102Strip(MultiSPI *spi, MultiSPI::Pin pin, int pixelCount) {
    return new APA102LedStrip(spi, pin, pixelCount);
}
}  // spixels namespace
