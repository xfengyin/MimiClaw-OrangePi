// Copyright 2024 MimiClaw Authors
// SPDX-License-Identifier: MIT

package hardware

import (
	"fmt"
	"log"
	"os"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/xfengyin/mimiclaw/pkg/config"
)

// HardwareInfo holds detected hardware information
type HardwareInfo struct {
	Model      string
	Cores      int
	TotalRAM   float64 // GB
	HasNEON    bool
	HasI2C     bool
	OS         string
	Arch       string
}

// Detect detects hardware configuration
func Detect() *HardwareInfo {
	info := &HardwareInfo{
		Cores: runtime.NumCPU(),
		Arch:  runtime.GOARCH,
	}

	// Read device model
	if data, err := os.ReadFile("/proc/device-tree/model"); err == nil {
		info.Model = strings.TrimSpace(string(data))
	} else {
		info.Model = "Unknown Linux Device"
	}

	// Read memory info
	if data, err := os.ReadFile("/proc/meminfo"); err == nil {
		lines := strings.Split(string(data), "\n")
		for _, line := range lines {
			if strings.HasPrefix(line, "MemTotal:") {
				fields := strings.Fields(line)
				if len(fields) >= 2 {
					if kb, err := strconv.ParseFloat(fields[1], 64); err == nil {
						info.TotalRAM = kb / 1024 / 1024 // Convert KB to GB
					}
				}
			}
		}
	}

	// Check for NEON support
	if data, err := os.ReadFile("/proc/cpuinfo"); err == nil {
		info.HasNEON = strings.Contains(string(data), "neon") || strings.Contains(string(data), "asimd")
	}

	// Check for I2C
	info.HasI2C = exists("/dev/i2c-0") || exists("/dev/i2c-1")

	// OS info
	if data, err := os.ReadFile("/etc/os-release"); err == nil {
		for _, line := range strings.Split(string(data), "\n") {
			if strings.HasPrefix(line, "PRETTY_NAME=") {
				info.OS = strings.Trim(strings.TrimPrefix(line, "PRETTY_NAME="), `"`)
				break
			}
		}
	}

	return info
}

// ClawController controls the robotic claw
type ClawController struct {
	cfg      *config.HardwareConfig
	mu       sync.RWMutex
	open     bool
	position int
	simulated bool
	pwm      *PCA9685
}

// PCA9685 represents PWM controller
type PCA9685 struct {
	bus       int
	address   int
	frequency int
	file      *os.File
	mu        sync.Mutex
}

// NewClawController creates a new claw controller
func NewClawController(cfg *config.HardwareConfig) (*ClawController, error) {
	c := &ClawController{
		cfg:       cfg,
		open:      false,
		position:  90,
		simulated: cfg.Simulated,
	}

	if !cfg.Enabled || cfg.Simulated {
		log.Println("Hardware: Running in simulated mode")
		return c, nil
	}

	// Try to initialize PCA9685
	if err := c.initPCA9685(); err != nil {
		log.Printf("Hardware: PCA9685 init failed: %v, using simulated mode", err)
		c.simulated = true
	}

	return c, nil
}

// NewSimulatedClaw creates a simulated claw
func NewSimulatedClaw() *ClawController {
	return &ClawController{
		open:      false,
		position:  90,
		simulated: true,
		cfg: &config.HardwareConfig{
			Enabled: true,
			I2CBus:  0,
			I2CAddress: 0x40,
			PWMFrequency: 50,
		},
	}
}

// initPCA9685 initializes I2C PWM controller
func (c *ClawController) initPCA9685() error {
	i2cPath := fmt.Sprintf("/dev/i2c-%d", c.cfg.I2CBus)
	
	file, err := os.OpenFile(i2cPath, os.O_RDWR, 0)
	if err != nil {
		return fmt.Errorf("failed to open I2C bus: %w", err)
	}

	c.pwm = &PCA9685{
		bus:       c.cfg.I2CBus,
		address:   c.cfg.I2CAddress,
		frequency: c.cfg.PWMFrequency,
		file:      file,
	}

	// Set PWM frequency (50Hz for servos)
	if err := c.pwm.SetFrequency(c.cfg.PWMFrequency); err != nil {
		return err
	}

	log.Printf("Hardware: PCA9685 initialized at 0x%02X on I2C-%d", c.cfg.I2CAddress, c.cfg.I2CBus)
	return nil
}

