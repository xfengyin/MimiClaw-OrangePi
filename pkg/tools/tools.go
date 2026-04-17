// Copyright 2024 MimiClaw Authors
// SPDX-License-Identifier: MIT

package tools

import (
	"context"
	"fmt"
)

// Tool represents a callable tool
type Tool struct {
	Name        string
	Description string
	Handler     func(ctx context.Context, args map[string]interface{}) (interface{}, error)
	Parameters  map[string]string
}

// Registry manages tool registrations
type Registry struct {
	tools map[string]*Tool
}

// NewRegistry creates a new tool registry
func NewRegistry() *Registry {
	return &Registry{
		tools: make(map[string]*Tool),
	}
}

// Register registers a tool
func (r *Registry) Register(tool Tool) {
	r.tools[tool.Name] = &tool
}

// Get retrieves a tool
func (r *Registry) Get(name string) (*Tool, bool) {
	tool, ok := r.tools[name]
	return tool, ok
}

// List returns all registered tools
func (r *Registry) List() []*Tool {
	tools := make([]*Tool, 0, len(r.tools))
	for _, t := range r.tools {
		tools = append(tools, t)
	}
	return tools
}

// Call calls a tool by name
func (r *Registry) Call(ctx context.Context, name string, args map[string]interface{}) (interface{}, error) {
	tool, ok := r.Get(name)
	if !ok {
		return nil, fmt.Errorf("tool not found: %s", name)
	}
	return tool.Handler(ctx, args)
}

// Count returns number of registered tools
func (r *Registry) Count() int {
	return len(r.tools)
}
