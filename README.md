# Simple Ultrasonic Movement Sensor

A very simple system applies an ultrasonic sensor to detect movements and signals the detection by broadcasting (enabling) a wireless network ID. 
Otherwise no wireless network is enabled.
 
         --------                -------
        | alert  |    <----     | alert |      ------
        |receiver|     wifi     |checker|-----|HCSR04|
         --------                -------       ------
	  * flashes screen        * en/disables wifi on
          * enables buzzer          detection/no detection

## Aims 
* simple
* zero configuration necessary
* can be detected with the "alerting-device" or any wifi capable device

## Non Aims
* not an accurate distance measurement tool
* not very accurate at all (because of reflections and varying air temperature)

## How to Use

* compile the project in alert-checker and flash on a device (ESP8266)
* compile the project in alert-receiver and flash on a device (ESP8266)
* clone the project and modify the source for your needs

