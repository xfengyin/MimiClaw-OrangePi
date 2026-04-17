// Copyright 2024 MimiClaw Authors
// SPDX-License-Identifier: MIT

package providers

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"strings"
	"time"
)

// Message represents a chat message
type Message struct {
	Role    string `json:"role"`
	Content string `json:"content"`
	Name    string `json:"name,omitempty"`
}

// LLMProvider interface for LLM providers
type LLMProvider interface {
	Chat(ctx context.Context, messages []Message) (string, error)
	Name() string
}

// OpenAIProvider implements OpenAI API
type OpenAIProvider struct {
	apiKey string
	model  string
	client *http.Client
}

// NewOpenAIProvider creates OpenAI provider
func NewOpenAIProvider(apiKey, model string) (*OpenAIProvider, error) {
	if apiKey == "" {
		return nil, fmt.Errorf("API key required")
	}
	return &OpenAIProvider{
		apiKey: apiKey,
		model:  model,
		client: &http.Client{Timeout: 60 * time.Second},
	}, nil
}

// Chat sends chat request to OpenAI
func (p *OpenAIProvider) Chat(ctx context.Context, messages []Message) (string, error) {
	url := "https://api.openai.com/v1/chat/completions"
	
	body := map[string]interface{}{
		"model": p.model,
		"messages": messages,
		"stream": false,
	}
	
	jsonBody, err := json.Marshal(body)
	if err != nil {
		return "", err
	}

	req, err := http.NewRequestWithContext(ctx, "POST", url, strings.NewReader(string(jsonBody)))
	if err != nil {
		return "", err
	}
	
	req.Header.Set("Content-Type", "application/json")
	req.Header.Set("Authorization", "Bearer "+p.apiKey)

	resp, err := p.client.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	respBody, err := io.ReadAll(resp.Body)
	if err != nil {
		return "", err
	}

	if resp.StatusCode != http.StatusOK {
		return "", fmt.Errorf("API error: %s - %s", resp.Status, string(respBody))
	}

	var result map[string]interface{}
	if err := json.Unmarshal(respBody, &result); err != nil {
		return "", err
	}

	choices := result["choices"].([]interface{})
	if len(choices) == 0 {
		return "", fmt.Errorf("no choices in response")
	}

	choice := choices[0].(map[string]interface{})
	msg := choice["message"].(map[string]interface{})
	return msg["content"].(string), nil
}

// Name returns provider name
func (p *OpenAIProvider) Name() string {
	return "openai"
}

// OllamaProvider implements Ollama API
type OllamaProvider struct {
	baseURL string
	model   string
	client  *http.Client
}

// NewOllamaProvider creates Ollama provider
func NewOllamaProvider(baseURL, model string) (*OllamaProvider, error) {
	if baseURL == "" {
		baseURL = "http://localhost:11434"
	}
	if model == "" {
		model = "llama3.2"
	}
	return &OllamaProvider{
		baseURL: baseURL,
		model:   model,
		client:  &http.Client{Timeout: 120 * time.Second},
	}, nil
}

// Chat sends chat request to Ollama
func (p *OllamaProvider) Chat(ctx context.Context, messages []Message) (string, error) {
	url := p.baseURL + "/api/chat"
	
	ollamaMessages := make([]map[string]string, len(messages))
	for i, m := range messages {
		ollamaMessages[i] = map[string]string{
			"role":    m.Role,
			"content": m.Content,
		}
	}
	
	body := map[string]interface{}{
		"model":    p.model,
		"messages": ollamaMessages,
		"stream":   false,
	}
	
	jsonBody, err := json.Marshal(body)
	if err != nil {
		return "", err
	}

	req, err := http.NewRequestWithContext(ctx, "POST", url, strings.NewReader(string(jsonBody)))
	if err != nil {
		return "", err
	}
	req.Header.Set("Content-Type", "application/json")

	resp, err := p.client.Do(req)
	if err != nil {
		return "", err
	}
	defer resp.Body.Close()

	respBody, err := io.ReadAll(resp.Body)
	if err != nil {
		return "", err
	}

	if resp.StatusCode != http.StatusOK {
		return "", fmt.Errorf("Ollama error: %s", string(respBody))
	}

	var result map[string]interface{}
	if err := json.Unmarshal(respBody, &result); err != nil {
		return "", err
	}

	msg := result["message"].(map[string]interface{})
	return msg["content"].(string), nil
}

// Name returns provider name
func (p *OllamaProvider) Name() string {
	return "ollama"
}

// OfflineProvider provides mock responses when no LLM is available
type OfflineProvider struct{}

// NewOfflineProvider creates offline provider
func NewOfflineProvider() *OfflineProvider {
	return &OfflineProvider{}
}

// Chat returns mock response
func (p *OfflineProvider) Chat(ctx context.Context, messages []Message) (string, error) {
	lastMsg := ""
	for i := len(messages) - 1; i >= 0; i-- {
		if messages[i].Role == "user" {
			lastMsg = messages[i].Content
			break
		}
	}

	text := strings.ToLower(lastMsg)
	
	switch {
	case strings.Contains(text, "hello") || strings.Contains(text, "hi"):
		return "Hello! I'm MimiClaw, your AI assistant on OrangePi. How can I help you today?", nil
	case strings.Contains(text, "time"):
		return fmt.Sprintf("The current time is %s", time.Now().Format("15:04:05 MST")), nil
	case strings.Contains(text, "date"):
		return fmt.Sprintf("Today's date is %s", time.Now().Format("2006-01-02")), nil
	case strings.Contains(text, "what can you do"):
		return "I can help you with:\n- Answering questions\n- Controlling the robotic claw\n- Remembering information\n- And more!", nil
	case strings.Contains(text, "claw") || strings.Contains(text, "open") || strings.Contains(text, "close"):
		return "I'll control the hardware claw for you! Use /open or /close commands.", nil
	default:
		return fmt.Sprintf("I received your message: '%s'. I'm running in offline mode. Configure an LLM provider for full functionality.", lastMsg), nil
	}
}

// Name returns provider name
func (p *OfflineProvider) Name() string {
	return "offline"
}
