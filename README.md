# Simple Ultrasonic Movement Sensor

A very simple system applies an ultrasonic sensor to detect movements and signals the detection by broadcasting (enabling) a wireless network ID. 
Otherwise no wireless network is enabled.

      --------           .   -------
     | alert  |    <---- .  | alert |      ------
     |receiver|     wifi .  |checker|-----|HCSR04|
      --------           .   -------       ------
         |               .      |
      --------           .   -------
     | buzzer | optional .  |temp   | optional
      --------           .  |sensor |
                         .    -------                                               
     * flashes screen    .  * en/disables wifi on
     * enables buzzer    .    detection/no detection

## Aims 

* simple
* zero configuration necessary
* can be detected with the "alerting-device" or any 2GHz wifi capable device

## Non Aims

* not an accurate distance measurement tool
* not very accurate at all (because of reflections/echoes and varying air temperature)

## How to Use

* compile the project in alert-checker and flash on a device (ESP8266)
  * you will need an 
   * 1x HCSR04 for distance measurement
   * 1x DS1820 temperature sensor (optional, for propagation speed compensation)
* compile the project in alert-receiver and flash on a device (ESP8266)
  * you will need 
   * 1x push button (optional, to silence current alert) connected to D3
   * 1x buzzer 6V DC (optional, to accustically alert) connected to D8

* clone the project and modify the source for your needs
* for case and mounting see https://github.com/rubienr/cad/esp/ultrasonic-sensor/

