#ifndef DSMR_H
#define DSMR_H

/**
 * OBIS Types
 * Floating point Fn(x,y);
 *  x = minimum of decimals;
 *  y = maximum of decimals
 *  Fn(0,3) = minumum of 0, max of 3 decimals
 * In = Integer number
 *  I4 = YYYY = 4 decimals
 * Sn = Alphanumeric string
 *  S6 = CCCCCC = 6
 * TST = YYMMDDhhmmssX (X=S if DST is active) or
 *  (X=W if DST is not active
 *  DST = Daylight Saving Time
 *
 * COSEM object attributes:
 * | Tag | COSEM Data Type | Value Format |
 * |  0  | null-data       | Empty        |
 * |  3  | boolean         | I1           |
 * |  4  | bit-string      | Sn           |
 * |  5  | double-long     | Fn(x,y)      |
 * |  6  | double long unsigned| Fn(x,y)  |
 * |  7  | floating-point  | Fn(x,y)      |
 * |  9  | octet-string    | Sn           |
 * |  10 | visible-string  | Sn           |
 * |  13 | bcd             | S2           |
 * |  15 | integer         | In           |
 * |  16 | long            | Fn(x,y)      |
 * |  17 | unsigned        | Fn(x,y)      |
 * |  18 | long-unsigned   | Fn(x,y)      |
 * |  20 | long64          | Fn(x,y)      |
 * |  21 | long64-unsigned | Fn(x,y)      |
 * |  22 | enum            | Fn(x,y)      |
 * |  23 | float-32        | Fn(x,y)      |
 * |  24 | float-64        | Fn(x,y)      |
 *
 * Representation of COSEM objects
 *  ID (Mv*U)
 *  1  23  45
 * 1) OBIS reduced ID-code
 * 2) Separator ( ASCII 28
 * 3) COSEM object attribute value
 * 4) Unit of measurement values
 * 5) Separator ) ASCII 29
 *
 * The following table holds data objects represented with P1 Interface
 *  together with OBIS ref-erence including object Attribute and Value
 *  Format for Reduced ID codes. Every line is ended with a CR/LF
 * (Carriage Return / Line Feed).
 */

/**
 * OBIS references
 */
#define DATE_TIME_STAMP "0-0:1.0.0"       // TST
#define EQUIPMENT_IDENTIFIER "0-0:96.1.1" // Sn (n=0..96), tag 9
// Tarrif Meter kWh
#define METER_READING_ELECTRICITY_DELIVERED_TO_CLIENT_TARIFF_1 "1-0:1.8.1" // F9(3,3)
#define METER_READING_ELECTRICITY_DELIVERED_TO_CLIENT_TARIFF_2 "1-0:1.8.2" // F9(3,3)
#define METER_READING_ELECTRICITY_DELIVERED_BY_CLIENT_TARIFF_1 "1-0:2.8.1" // F9(3,3)
#define METER_READING_ELECTRICITY_DELIVERED_BY_CLIENT_TARIFF_2 "1-0:2.8.2" // F9(3,3)
#define TARIFF_INDICATOR_ELECTRICITY "0-0:96.14.0"                         // S4
// Power delivery/receiving
#define ACTUAL_ELECTRICITY_POWER_DELIVERED "1-0:1.7.0" // F5(3,3)
#define ACTUAL_ELECTRICITY_POWER_RECEIVED "1-0:2.7.0"  // F5(3,3)

// Text message max 1024 characters
#define TEXT_MESSAGE_MAX_1024 "0-0:96.13.0" // Sn (n=0..2048????????)

// Voltages
#define INSTANTANEOS_VOLTAGE_L1 "1-0:32.7.0" // F4(1,1)
#define INSTANTANEOS_VOLTAGE_L2 "1-0:52.7.0" // F4(1,1)
#define INSTANTANEOS_VOLTAGE_L3 "1-0:72.7.0" // F4(1,1)
// Currents
#define INSTANTANEOS_CURRENT_L1 "1-0:31.7.0" // F5(1,1)
#define INSTANTANEOS_CURRENT_L2 "1-0:51.7.0" // F5(1,1)
#define INSTANTANEOS_CURRENT_L3 "1-0:71.7.0" // F5(1,1)
// Instantaneous active (positive/negative) power
#define INSTANTANEOUS_ACTIVE_POSITIVE_POWER_L1 "1-0:21.7.0" // F5(3,3)
#define INSTANTANEOUS_ACTIVE_POSITIVE_POWER_L2 "1-0:41.7.0" // F5(3,3)
#define INSTANTANEOUS_ACTIVE_POSITIVE_POWER_L3 "1-0:61.7.0" // F5(3,3)
#define INSTANTANEOUS_ACTIVE_NEGATIVE_POWER_L1 "1-0:22.7.0" // F5(3,3)
#define INSTANTANEOUS_ACTIVE_NEGATIVE_POWER_L2 "1-0:42.7.0" // F5(3,3)
#define INSTANTANEOUS_ACTIVE_NEGATIVE_POWER_L3 "1-0:62.7.0" // F5(3,3)

// Device Type
#define DEVICE_TYPE "0-1:24.1.0" // F3(0,0)

#define CURRENT_AVERAGE_DEMAND_ACTIVE_ENERGY_IMPORT "1-0:1.4.0" // F5(3,3) Unit: kW ???

// Peak power of the running month
#define MAXIMUM_DEMAND_RUNNING_MONTH "1-0:1.6.0"   // (TST)(F5(3x3)) Unit kW
#define MAXIMUM_DEMAND_LAST_13_MONTHS "0-0:98.1.0" // (TST)(F5(3,3)) Unit kW

/**
 * DSMR Telegram
 */
typedef struct DSMR
{

} DSMR_T;

void decodeLine(char *, int, struct DSMR *);
void decodeOBISHashKeyValue(char *, int, int *, int *, unsigned char *);
int findOBISOIDByHash(unsigned char hash);

#endif