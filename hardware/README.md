# Hardware Documentation

## Wiring Diagram

### PCA9685 to OrangePi Zero 3

```
PCA9685 Module          OrangePi Zero 3 Pinout
-------------           -------------------
VCC (5V)         ──→    Pin 2 (5V) or Pin 4 (5V)
GND              ──→    Pin 6 (GND) or Pin 9/14/20 (GND)
SDA              ──→    Pin 3 (SDA/I2C0)
SCL              ──→    Pin 5 (SCL/I2C0)
```

### Pinout Reference

```
    ╔═══════════════════════════════════════╗
    ║  OrangePi Zero 3 GPIO Header          ║
    ╠═══╦═══╦═══════╦════════╦═════════════╣
    ║ 1 ║ 2 ║ 3.3V   ║ 5V     ║ GND         ║
    ║ 3 ║ 4 ║ SDA    ║ 5V     ║ (GPIO 226)  ║
    ║ 5 ║ 6 ║ SCL    ║ GND    ║ (GPIO 229)  ║
    ║ 7 ║ 8 ║ (TX)   ║ (RX)   ║ GND         ║
    ╚═══╩═══╩═══════╩════════╩═════════════╣
```

### Servo Connections

```
PCA9685 Channel    Servo Function
-----------        --------------
Channel 0         Base rotation (optional)
Channel 1         Shoulder joint
Channel 2         Gripper

V+ (5V external)  Servo power (DO NOT use OrangePi 5V!)
```

## Hardware Components

### Required

| Component | Model | Quantity | Notes |
|-----------|-------|----------|-------|
| Development Board | OrangePi Zero 3 | 1 | 1GB+ RAM |
| Servo Motor | SG90 9G | 2-3 | 180° rotation |
| PWM Controller | PCA9685 | 1 | 16-channel I2C |
| Jumper Wires | M-M | 4+ | For I2C connection |
| Power Supply | 5V/2A | 1 | For servos |

### Optional

| Component | Purpose |
|-----------|---------|
| External 5V Supply | Stable power for servos |
| 3D Printed Claw | Mechanical structure |
| Cooling Fan | Board cooling |

## Setup Steps

### 1. Enable I2C

```bash
# Check if I2C is available
ls /dev/i2c-*

# If not, enable in armbian-config
sudo armbian-config
# → System → Hardware → Enable I2C-0

# Or manually add to /boot/armbianEnv.txt
echo "dtparam=i2c0=on" | sudo tee -a /boot/armbianEnv.txt
sudo reboot
```

### 2. Install I2C Tools

```bash
sudo apt update
sudo apt install -y i2c-tools
sudo apt install -y libi2c-dev

# Test I2C bus
sudo i2cdetect -y 0
```

### 3. Connect Hardware

1. Power off OrangePi
2. Connect I2C wires (SDA, SCL, VCC, GND)
3. Connect external 5V to PCA9685 V+
4. Connect servos to channels 0, 1, 2
5. Power on

### 4. Verify Connection

```bash
# Scan I2C bus - should show 0x40
sudo i2cdetect -y 0

# Output should be:
#      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
# 00:                         -- -- -- -- -- -- -- --
# 10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 40: 40 -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
# 60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
```

### 5. Test with MimiClaw

```bash
# Run in simulated mode first
mimiclaw --hardware.simulated=true demo

# Then run with real hardware
mimiclaw demo
```

## Troubleshooting

### I2C Not Detected

1. Check wiring
2. Verify I2C is enabled
3. Try different I2C bus (1)

### Servos Not Moving

1. Check power supply (must be 5V, 2A+)
2. Verify servo connections
3. Test with PCA9685 Arduino library first

### Permission Denied

```bash
# Add user to i2c group
sudo usermod -aG i2c $USER
# Log out and back in
```
