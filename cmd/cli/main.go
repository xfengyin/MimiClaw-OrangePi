// Copyright 2024 MimiClaw Authors
// SPDX-License-Identifier: MIT

package main

import (
	"context"
	"fmt"
	"log"
	"os"
	"os/signal"
	"path/filepath"
	"syscall"
	"time"

	"github.com/xfengyin/mimiclaw/pkg/agent"
	"github.com/xfengyin/mimiclaw/pkg/config"
	"github.com/xfengyin/mimiclaw/pkg/hardware"
	"github.com/xfengyin/mimiclaw/pkg/memory"
	"github.com/xfengyin/mimiclaw/pkg/providers"
	"github.com/xfengyin/mimiclaw/pkg/tools"
)

// Version information (set by goreleaser)
var (
	version   = "dev"
	commit    = "none"
	date      = "unknown"
	builtBy   = "unknown"
)

const (
	appName    = "MimiClaw"
	configFile = "config.yaml"
)

func main() {
	fmt.Println(banner())
	fmt.Printf("Version: %s (%s)\n", version, commit)
	fmt.Printf("Built: %s by %s\n\n", date, builtBy)

	// Detect platform
	hwInfo := hardware.Detect()
	fmt.Printf("Platform: %s\n", hwInfo.Model)
	fmt.Printf("CPU Cores: %d | RAM: %.1fGB | NEON: %v\n\n",
		hwInfo.Cores, hwInfo.TotalRAM, hwInfo.HasNEON)

	// Load configuration
	cfg, err := loadConfig()
	if err != nil {
		log.Fatalf("Failed to load config: %v", err)
	}

	// Initialize memory
	mem, err := memory.New(cfg.Memory.Path)
	if err != nil {
		log.Fatalf("Failed to initialize memory: %v", err)
	}
	defer mem.Close()

	// Initialize hardware
	claw, err := hardware.NewClawController(cfg.Hardware)
	if err != nil {
		log.Printf("Warning: Hardware init failed: %v (continuing in simulation mode)", err)
		claw = hardware.NewSimulatedClaw()
	}

	// Initialize LLM provider
	var llm providers.LLMProvider
	if cfg.Provider.Type == "openai" {
		llm, err = providers.NewOpenAIProvider(cfg.Provider.APIKey, cfg.Provider.Model)
	} else if cfg.Provider.Type == "ollama" {
		llm, err = providers.NewOllamaProvider(cfg.Provider.BaseURL, cfg.Provider.Model)
	}
	if err != nil {
		log.Printf("Warning: LLM provider init failed: %v (running in offline mode)", err)
		llm = providers.NewOfflineProvider()
	}

	// Initialize tools
	toolRegistry := tools.NewRegistry()
	registerTools(toolRegistry)

	// Create agent
	aiAgent := agent.New(cfg.Agent, mem, llm, claw, toolRegistry)

	// Setup graceful shutdown
	ctx, cancel := context.WithCancel(context.Background())
	defer cancel()

	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	// Parse mode from args
	mode := "interactive"
	if len(os.Args) > 1 {
		mode = os.Args[1]
	}

	switch mode {
	case "demo":
		runDemo(ctx, aiAgent)
	case "agent", "interactive":
		runInteractive(ctx, aiAgent)
	case "gateway":
		runGateway(ctx, aiAgent)
	case "version":
		fmt.Printf("MimiClaw %s\n", version)
	default:
		fmt.Printf("Unknown mode: %s\n", mode)
		fmt.Println("Usage: mimiclaw [demo|agent|gateway]")
	}

	// Wait for shutdown signal
	select {
	case sig := <-sigChan:
		fmt.Printf("\nReceived signal: %v\n", sig)
		cancel()
	case <-ctx.Done():
	}
}

func loadConfig() (*config.Config, error) {
	// Try current directory first
	cfgPath := configFile
	if _, err := os.Stat(cfgPath); os.IsNotExist(err) {
		// Try home directory
		home, _ := os.UserHomeDir()
		cfgPath = filepath.Join(home, ".mimiclaw", configFile)
	}

	if _, err := os.Stat(cfgPath); os.IsNotExist(err) {
		// Create default config
		return config.Default(), nil
	}

	return config.Load(cfgPath)
}

func runDemo(ctx context.Context, aiAgent *agent.Agent) {
	fmt.Println("=== MimiClaw Demo Mode ===")
	queries := []string{
		"Hello, what can you do?",
		"What time is it?",
		"Open the claw",
		"Close the claw",
	}

	for _, query := range queries {
		fmt.Printf("\n[You] %s\n", query)
		resp, err := aiAgent.Run(ctx, query)
		if err != nil {
			fmt.Printf("[Error] %v\n", err)
		} else {
			fmt.Printf("[MimiClaw] %s\n", resp)
		}
		time.Sleep(500 * time.Millisecond)
	}
	fmt.Println("\n=== Demo Complete ===")
}

func runInteractive(ctx context.Context, aiAgent *agent.Agent) {
	fmt.Println("=== MimiClaw Interactive Mode ===")
	fmt.Println("Type 'exit' or 'quit' to end\n")

	for {
		fmt.Print("[You] ")
		var input string
		n, err := fmt.Scanln(&input)
		if err != nil || n == 0 {
			continue
		}

		input = trimNewline(input)
		if input == "" {
			continue
		}

		switch input {
		case "exit", "quit", "bye":
			fmt.Println("[MimiClaw] Goodbye! Have a great day!")
			return
		}

		resp, err := aiAgent.Run(ctx, input)
		if err != nil {
			fmt.Printf("[Error] %v\n", err)
		} else {
			fmt.Printf("[MimiClaw] %s\n", resp)
		}
	}
}

func runGateway(ctx context.Context, aiAgent *agent.Agent) {
	fmt.Println("=== MimiClaw Gateway Mode ===")
	fmt.Println("Gateway not yet implemented")
	<-ctx.Done()
}

func registerTools(reg *tools.Registry) {
	// Time tool
	reg.Register(tools.Tool{
		Name:        "get_time",
		Description: "Get the current time",
		Handler: func(ctx context.Context, args map[string]interface{}) (interface{}, error) {
			return time.Now().Format("15:04:05 MST"), nil
		},
	})

	// Date tool
	reg.Register(tools.Tool{
		Name:        "get_date",
		Description: "Get the current date",
		Handler: func(ctx context.Context, args map[string]interface{}) (interface{}, error) {
			return time.Now().Format("2006-01-02"), nil
		},
	})

	// Weather tool (placeholder)
	reg.Register(tools.Tool{
		Name:        "weather",
		Description: "Get weather information",
		Handler: func(ctx context.Context, args map[string]interface{}) (interface{}, error) {
			return "Sunny, 25°C", nil
		},
	})
}

func banner() string {
	return `
╔═══════════════════════════════════════════════╗
║   ____  _          _ _    _     _   _  ____    ║
║  / ___|| |__   ___| | |  (_)___| |_| |/ ___|   ║
║  \___ \| '_ \ / _ \ | |  | / __| __| |\___ \   ║
║   ___) | | | |  __/ | |__| \__ \ |_| | ___) |  ║
║  |____/|_| |_|\___|_|____/_|___/\__|_||____/   ║
║                                               ║
║   Ultra-Lightweight AI Assistant for OrangePi ║
╚═══════════════════════════════════════════════╝
`
}

func trimNewline(s string) string {
	for len(s) > 0 && (s[len(s)-1] == '\n' || s[len(s)-1] == '\r') {
		s = s[:len(s)-1]
	}
	return s
}