// Open opens the claw
func (c *ClawController) Open() error {
	c.mu.Lock()
	defer c.mu.Unlock()
	
	log.Println("Hardware: Opening claw")
	
	if c.simulated {
		c.open = true
		c.position = 0
		return nil
	}

	return c.pwm.SetAngle(0, 0) // Channel 0, 0 degrees
}

// Close closes the claw
func (c *ClawController) Close() error {
	c.mu.Lock()
	defer c.mu.Unlock()
	
	log.Println("Hardware: Closing claw")
	
	if c.simulated {
		c.open = false
		c.position = 180
		return nil
	}

	return c.pwm.SetAngle(0, 180) // Channel 0, 180 degrees
}

// SetGripperAngle sets gripper to specific angle
func (c *ClawController) SetGripperAngle(angle int) error {
	c.mu.Lock()
	defer c.mu.Unlock()

	if angle < 0 || angle > 180 {
		return fmt.Errorf("angle out of range: %d (valid: 0-180)", angle)
	}

	log.Printf("Hardware: Setting gripper to %d degrees", angle)

	if c.simulated {
		c.position = angle
		c.open = angle < 90
		return nil
	}

	return c.pwm.SetAngle(0, angle)
}

// Status returns hardware status
func (c *ClawController) Status() string {
	c.mu.RLock()
	defer c.mu.RUnlock()

	if c.simulated {
		return fmt.Sprintf("Hardware Status:\n- Mode: SIMULATED\n- Position: %d°\n- State: %s\n- PCA9685: N/A",
			c.position, map[bool]string{true: "OPEN", false: "CLOSED"}[c.open])
	}

	return fmt.Sprintf("Hardware Status:\n- Mode: HARDWARE\n- Position: %d°\n- State: %s\n- PCA9685: 0x%02X@I2C-%d@%dHz",
		c.position, map[bool]string{true: "OPEN", false: "CLOSED"}[c.open],
		c.cfg.I2CAddress, c.cfg.I2CBus, c.cfg.PWMFrequency)
}

// SetFrequency sets PWM frequency
func (p *PCA9685) SetFrequency(hz int) error {
	// PCA9685 clock is 25MHz
	prescale := 25000000/4096/hz - 1
	if prescale < 3 {
		prescale = 3
	}
	if prescale > 255 {
		prescale = 255
	}

	// Set MODE1 to sleep
	p.writeReg(0x00, 0x10)
	// Set prescale
	p.writeReg(0xFE, uint8(prescale))
	// Wake up MODE1
	p.writeReg(0x00, 0x00)
	time.Sleep(5 * time.Millisecond)
	// Auto increment
	p.writeReg(0x00, 0x21)

	p.frequency = hz
	return nil
}

// SetAngle converts angle to PWM and sets it
func (p *PCA9685) SetAngle(channel, angle int) error {
	// Convert angle (0-180) to PWM pulse width (150-600)
	// 50Hz = 20ms period, 4096 steps
	// 0.5ms = 102 steps (0°), 2.5ms = 512 steps (180°)
	pulse := 102 + angle*410/180
	return p.SetPWM(channel, 0, pulse)
}

// SetPWM sets PWM for a channel
func (p *PCA9685) SetPWM(channel, on, off int) error {
	reg := 0x06 + channel*4
	p.mu.Lock()
	defer p.mu.Unlock()

	// Write in one transaction via ioctl would be better, but for simplicity:
	p.writeReg(reg, uint8(on&0xFF))
	p.writeReg(reg+1, uint8(on>>8))
	p.writeReg(reg+2, uint8(off&0xFF))
	p.writeReg(reg+3, uint8(off>>8))
	return nil
}

// writeReg writes a register via I2C
func (p *PCA9685) writeReg(reg, value uint8) error {
	data := []byte{reg, value}
	_, err := p.file.Write(data)
	return err
}

// Close closes the hardware connection
func (c *ClawController) Close() error {
	c.mu.Lock()
	defer c.mu.Unlock()
	
	if c.pwm != nil && c.pwm.file != nil {
		return c.pwm.file.Close()
	}
	return nil
}

// === Helper functions ===

func exists(path string) bool {
	_, err := os.Stat(path)
	return err == nil
}
