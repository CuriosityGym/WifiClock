# WifiClock
ts a pretty simple project. We use an ESP8266 to fetch time from an NTP server, and two Neopixel Rings ( 12 LED and 24 LEDs) To show minutes and hours. 

Feature List 
- Shows AM and PM by Color of LED's on 3,6,9,12 positions ( AM is yellow because the Sun is yellow, PM is white because the Moon is white) 

-  Hours is green in color, Minutes is Blue.

- Exposes Webpage and AP for configuration of Wifi credentials, timezone setup. 

TODO 
- Allow turning off the clock from webpage( ESP remains on, Neopixels are turned off) 
- Color choice from webpage for Hours and Minutes 
- Better Case 
- A Stand 


<img src="https://github.com/CuriosityGym/WifiClock/blob/master/images/IMG-20160812-WA0006.jpg" width="400">

<img src="https://github.com/CuriosityGym/WifiClock/blob/master/images/IMG_20160518_140716618.jpg" width="400">&nbsp;<img src="https://github.com/CuriosityGym/WifiClock/blob/master/images/IMG_20160518_141220412.jpg" width="400">



#Parts List

1× ESP8266-01

1 × LM1117-3.3

1× Neopixel Ring- 12 LEDs

1× Neopixel Ring- 24 LEDs

1× TPA4056 Li-ion Battery Charger Module

1× 2200mAH Flat Li-ion Battery
