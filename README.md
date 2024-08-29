#  Boardoza GP-02 GNSS Satellite Navigation Breakout Board
GP-02 is a high-performance BDS/GNSS multi-mode satellite navigation receiver SOC
module, which integrates RF front-end, digital baseband processor, 32-bit RISC CPU,
power management and active antenna detection and protection functions. Support a
variety of satellite navigation systems, including China's Beidou satellite navigation
system BDS, the United States' GPS, and Russia's GLONASS, which can realize
multi-system joint positioning.
### [Click here to purchase!](https://www.ozdisan.com)
<!-- ChatGPT: The GP-02 High-Performance BDS/GNSS Multi-Mode Satellite Navigation Receiver SOC Breakout Board offers advanced positioning and navigation capabilities, leveraging the GP-02 SOC. This board supports multiple satellite systems, including BeiDou, GPS, and GLONASS, providing robust and accurate location tracking. The GP-02 is ideal for applications in automotive navigation, asset tracking, and any project requiring reliable and precise geolocation data. -->

|Front Side|Back Side|
|:---:|:---:|
| ![ GP-02 Front](./assets/GP-02%20Front.png)| ![GP-02 Back](./assets/GP-02%20Back.png)|

---
## Key Features
- It can work with 3.3V or 5V.
- It easily communicates with an MCU with TTL  UART outputs.
- J3 and J4 connectors are work with 3.3V level.
- This board works with UART. (You can see more info in GP-02 Datasheet.)

<!-- ChatGPT:
- Multi-Mode Support: Supports BDS, GPS, and GLONASS for reliable and accurate global positioning.
- High-Performance SOC: Ensures precise geolocation even in challenging environments.
- Compact Design: Integrated RF front-end in a small form factor for superior signal reception.
- Low Power Consumption: Ideal for battery-powered applications, extending device battery life.
- Easy Integration: Standard UART interface enables seamless connectivity with various systems. -->
---

## Technical Specifications
**Voltage Input Type:**	Molex 4 pin 2.50mm header

**Input Voltage:**	5V or 1S Battery

**Functions:**	BDS/GNSS multi-mode satellite navigation receiver SOC module.

**Operating Temperature:**	 -40°C to +85°C

**Board Dimensions:**	20mm x 40mm

---
## Board Pinout
| ( J1 ) Pin Number | Pin Name | Description |
| :---: | :---: | --- |
|1|	VCC	|Power Supply|
|2|	RX	|Received Data Pin|
|3|	TX	|Transmit Data Pin|
|4|	GND	|Ground|

| ( J3 ) Pin Number | Pin Name | Description |
| :---: | :---: | --- |
|1|	PPS	| Time pulse signal|
|2|	N/F	| Shutdown control, keep high level during normal operation; internal pull-up|
| 3 |	RST	| External reset input, internal pull-up, it must be left.|

| ( J4 ) Pin Number | Pin Name | Description |
| :---: | :---: | --- |
|1-2|	BDS-GPS	| When HIGH level or floating, it is BDS+GPS. (Actually this pin is 3.3V power pin, middle pin is gps io pin; when you jumper it together io pin will be “HIGH”. )|
|2-3|	GPS-GLNSS	| When LOW level, it is GPS+GLONASS. (Actually this pin is GND power pin, middle pin is gps io pin; when you jumper it together io pin will be “LOW”. )|
---
## Board Dimensions

<img src="./assets/GP-02 Dimension.png" alt=" GP-02 Dimension" width="450"/>

---
## Step Files

[Boardoza GP-02.step]()

---
## Datasheet

[Boardoza GP-02 Datasheet.pdf](.)

---
## Version History
- V1.0.0 - Initial Release

---
## Support
- If you have any questions or need support, please contact support@boardoza.com

---
## License

Shield: [![CC BY-SA 4.0][cc-by-sa-shield]][cc-by-sa]

This work is licensed under a [Creative Commons Attribution-ShareAlike 4.0 International License][cc-by-sa].

[![CC BY-SA 4.0][cc-by-sa-image]][cc-by-sa]

[cc-by-sa]: http://creativecommons.org/licenses/by-sa/4.0/
[cc-by-sa-image]: https://licensebuttons.net/l/by-sa/4.0/88x31.png
[cc-by-sa-shield]: https://img.shields.io/badge/License-CC%20BY--SA%204.0-lightgrey.svg
