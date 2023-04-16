# HyperSerialPico
HyperHDR's highspeed adalight serial port LED driver implementation for Raspberry Pi Pico RP2040.  

# Example of supported boards using Rp2040
<p align="center"><img src="https://user-images.githubusercontent.com/69086569/236885968-baab51ba-a54b-4072-9a2a-cf867f2edb4b.png"/><img src="https://user-images.githubusercontent.com/69086569/236885360-dce9cfd7-92a8-43c6-911f-649325ee8a96.png"/></p>

# Supported LED strips
| LED strip / Device             |    Single lane   |    Multi-segment   |
|--------------------------------|:----------------:|:------------------:|
| SK6812 cold white              |       yes        |        yes         |
| SK6812 neutral white           |       yes        |        yes         |
| WS281x                         |       yes        |        yes         |
| SPI (APA102, SK9812, HD107...) |       yes        |        no*         |

`*` you don't need parallel mode for SPI LEDs, they are already using high speed 10Mb internal data line

# How to flash it?
It's very easy and you don't need any special flasher.  
Download the firmware from the [Release folder](https://github.com/awawa-dev/HyperSerialPico/releases).
* If you Pico board has only one button (`boot`) then press & hold it and connect the board to the USB port. Then you can release the button.
* If you Pico board has two buttons, connect it to the USB port. Then press & hold `boot` and `reset` buttons, then release `reset` and next release `boot` button.  

In the system file explorer you should find new drive (e.g. called `RPI-RP2` drive) exposed by the Pico board. Drap & drop the selected fimrware. 
The Pico will reset automaticly after the upload and after few seconds it will be ready to use by HyperHDR as a serial port device using Adalight driver.

# HyperHDR configuration
Make sure you have enabled `AWA protocol` and `Esp8266/ESP32/Rp2040 handshake` options.  
You can leave the speed at `2000000` because the CDC driver should use the highest possible speed automatically (at least it happens on Windows).  
![obraz](https://user-images.githubusercontent.com/69086569/236870662-12f67d14-c2ca-4ba1-b6a3-e34c27949d19.png)

# Default pinout
  
**LED output (SK6812/WS281x):** GPIO2 for Data    
**LED output (SPI LEDs):** GPIO2 for Data, GPIO3 for Clock  

Pinout can be changed, but you need to make changes to CMakeList.txt (`OUTPUT_DATA_PIN`/`OUTPUT_CLOCK_PIN`) and recompile the project. Also multi-segment mode can be enabled in this file: `SECOND_SEGMENT_INDEX` option at the beginning and optionally `SECOND_SEGMENT_REVERSED`.  
Once compiled, the results can be found in the `firmwares` folder.  

# Some benchmark results

| Single RGBW LED strip                          | Max. refresh rate |
|------------------------------------------------|-------------------|
| 300LEDs RGBW<br>Refresh rate/continues output  |         83        |
| 600LEDs RGBW<br>Refresh rate/continues output  |         42        |
| 900LEDs RGBW<br>Refresh rate/continues output  |         28        |

| Parallel multi-segment mode                           | Max. refresh rate |
|---------------------------------------------------------------------------|----------------------|
| 300LEDs RGBW<br>Refresh rate/continues output<br>SECOND_SEGMENT_INDEX=150 |          100         |
| 600LEDs RGBW<br>Refresh rate/continues output<br>SECOND_SEGMENT_INDEX=300 |           83         |
| 900LEDs RGBW<br>Refresh rate/continues output<br>SECOND_SEGMENT_INDEX=450 |           55         |
