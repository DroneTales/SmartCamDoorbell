# Smart Camera Doorbell
 ESP32C3 smart camera doorbell for Apple Home Kit

 This repository contains firmware for the HomeKit device described in [this video](https://youtu.be/UexestUQARw).
 
 **Required Hardware**
 - 433Mhz doorbell
 - ESP32C4FN Super Mini
 
 **Required Arduino Libraries**
 - esp32 by Espressif Systems (board) 3.3.7
 - HomeSpan 2.1.7
 
 **Arduino IDE Settings**
 - Board: ESP32C3 Dev BModule
 - ESP CDC On Boot: Enabled
 - CPU Frequency: 80MHz (WiFi)
 - Core Debug Level: None
 - Erase All Flash Before Sketch Upload: Disabled
 - Flash frequency: 80Mhz
 - Flash Mode: QIO
 - Flash Size: 4MB (32Mb)
 - JTAG Adapter: Disabled
 - Partition Scheme: Huge APP (3MB No OTA/1MB SPIFFS)
 - Upload Speed: 921600
 - Zigbee Mode: Disabled
 - Programmer: Esptool

 ## Installing MQTT broker

```
sudo apt-get update 
sudo apt-get upgrade 

sudo apt-get install mosquitto mosquitto-clients

sudo systemctl enable mosquitto

sudo nano /etc/mosquitto/mosquitto.conf
```

Delete all lines and paste the following configuration

```
per_listener_settings true

pid_file /run/mosquitto/mosquitto.pid

persistence true
persistence_location /var/lib/mosquitto/

log_dest file /var/log/mosquitto/mosquitto.log

include_dir /etc/mosquitto/conf.d

listener 1883
allow_anonymous false
password_file /etc/mosquitto/passwd
```

Create new MQTT user

`sudo mosquitto_passwd -c /etc/mosquitto/passwd dronetales`

Enter new password `dronetales`

Start MQTT server

`sudo systemctl restart mosquitto`

## Configure HomeBridge

Open HomeBridge Web UI. Select "Edit JSON". 

In the CameraUI section replace MQTT settings with the following lines

```
"mqtt": {
    "active": true,
    "tls": false,
    "host": "127.0.0.1",
    "port": 1883,
    "username": "dronetales",
    "password": "dronetales"
},
```

If you used different user name and password when configuring MQTT provide them in the "username" and "password" parameters.

Scroll down to the "cameras" section and look for "mqtt" section there. If any then repace it with the following lines. If there is no such section add new right after "videoanalysis".

```
"mqtt": {
      "doorbellTopic": "doorcam/bell",
      "doorbellMessage": "RING",
},
```

Now add the following line right before "videoConfig" section

'"doorbell": true,`

That's ALL.

## Support the project
 
 If you like the project you can support me by the following link:  

 **BuyMeACoffee**: https://buymeacoffee.com/dronetales  
 **Boosty**: https://boosty.to/drone_tales/donate  
 
 **BTC**: bitcoin:1A1WM3CJzdyEB1P9SzTbkzx38duJD6kau  
 **BCH**: bitcoincash:qre7s8cnkwx24xpzvvfmqzx6ex0ysmq5vuah42q6yz  
 **ETH**: 0xf780b3B7DbE2FC74b5F156cBBE51F67eDeAd8F9a  
