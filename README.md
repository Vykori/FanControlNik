a tiny Arduino sketch for the ATtiny to control a PWM fan's speed with an intuitive 1-button interface

## Arduino IDE setup

Info for setting up the ATtiny in Arduino IDE can be found here https://highlowtech.org/?p=1695

## Arduino CLI setup

If you're using arduino-cli you can add an entry to your config file at `~/.arduino15/arduino-cli.yaml`  
```yaml
board_manager:
    additional_urls:
        - https://raw.githubusercontent.com/damellis/attiny/ide-1.6.x-boards-manager/package_damellis_attiny_index.json
```  
then run:
```bash
arduino-cli core update-index
arduino-cli core install arduino:avr
arduino-cli core install attiny:avr
```

Now you can build by running:
```bash
arduino-cli compile --fqbn attiny:avr:ATtinyX5 FanControlNik.ino
```

([more info about arduino-cli setup here](https://docs.arduino.cc/arduino-cli/getting-started))
