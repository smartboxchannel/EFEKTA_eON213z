# EFEKTA_eON213z
My new zigbee project. Wireless temperature and humidity mini sensor with electronic ink display 2.13 inches, low power consumption, compact size, enclosure with magnets. The device use SHTC3 sensors, chip CC2530, battery CR2477. Support in zigbee2mqtt, zha.

### It is forbidden to manufacture devices for commercial sale, only for personal use.

### You can purchase a ready-made device by writing to the mail hello@efektalab.com

### Delivery is carried out worldwide.

## You can make your own pcb here - https://www.pcbway.com/setinvite.aspx?inviteid=550959




#### Donate to me: https://paypal.me/efektalab or just buy to support this project

#### Sale: https://www.tindie.com/products/diyberk/indoor-climate-sensor-efekta-eon213-zigbee/

#### Video: https://youtu.be/5huTVfVpZgs

#### zigbee2mqtt.io/supported-devices: https://www.zigbee2mqtt.io/supported-devices/#s=efekta

#### Telegram ZigDev - https://t.me/zigdev

#### Telegram DiyDev - https://t.me/diy_devices

More info at http://efektalab.com/eON213z

Chinese version - https://github.com/wzqvip/EFEKTA_eON213z/tree/Chinese_version



## V1_R1

![Indoor climate sensor EFEKTA eON213 Zigbee](https://github.com/smartboxchannel/EFEKTA_eON213z/blob/main/IMAGES/002.jpg) 


![Indoor climate sensor EFEKTA eON213 Zigbee](https://github.com/smartboxchannel/EFEKTA_eON213z/blob/main/IMAGES/EFEKTA_eON213z.jpg) 


### How to flash the device

1. Download the Smart RF Flash Programmer V1 https://www.ti.com/tool/FLASH-PROGRAMMER

2. Open the application select the HEX firmware file

3. Connect the device with wires to CCDebugger, first erase the chip, then flash it.

---

### How to install IAR

https://github.com/ZigDevWiki/zigdevwiki.github.io/blob/main/docs/Begin/IAR_install.md

https://github.com/sigma7i/zigbee-wiki/wiki/zigbee-firmware-install (RU)

---

### How to join:
#### If device in FN(factory new) state:
##### one way
1. Open z2m, make sure that joining is prohibited
2. Insert the battery into the device
3. Click on the icon in z2m - allow joining (you have 180 seconds to add the device)
4. Go to the LOGS tab
5. Press the reset button on the device (the join procedure will begin, еhe device starts flashing the LED repeatedly)
6. Wait, in case of successfull join, device will flash led 5 times, if join failed, device will flash led 2 times

##### another way
1. Open z2m, make sure that joining is prohibited
2. Insert the battery into the device
3. Click on the icon in z2m - allow joining (you have 180 seconds to add the device)
4. Go to the LOGS tab
5. Press and hold button (1) for 2-3 seconds, until device start flashing the LED repeatedly
6. Wait, in case of successfull join, device will flash led 5 times, if join failed, device will flash led 2 times


#### If device in a network:
##### one way 
1. Hold button (1) for 10 seconds, this will reset device to FN(factory new) status 
2. Click on the icon in z2m - allow joining (you have 180 seconds to add the device)
3. Go to the LOGS tab
5. Press and hold button (1) for 2-3 seconds, until device start flashing the LED repeatedly
6. Wait, in case of successfull join, device will flash led 5 times, if join failed, device will flash led 2 times

##### another way
1.Find the device in the list of z2m devices and delete it by applying force remove
2. Click on the icon in z2m - allow joining (you have 180 seconds to add the device)
3. Go to the LOGS tab
4. Press the reset button on the device (the join procedure will begin, еhe device starts flashing the LED repeatedly)
5. Wait, in case of successfull join, device will flash led 5 times, if join failed, device will flash led 2 times

![Efekta THP_LR \ THP](https://github.com/smartboxchannel/EFEKTA_eON213z/blob/main/IMAGES/003.jpg) 

### Troubleshooting

If a device does not connect to your coordinator, please try the following:

1. Power off all routers in your network.
2. Move the device near to your coordinator (about 1 meter).
or if you cannot disable routers (for example, internal switches), you may try the following:
2.1. Disconnect an external antenna from your coordinator.
2.2. Move a device to your coordinator closely (1-3 centimeters).
3. Power on, power on the device.
4. Restart your coordinator (for example, restart Zigbee2MQTT if you use it).

If the device has not fully passed the join

1. If the device is visible in the list of z2m devices, remove it by applying force remove
2. Restart your coordinator (for example, restart Zigbee2MQTT if you use it).
3. Click on the icon in z2m - allow joining (you have 180 seconds to add the device)
4. Go to the LOGS tab
5. Press and hold button (1) for 2-3 seconds, until device start flashing the LED repeatedly
6. Wait, in case of successfull join, device will flash led 5 times, if join failed, device will flash led 2 times



### Other checks

Please, ensure the following:

1. Your power source is OK (a battery has more than 3V). You can temporarily use an external power source for testings (for example, from a debugger).
2. The RF part of your E18 board works. You can upload another firmware to it and try to pair it with your coordinator. Or you may use another coordinator and build a separate Zigbee network for testing.
3. Your coordinator has free slots for direct connections.
4. You permit joining on your coordinator.
5. Your device did not join to other opened Zigbee network. When you press and hold the button, it should flash every 3-4 seconds. It means that the device in the joining state.


## Components (BOM):

CC2530 - https://ali.ski/x1m0vF (1 pc - $2.75)

CC2530 - https://ali.ski/mjth48 (10 pcs)

SHTC3 - https://ali.ski/yBYSsV (10 pcs - $15.76)

Waveshare 250x122 2,13 - https://ali.ski/D4eOVU (1pc - $13.3)

FPC FFC 0.5mm connector socket 24 pin - https://ali.ski/lCBot0 (1 p - $0.77)

Cell Holder CR2477 - https://ali.ski/CTUHOc (20 pcs - $5)

Micro Screws M1.4 3mm - https://ali.ski/gaFdO (100 pcs - $1)

Micro Screws M1.4 5mm - https://ali.ski/gaFdO (100 pcs - $1)

Micro SMD Tact Switch 2x4 2*4*3.5 - https://ali.ski/_D78Q (10 pcs - $1)

Neodymium magnet 15x3x3m - https://ali.ski/44p3R (10 pcs - $7)

Tantalum Capacitor 220UF 10V - https://ali.ski/Lx9iQd (10 pcs - $1,85)

Inductor Power Shielded Wirewound NR5040 5x5x4mm - https://ali.ski/iblu8q (50 pcs - $3)

Micro Button Tact Switch SMD 4Pin 3X4X2.5 - https://ali.ski/sGwFu (100 pcs - $1.7)

SMD Mini Toggle Slide Switch 7-Pin On/Off - https://ali.ski/xz9Yt (100 pcs - $3.2)

SI1308EDL - https://ali.ski/Q6Y12 (20 pcs - $5.3)
OR
Si1304BDL - https://ali.ski/938Klh (100 pcs - $15.5)

SMD Chip Multilayer Ceramic Capacitor 0603 1UF 25V - https://ali.ski/p3yr60 (100 pcs - $1.5)

SMD Chip Multilayer Ceramic Capacitor 0603 0.1UF 50V - https://ali.ski/p3yr60 (100 pcs - $1.5)

SMD Chip Multilayer Ceramic Capacitor 0805 4.7UF - https://ali.ski/iFXAc (100 pcs - $1.85)

MBR0530T1G - https://ali.ski/SJM7aK (100 pcs - $1.1)

SMD LED 0805 - https://ali.ski/wb6ZP (100 pcs - $2)

1% SMD resistor 0.47R 0603 - https://ali.ski/bX9HUg (100 pcs - $1.2)

1% SMD Resistor Kit Assorted Kit 1R-1M 0603 -  https://ali.ski/npItF (660 pcs - $1.45)


SUNLU PLA Carbon Fiber Premium 3D Printer Filament - https://ali.ski/bQkNR (1 pcs - $26.5)
