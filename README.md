# Boardoza GP-02 BDS/GNSS Multi-Mode Navigation Receiver Breakout Board

The **Boardoza GP-02** is a high-performance **satellite navigation breakout board** featuring the GP-02 SOC, which supports **multi-mode positioning systems** including **BeiDou (BDS)**, **GPS**, and **GLONASS**. It is designed to provide **accurate and reliable geolocation data**, even in challenging signal environments, making it ideal for applications such as **automotive navigation**, **asset tracking**, and **outdoor robotics**.

This module offers **TTL-level UART communication**, making it easy to interface with microcontrollers, and features **low power consumption**, making it highly suitable for battery-powered systems. Its compact design and integrated RF front-end simplify integration in space-constrained projects.

## [Click here to purchase!](https://www.ozdisan.com/maker-and-iot-products/boardoza/boardoza-modules/BOARDOZA-GP-02/1206509)

|Front Side|Back Side|
|:---:|:---:|
| ![ GP-02 Front](./assets/GP-02%20Front.png)| ![GP-02 Back](./assets/GP-02%20Back.png)|

---

## Key Features

- **Multi-Mode Satellite Support:** Simultaneous support for BeiDou, GPS, and GLONASS systems ensures broad global coverage and enhanced positioning accuracy.
- **Integrated RF Front-End:** Compact SOC design includes signal processing and antenna interface for superior reception in a small footprint.
- **Low Power Design:** Operates efficiently from 3.3V or 5V sources; ideal for portable devices.
- **Simple TTL UART Communication:** Easily integrates with microcontrollers for real-time geolocation data exchange.
- **Mode Selection via Jumper Pins:** Configure BDS+GPS or GPS+GLONASS modes via J4 jumper configuration.

---

## Technical Specifications

**Model:** Ai Thinker GP-02  
**Voltage Input Type:** 4-pin 2.50mm header  
**Input Voltage:** 5V or 1S Li-Ion Battery (3.3V)  
**Functions:** BDS/GNSS Multi-Mode Navigation Receiver SOC  
**Communication Interface:** UART (TTL Level)  
**Connector Voltage Level:** 3.3V (J3, J4)  
**Operating Temperature:** -40°C ~ +85°C  
**Board Dimensions:** 20mm x 40mm  

---

## Board Pinout

### ( J1 ) UART Communication & Power

| Pin Number | Pin Name | Description            |
|:----------:|:--------:|------------------------|
| 1          | VCC      | Power Supply           |
| 2          | RX       | Received Data Pin      |
| 3          | TX       | Transmitted Data Pin   |
| 4          | GND      | Ground                 |

### ( J3 ) GNSS Control & Timing

| Pin Number | Pin Name | Description                                   |
|:----------:|:--------:|-----------------------------------------------|
| 1          | PPS      | Time pulse signal output                      |
| 2          | N/F      | Shutdown control (active LOW, internal pull-up) |
| 3          | RST      | External reset input (leave unconnected)      |

### ( J4 ) GNSS System Selection Jumper

| Pin Combination | Mode          | Description |
|:---------------:|---------------|-------------|
| 1–2             | BDS + GPS     | Jumper connects IO to HIGH (via VCC) |
| 2–3             | GPS + GLONASS | Jumper connects IO to GND (LOW state) |

> **Note:** J4 is a clever use of hardware jumper to select GNSS mode. When middle pin floats (no jumper), the module defaults to BDS + GPS.

---

## Board Dimensions

<img src="./assets/GP-02 Dimension.png" alt=" GP-02 Dimension" width="450"/>

---

## Step Files

[Boardoza GP-02.step](./assets/Boardoza%20GP-02%20Step.step)

---

## Datasheet

[Boardoza GP-02 Datasheet.pdf](./assets/GP-02%20Datasheet.pdf)

---

## Version History

- V1.0.0 - Initial Release

---

## Support

- If you have any questions or need support, please contact <support@boardoza.com>

---

## License

Shield: [![CC BY-SA 4.0][cc-by-sa-shield]][cc-by-sa]

This work is licensed under a [Creative Commons Attribution-ShareAlike 4.0 International License][cc-by-sa].

[![CC BY-SA 4.0][cc-by-sa-image]][cc-by-sa]

[cc-by-sa]: http://creativecommons.org/licenses/by-sa/4.0/
[cc-by-sa-image]: https://licensebuttons.net/l/by-sa/4.0/88x31.png
[cc-by-sa-shield]: https://img.shields.io/badge/License-CC%20BY--SA%204.0-lightgrey.svg
