// Copyright 2024 MimiClaw Authors
// SPDX-License-Identifier: MIT

package config

import (
	"os"

	"gopkg.in/yaml.v3"
)

// Config represents the application configuration
type Config struct {
	Agent    AgentConfig    `yaml:"agent"`
	Memory   MemoryConfig   `yaml:"memory"`
	Hardware HardwareConfig `yaml:"hardware"`
	Provider ProviderConfig `yaml:"provider"`
	Logging  LoggingConfig  `yaml:"logging"`
}

// AgentConfig for AI agent settings
type AgentConfig struct {
	Name         string  `yaml:"name"`
	Model        string  `yaml:"model"`
	MaxTokens    int     `yaml:"max_tokens"`
	Temperature  float64 `yaml:"temperature"`
	SystemPrompt string  `yaml:"system_prompt"`
}

// MemoryConfig for persistent memory
type MemoryConfig struct {
	Type      string `yaml:"type"`
	Path      string `yaml:"path"`
	VectorDim int    `yaml:"vector_dim"`
	MaxMemory int    `yaml:"max_memory"`
}

// HardwareConfig for hardware control
type HardwareConfig struct {
	Enabled      bool   `yaml:"enabled"`
	I2CBus       int    `yaml:"i2c_bus"`
	I2CAddress   int    `yaml:"i2c_address"`
	PWMFrequency int    `yaml:"pwm_frequency"`
	Simulated    bool   `yaml:"simulated"`
}

// ProviderConfig for LLM provider
type ProviderConfig struct {
	Type    string `yaml:"type"`
	APIKey  string `yaml:"api_key"`
	Model   string `yaml:"model"`
	BaseURL string `yaml:"base_url"`
}

// LoggingConfig for logging settings
type LoggingConfig struct {
	Level  string `yaml:"level"`
	File   string `yaml:"file"`
	Format string `yaml:"format"`
}

// Default returns default configuration
func Default() *Config {
	return &Config{
		Agent: AgentConfig{
			Name:         "MimiClaw",
			Model:        "gpt-4o-mini",
			MaxTokens:    512,
			Temperature:  0.7,
			SystemPrompt: "You are MimiClaw, an AI assistant running on OrangePi hardware.",
		},
		Memory: MemoryConfig{
			Type:      "sqlite",
			Path:      "./data/memory.db",
			VectorDim: 384,
			MaxMemory: 1000,
		},
		Hardware: HardwareConfig{
			Enabled:      true,
			I2CBus:       0,
			I2CAddress:   0x40,
			PWMFrequency: 50,
			Simulated:    false,
		},
		Provider: ProviderConfig{
			Type:    "openai",
			APIKey:  "",
			Model:   "gpt-4o-mini",
			BaseURL: "https://api.openai.com/v1",
		},
		Logging: LoggingConfig{
			Level:  "info",
			Format: "text",
		},
	}
}

// Load loads configuration from file
func Load(path string) (*Config, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return nil, err
	}

	var cfg Config
	if err := yaml.Unmarshal(data, &cfg); err != nil {
		return nil, err
	}

	return &cfg, nil
}

// Save saves configuration to file
func (c *Config) Save(path string) error {
	data, err := yaml.Marshal(c)
	if err != nil {
		return err
	}
	return os.WriteFile(path, data, 0644)
}
