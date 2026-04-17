// Copyright 2024 MimiClaw Authors
// SPDX-License-Identifier: MIT

package agent

import (
	"context"
	"fmt"
	"strings"
	"time"

	"github.com/xfengyin/mimiclaw/pkg/config"
	"github.com/xfengyin/mimiclaw/pkg/hardware"
	"github.com/xfengyin/mimiclaw/pkg/memory"
	"github.com/xfengyin/mimiclaw/pkg/providers"
	"github.com/xfengyin/mimiclaw/pkg/tools"
)

// Agent is the main AI agent
type Agent struct {
	cfg      *config.AgentConfig
	memory   *memory.Memory
	llm      providers.LLMProvider
	claw     *hardware.ClawController
	tools    *tools.Registry
	systemPrompt string
}

// New creates a new agent
func New(
	cfg *config.AgentConfig,
	mem *memory.Memory,
	llm providers.LLMProvider,
	claw *hardware.ClawController,
	toolReg *tools.Registry,
) *Agent {
	return &Agent{
		cfg:         cfg,
		memory:      mem,
		llm:         llm,
		claw:        claw,
		tools:       toolReg,
		systemPrompt: cfg.SystemPrompt,
	}
}

// Run processes user input and returns response
func (a *Agent) Run(ctx context.Context, input string) (string, error) {
	// Check for tool calls
	if strings.HasPrefix(input, "/") {
		return a.handleCommand(ctx, input)
	}

	// Retrieve relevant memories
	contexts := a.memory.Recent(5)

	// Build messages
	messages := []providers.Message{
		{Role: "system", Content: a.systemPrompt},
	}
	
	// Add memory context
	if len(contexts) > 0 {
		memContext := "Recent memories:\n"
		for _, c := range contexts {
			memContext += fmt.Sprintf("- %s: %v\n", c.Key, c.Value)
		}
		messages = append(messages, providers.Message{
			Role:    "system",
			Content: memContext,
		})
	}

	messages = append(messages, providers.Message{
		Role:    "user",
		Content: input,
	})

	// Get LLM response
	resp, err := a.llm.Chat(ctx, messages)
	if err != nil {
		return "", fmt.Errorf("LLM error: %w", err)
	}

	// Check for hardware commands in response
	a.handleHardwareCommands(resp)

	// Store in memory
	key := fmt.Sprintf("user_%d", time.Now().Unix())
	a.memory.Store(key, input)
	key = fmt.Sprintf("agent_%d", time.Now().Unix())
	a.memory.Store(key, resp)

	return resp, nil
}

// handleCommand processes slash commands
func (a *Agent) handleCommand(ctx context.Context, cmd string) (string, error) {
	parts := strings.SplitN(strings.TrimPrefix(cmd, "/"), " ", 2)
	command := parts[0]
	args := ""
	if len(parts) > 1 {
		args = parts[1]
	}

	switch command {
	case "open":
		if err := a.claw.Open(); err != nil {
			return "", err
		}
		return "Claw opened!", nil
	case "close":
		if err := a.claw.Close(); err != nil {
			return "", err
		}
		return "Claw closed!", nil
	case "grip":
		pos := 90
		if args != "" {
			fmt.Sscanf(args, "%d", &pos)
		}
		if err := a.claw.SetGripperAngle(pos); err != nil {
			return "", err
		}
		return fmt.Sprintf("Gripper set to %d°", pos), nil
	case "status":
		return a.claw.Status(), nil
	case "memory":
		count := a.memory.Count()
		return fmt.Sprintf("Memory entries: %d", count), nil
	case "clear":
		a.memory.Clear()
		return "Memory cleared!", nil
	case "help":
		return a.helpText(), nil
	default:
		return fmt.Sprintf("Unknown command: /%s", command), nil
	}
}

// handleHardwareCommands checks response for hardware commands
func (a *Agent) handleHardwareCommands(response string) {
	text := strings.ToLower(response)
	if strings.Contains(text, "opening claw") || strings.Contains(text, "open the claw") {
		a.claw.Open()
	}
	if strings.Contains(text, "closing claw") || strings.Contains(text, "close the claw") {
		a.claw.Close()
	}
}

// helpText returns help information
func (a *Agent) helpText() string {
	return `
Available commands:
  /open           - Open the claw
  /close          - Close the claw
  /grip [angle]   - Set gripper angle (0-180°)
  /status         - Show hardware status
  /memory         - Show memory count
  /clear          - Clear memory
  /help           - Show this help

Hardware commands: open, close, gripper control
`
}
