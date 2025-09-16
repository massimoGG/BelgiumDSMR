# DSMR P1 Port Reader

![Alt text](/Grafana.png?raw=true "Grafana Dashboard")

## Overview

This is a personal project aimed at reading the serial DSMR P1 port of my Fluvius (Belgium) digital electricity meter. The project is developed in C and serves as a proof of concept for future implementations. The ultimate goal is to transition this project to an STM32 microcontroller with Ethernet capabilities, allowing for more robust and efficient data handling.

## Project Description

The DSMR (Dutch Smart Meter Requirements) P1 port provides access to real-time data from digital electricity meters. This project focuses on reading and processing the data transmitted through this port, enabling users to monitor their electricity consumption and other relevant metrics.

### Current Implementation

- **Platform**: Raspberry Pi
- **Language**: C
- **Functionality**: 
  - Reads data from the DSMR P1 port.
  - Parses the data and convert to line protocol
  - Line protocol is then sent to InfluxDB

### Future Plans

- Transition the project to an STM32 microcontroller.
- Implement Ethernet connectivity for remote data access and monitoring.
- Enhance the functionality to include additional features such as data visualization and alerting.

## Getting Started

### Prerequisites

To run this project, you will need:

- A Raspberry Pi (or compatible device).
- A Fluvius digital electricity meter with a DSMR P1 port.
- A serial connection to the meter. (I bought this one: https://www.sossolutions.nl/slimme-meter-kabel-p1-kabel-3-meter)
- An InfluxDB server
