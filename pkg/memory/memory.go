// Copyright 2024 MimiClaw Authors
// SPDX-License-Identifier: MIT

package memory

import (
	"database/sql"
	"encoding/json"
	"fmt"
	"log"
	"os"
	"path/filepath"
	"time"

	_ "github.com/mattn/go-sqlite3"
)

// Entry represents a memory entry
type Entry struct {
	ID        int64
	Key       string
	Value     string
	Timestamp float64
	Metadata  string
}

// Memory provides persistent key-value storage
type Memory struct {
	db  *sql.DB
	path string
}

// New creates a new memory instance
func New(path string) (*Memory, error) {
	// Create directory if not exists
	dir := filepath.Dir(path)
	if err := os.MkdirAll(dir, 0755); err != nil {
		return nil, fmt.Errorf("failed to create directory: %w", err)
	}

	// Connect to SQLite
	db, err := sql.Open("sqlite3", path+"?_journal_mode=WAL&_synchronous=NORMAL&_cache_size=-64000")
	if err != nil {
		return nil, fmt.Errorf("failed to open database: %w", err)
	}

	// Test connection
	if err := db.Ping(); err != nil {
		return nil, fmt.Errorf("failed to ping database: %w", err)
	}

	m := &Memory{db: db, path: path}
	m.createTables()

	log.Printf("Memory initialized: %s", path)
	return m, nil
}

// createTables creates necessary tables
func (m *Memory) createTables() {
	m.db.Exec(`
		CREATE TABLE IF NOT EXISTS memory (
			id INTEGER PRIMARY KEY AUTOINCREMENT,
			key TEXT UNIQUE NOT NULL,
			value TEXT NOT NULL,
			timestamp REAL NOT NULL,
			metadata TEXT DEFAULT '{}'
		)
	`)
	m.db.Exec(`CREATE INDEX IF NOT EXISTS idx_timestamp ON memory(timestamp)`)
	m.db.Exec(`CREATE INDEX IF NOT EXISTS idx_key ON memory(key)`)
}

// Store stores a value
func (m *Memory) Store(key string, value interface{}) error {
	jsonValue, err := json.Marshal(value)
	if err != nil {
		return err
	}

	_, err = m.db.Exec(`
		INSERT OR REPLACE INTO memory (key, value, timestamp)
		VALUES (?, ?, ?)
	`, key, string(jsonValue), time.Now().UnixNano()/1e6)
	return err
}

// Retrieve retrieves a value
func (m *Memory) Retrieve(key string) (string, error) {
	var value string
	err := m.db.QueryRow("SELECT value FROM memory WHERE key = ?", key).Scan(&value)
	if err == sql.ErrNoRows {
		return "", nil
	}
	return value, err
}

// Delete deletes a key
func (m *Memory) Delete(key string) error {
	_, err := m.db.Exec("DELETE FROM memory WHERE key = ?", key)
	return err
}

// Recent returns recent entries
func (m *Memory) Recent(limit int) []Entry {
	rows, err := m.db.Query(`
		SELECT id, key, value, timestamp, metadata 
		FROM memory 
		ORDER BY timestamp DESC 
		LIMIT ?
	`, limit)
	if err != nil {
		return nil
	}
	defer rows.Close()

	var entries []Entry
	for rows.Next() {
		var e Entry
		if err := rows.Scan(&e.ID, &e.Key, &e.Value, &e.Timestamp, &e.Metadata); err != nil {
			continue
		}
		entries = append(entries, e)
	}
	return entries
}

// Search searches for entries
func (m *Memory) Search(pattern string) []Entry {
	rows, err := m.db.Query(`
		SELECT id, key, value, timestamp, metadata 
		FROM memory 
		WHERE key LIKE ? 
		ORDER BY timestamp DESC
	`, "%"+pattern+"%")
	if err != nil {
		return nil
	}
	defer rows.Close()

	var entries []Entry
	for rows.Next() {
		var e Entry
		if err := rows.Scan(&e.ID, &e.Key, &e.Value, &e.Timestamp, &e.Metadata); err != nil {
			continue
		}
		entries = append(entries, e)
	}
	return entries
}

// Count returns number of entries
func (m *Memory) Count() int {
	var count int
	m.db.QueryRow("SELECT COUNT(*) FROM memory").Scan(&count)
	return count
}

// Clear removes all entries
func (m *Memory) Clear() error {
	_, err := m.db.Exec("DELETE FROM memory")
	return err
}

// Close closes the database
func (m *Memory) Close() error {
	return m.db.Close()
}

// Stats returns memory statistics
func (m *Memory) Stats() map[string]interface{} {
	var count int
	var size int64
	m.db.QueryRow("SELECT COUNT(*) FROM memory").Scan(&count)
	
	if info, err := os.Stat(m.path); err == nil {
		size = info.Size()
	}

	return map[string]interface{}{
		"entries":  count,
		"file_size": size,
		"path":      m.path,
	}
}
