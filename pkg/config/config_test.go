// Copyright 2024 MimiClaw Authors
// SPDX-License-Identifier: MIT

package config

import (
	"testing"
)

func TestDefaultConfig(t *testing.T) {
	cfg := Default()

	if cfg.Agent.Name != "MimiClaw" {
		t.Errorf("Expected agent name 'MimiClaw', got '%s'", cfg.Agent.Name)
	}

	if cfg.Memory.Path != "./data/memory.db" {
		t.Errorf("Expected memory path './data/memory.db', got '%s'", cfg.Memory.Path)
	}

	if cfg.Hardware.I2CAddress != 0x40 {
		t.Errorf("Expected I2C address 0x40, got 0x%02X", cfg.Hardware.I2CAddress)
	}
}

func TestMemoryConfig(t *testing.T) {
	cfg := Default()

	if cfg.Memory.Type != "sqlite" {
		t.Errorf("Expected memory type 'sqlite', got '%s'", cfg.Memory.Type)
	}

	if cfg.Memory.MaxMemory != 1000 {
		t.Errorf("Expected max memory 1000, got %d", cfg.Memory.MaxMemory)
	}
}
