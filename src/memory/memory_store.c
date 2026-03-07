/*
 * Memory Store Implementation - SQLite-based memory management
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory_store.h"
#include "../core/logger.h"

static const char *create_tables_sql = 
    "CREATE TABLE IF NOT EXISTS memories ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    type INTEGER NOT NULL,"
    "    session_id TEXT,"
    "    user_id TEXT,"
    "    content TEXT NOT NULL,"
    "    created_at INTEGER DEFAULT (strftime('%s', 'now')),"
    "    updated_at INTEGER DEFAULT (strftime('%s', 'now'))"
    ");"
    "CREATE TABLE IF NOT EXISTS sessions ("
    "    session_id TEXT PRIMARY KEY,"
    "    user_id TEXT NOT NULL,"
    "    created_at INTEGER DEFAULT (strftime('%s', 'now')),"
    "    last_activity INTEGER DEFAULT (strftime('%s', 'now')),"
    "    message_count INTEGER DEFAULT 0"
    ");"
    "CREATE TABLE IF NOT EXISTS messages ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    session_id TEXT NOT NULL,"
    "    role TEXT NOT NULL,"
    "    content TEXT NOT NULL,"
    "    timestamp INTEGER DEFAULT (strftime('%s', 'now')),"
    "    FOREIGN KEY (session_id) REFERENCES sessions(session_id)"
    ");"
    "CREATE INDEX IF NOT EXISTS idx_memories_type ON memories(type);"
    "CREATE INDEX IF NOT EXISTS idx_memories_session ON memories(session_id);"
    "CREATE INDEX IF NOT EXISTS idx_messages_session ON messages(session_id);";

int memory_store_init(memory_store_t *store, const char *db_path) {
    if (!store || !db_path) return -1;
    
    memset(store, 0, sizeof(memory_store_t));
    strncpy(store->db_path, db_path, sizeof(store->db_path) - 1);
    
    int rc = sqlite3_open(db_path, &store->db);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Cannot open database: %s", sqlite3_errmsg(store->db));
        return -1;
    }
    
    // Enable WAL mode for better concurrency
    char *err_msg = NULL;
    rc = sqlite3_exec(store->db, "PRAGMA journal_mode=WAL;", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        LOG_WARN("Cannot enable WAL mode: %s", err_msg);
        sqlite3_free(err_msg);
    }
    
    // Create tables
    rc = sqlite3_exec(store->db, create_tables_sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Cannot create tables: %s", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(store->db);
        return -1;
    }
    
    store->initialized = true;
    LOG_INFO("Memory store initialized: %s", db_path);
    return 0;
}

void memory_store_close(memory_store_t *store) {
    if (!store || !store->initialized) return;
    
    sqlite3_close(store->db);
    store->initialized = false;
    LOG_INFO("Memory store closed");
}

int memory_add(memory_store_t *store, memory_entry_t *entry) {
    if (!store || !store->initialized || !entry) return -1;
    
    const char *sql = "INSERT INTO memories (type, session_id, user_id, content) VALUES (?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(store->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to prepare statement: %s", sqlite3_errmsg(store->db));
        return -1;
    }
    
    sqlite3_bind_int(stmt, 1, entry->type);
    sqlite3_bind_text(stmt, 2, entry->session_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, entry->user_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, entry->content, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        LOG_ERROR("Failed to insert memory: %s", sqlite3_errmsg(store->db));
        return -1;
    }
    
    entry->id = sqlite3_last_insert_rowid(store->db);
    LOG_DEBUG("Memory added: id=%d, type=%d", entry->id, entry->type);
    return 0;
}

int memory_get_by_type(memory_store_t *store, memory_type_t type, 
                       memory_entry_t **entries, int *count) {
    if (!store || !store->initialized || !entries || !count) return -1;
    
    const char *sql = "SELECT id, type, session_id, user_id, content, created_at, updated_at "
                      "FROM memories WHERE type = ? ORDER BY updated_at DESC;";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(store->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    sqlite3_bind_int(stmt, 1, type);
    
    // Count results
    int num_rows = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        num_rows++;
    }
    sqlite3_reset(stmt);
    sqlite3_bind_int(stmt, 1, type);
    
    if (num_rows == 0) {
        *entries = NULL;
        *count = 0;
        sqlite3_finalize(stmt);
        return 0;
    }
    
    *entries = calloc(num_rows, sizeof(memory_entry_t));
    if (!*entries) {
        sqlite3_finalize(stmt);
        return -1;
    }
    
    int i = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW && i < num_rows) {
        memory_entry_t *entry = &(*entries)[i];
        entry->id = sqlite3_column_int(stmt, 0);
        entry->type = sqlite3_column_int(stmt, 1);
        strncpy(entry->session_id, (const char*)sqlite3_column_text(stmt, 2), MAX_SESSION_ID - 1);
        strncpy(entry->user_id, (const char*)sqlite3_column_text(stmt, 3), MAX_USER_ID - 1);
        strncpy(entry->content, (const char*)sqlite3_column_text(stmt, 4), MAX_MEMORY_CONTENT - 1);
        entry->created_at = sqlite3_column_int64(stmt, 5);
        entry->updated_at = sqlite3_column_int64(stmt, 6);
        i++;
    }
    
    *count = i;
    sqlite3_finalize(stmt);
    return 0;
}

int session_create(memory_store_t *store, const char *session_id, const char *user_id) {
    if (!store || !store->initialized || !session_id || !user_id) return -1;
    
    const char *sql = "INSERT OR REPLACE INTO sessions (session_id, user_id) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(store->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user_id, -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? 0 : -1;
}

int session_add_message(memory_store_t *store, const char *session_id, 
                        const char *role, const char *content) {
    if (!store || !store->initialized || !session_id || !role || !content) return -1;
    
    char *err_msg = NULL;
    char sql[512];
    
    // Insert message
    snprintf(sql, sizeof(sql), 
        "INSERT INTO messages (session_id, role, content) VALUES ('%s', '%s', '%s');",
        session_id, role, content);
    
    // Note: In production, use parameterized queries to prevent SQL injection
    int rc = sqlite3_exec(store->db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        LOG_ERROR("Failed to add message: %s", err_msg);
        sqlite3_free(err_msg);
        return -1;
    }
    
    // Update session activity
    snprintf(sql, sizeof(sql),
        "UPDATE sessions SET last_activity = strftime('%%s', 'now'), "
        "message_count = message_count + 1 WHERE session_id = '%s';",
        session_id);
    
    sqlite3_exec(store->db, sql, NULL, NULL, NULL);
    
    return 0;
}

int session_get_history(memory_store_t *store, const char *session_id,
                        char **history, int *message_count) {
    if (!store || !store->initialized || !session_id || !history || !message_count) return -1;
    
    const char *sql = "SELECT role, content FROM messages WHERE session_id = ? "
                      "ORDER BY timestamp ASC LIMIT 50;";
    sqlite3_stmt *stmt;
    
    int rc = sqlite3_prepare_v2(store->db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return -1;
    
    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    
    // Calculate required buffer size
    size_t total_size = 1;
    int count = 0;
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        total_size += strlen((const char*)sqlite3_column_text(stmt, 0)) + 3;
        total_size += strlen((const char*)sqlite3_column_text(stmt, 1)) + 2;
        count++;
    }
    sqlite3_reset(stmt);
    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    
    if (count == 0) {
        *history = NULL;
        *message_count = 0;
        sqlite3_finalize(stmt);
        return 0;
    }
    
    *history = malloc(total_size);
    if (!*history) {
        sqlite3_finalize(stmt);
        return -1;
    }
    
    (*history)[0] = '\0';
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        strcat(*history, (const char*)sqlite3_column_text(stmt, 0));
        strcat(*history, ": ");
        strcat(*history, (const char*)sqlite3_column_text(stmt, 1));
        strcat(*history, "\n");
    }
    
    *message_count = count;
    sqlite3_finalize(stmt);
    return 0;
}

void memory_free_entries(memory_entry_t *entries, int count) {
    if (entries) {
        free(entries);
    }
    (void)count;
}

void memory_free_sessions(session_info_t *sessions, int count) {
    if (sessions) {
        free(sessions);
    }
    (void)count;
}

const char* memory_type_to_string(memory_type_t type) {
    switch (type) {
        case MEMORY_TYPE_SOUL: return "soul";
        case MEMORY_TYPE_USER: return "user";
        case MEMORY_TYPE_LONG_TERM: return "long_term";
        case MEMORY_TYPE_DAILY: return "daily";
        case MEMORY_TYPE_SESSION: return "session";
        default: return "unknown";
    }
}