# Vallox ESPHome component
Vallox SE ventilation control software for ESPHome (and thus Home Assistant)

# Hardware
See guide at https://www.creatingsmarthome.com/?p=73

## Should work with any device using the VALLOX DIGIT bus protocol over RS485 including:
- Vallox 096 SE
- Vallox 110 SE
- Vallox 121 SE (version without front heating module)
- Vallox 121 SE (version with front heating)
- Vallox 150 SE
- Vallox 270 SE
- Vallox Digit SE
- Vallox Digit SE 2
- Vallox ValloPlus 350 SE
- Vallox ValloPlus SE 500

# Home Assistant components
Contained entities:
* climate: Climate entity with Off/Fan Only/Heating operation mode, fan speed control, target temperature control
* All additional sensors, number, button, etc. are optional and can be included as needed.
* If you need a specific feature that is not included let me know, adding it should be straightforward.

# Configuration
Copy content of example_vallox.yaml to your esphome configuration for your device.
Adjust UART pins (RX/TX) as used with your device.
