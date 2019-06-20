# DIY Code
<img src='res/images/header.jpg'>

## Usage
1. Clone or download repository
2. Download and install Arduino:
  <https://www.arduino.cc/en/Main/Software>
<img src='res/images/arduino.png'>
3. Add this to your boards:
   <https://dl.espressif.com/dl/package_esp32_index.json>

    (more instructions:
<https://github.com/espressif/arduino-esp32/blob/master/docs/arduino-ide/boards_manager.md>)
<img src='res/images/aPref.png'>
4. Open `camera-2019.ino`
<img src='res/images/ide.png'>
5. Install USB to UART Driver:
   <https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers>
<img src='res/images/driver.png'>
6. Connect camera via USB cable and choose your port in Arduino IDE.
   (Make sure PSRAM is "Enabled").
<img src='res/images/port.png'>
7. Compile and run!
<img src='res/images/run.png'>

## Configuration
Line 11.

```
//#define SOFTAP_MODE 
```

Commenting it will connect it to your home router.
Provide wifi router credentials in lines 17,18

```
#define WIFI_SSID   ""
#define WIFI_PASSWD ""
```

## Further Reading
1. AWS Kinesis: https://aws.amazon.com/quickstart/architecture/camera-connector-onica/
2. AWS Rekognition: https://aws.amazon.com/rekognition/
