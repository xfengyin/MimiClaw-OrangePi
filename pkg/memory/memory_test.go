// Copyright 2024 MimiClaw Authors
// SPDX-License-Identifier: MIT

package memory

import (
	"os"
	"testing"
)

func TestMemoryStoreRetrieve(t *testing.T) {
	// Create temp file
	tmp := "/tmp/mimiclaw_test_" + randomString(8) + ".db"
	defer os.Remove(tmp)

	mem, err := New(tmp)
	if err != nil {
		t.Fatalf("Failed to create memory: %v", err)
	}
	defer mem.Close()

	// Store
	err = mem.Store("test_key", "test_value")
	if err != nil {
		t.Errorf("Store failed: %v", err)
	}

	// Retrieve
	val, err := mem.Retrieve("test_key")
	if err != nil {
		t.Errorf("Retrieve failed: %v", err)
	}
	if val != `"test_value"` {
		t.Errorf("Expected '\"test_value\"', got '%s'", val)
	}
}

func TestMemoryCount(t *testing.T) {
	tmp := "/tmp/mimiclaw_test_" + randomString(8) + ".db"
	defer os.Remove(tmp)

	mem, err := New(tmp)
	if err != nil {
		t.Fatalf("Failed to create memory: %v", err)
	}
	defer mem.Close()

	before := mem.Count()
	mem.Store("key1", "value1")
	mem.Store("key2", "value2")
	after := mem.Count()

	if after != before+2 {
		t.Errorf("Expected count %d, got %d", before+2, after)
	}
}

func randomString(n int) string {
	const letters = "abcdefghijklmnopqrstuvwxyz"
	b := make([]byte, n)
	for i := range b {
		b[i] = letters[i%len(letters)]
	}
	return string(b)
}
