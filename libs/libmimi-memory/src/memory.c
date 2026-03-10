/**
 * @file memory.c
 * @brief MimiClaw Memory Library Implementation
 * @version 2.0.0
 */

#define _GNU_SOURCE

#include "mimi_memory.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sqlite3.h>
#include <time.h>

/* ============================================================================
 * Internal Structures
 * ============================================================================ */

/**
 * @brief Prepared statements cache for a connection
 */
typedef struct {
    sqlite3_stmt *stmt_session_create;
    sqlite3_stmt *stmt_session_delete;
    sqlite3_stmt *stmt_session_list;
    sqlite3_stmt *stmt_session_exists;
    sqlite3_stmt *stmt_message_append;
    sqlite3_stmt *stmt_message_query;
    sqlite3_stmt *stmt_message_clear;
    sqlite3_stmt *stmt_memory_write;
    sqlite3_stmt *stmt_memory_read;
    sqlite3_stmt *stmt_memory_delete;
    sqlite3_stmt *stmt_memory_search;
} mimi_mem_stmt_cache_t;

/**
 * @brief Connection pool slot
 */
typedef struct {
    sqlite3 *db;
    mimi_mem_stmt_cache_t stmts;
    int in_use;
    int64_t last_used;
} mimi_mem_conn_slot_t;

/**
 * @brief Internal memory pool structure
 */
struct mimi_mem_pool {
    mimi_mem_config_t config;
    mimi_mem_conn_slot_t *connections;
    int pool_size;
    char last_error[512];
};

/* ============================================================================
 * Internal Helper Functions
 * ============================================================================ */

static int64_t get_timestamp_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int init_statements(sqlite3 *db, mimi_mem_stmt_cache_t *stmts)
{
    int ret;
    
    /* Session statements */
    ret = sqlite3_prepare_v2(db, 
        "INSERT OR IGNORE INTO sessions (session_id) VALUES (?)",
        -1, &stmts->stmt_session_create, NULL);
    if (ret != SQLITE_OK) return -1;
    
    ret = sqlite3_prepare_v2(db,
        "DELETE FROM sessions WHERE session_id = ?",
        -1, &stmts->stmt_session_delete, NULL);
    if (ret != SQLITE_OK) return -1;
    
    ret = sqlite3_prepare_v2(db,
        "SELECT session_id FROM sessions ORDER BY created_at DESC",
        -1, &stmts->stmt_session_list, NULL);
    if (ret != SQLITE_OK) return -1;
    
    ret = sqlite3_prepare_v2(db,
        "SELECT 1 FROM sessions WHERE session_id = ? LIMIT 1",
        -1, &stmts->stmt_session_exists, NULL);
    if (ret != SQLITE_OK) return -1;
    
    /* Message statements */
    ret = sqlite3_prepare_v2(db,
        "INSERT INTO messages (session_id, role, content) VALUES (?, ?, ?)",
        -1, &stmts->stmt_message_append, NULL);
    if (ret != SQLITE_OK) return -1;
    
    ret = sqlite3_prepare_v2(db,
        "SELECT msg_id, session_id, role, content, timestamp FROM messages "
        "WHERE session_id = ? ORDER BY timestamp ASC LIMIT ?",
        -1, &stmts->stmt_message_query, NULL);
    if (ret != SQLITE_OK) return -1;
    
    ret = sqlite3_prepare_v2(db,
        "DELETE FROM messages WHERE session_id = ?",
        -1, &stmts->stmt_message_clear, NULL);
    if (ret != SQLITE_OK) return -1;
    
    /* Memory statements */
    ret = sqlite3_prepare_v2(db,
        "INSERT INTO memory (key, value, created_at, updated_at) "
        "VALUES (?, ?, strftime('%s', 'now') * 1000, strftime('%s', 'now') * 1000) "
        "ON CONFLICT(key) DO UPDATE SET "
        "value = excluded.value, updated_at = strftime('%s', 'now') * 1000",
        -1, &stmts->stmt_memory_write, NULL);
    if (ret != SQLITE_OK) return -1;
    
    ret = sqlite3_prepare_v2(db,
        "SELECT value FROM memory WHERE key = ?",
        -1, &stmts->stmt_memory_read, NULL);
    if (ret != SQLITE_OK) return -1;
    
    ret = sqlite3_prepare_v2(db,
        "DELETE FROM memory WHERE key = ?",
        -1, &stmts->stmt_memory_delete, NULL);
    if (ret != SQLITE_OK) return -1;
    
    ret = sqlite3_prepare_v2(db,
        "SELECT key, value, created_at, updated_at FROM memory "
        "WHERE key LIKE ? OR value LIKE ? ORDER BY updated_at DESC",
        -1, &stmts->stmt_memory_search, NULL);
    if (ret != SQLITE_OK) return -1;
    
    return 0;
}

static void finalize_statements(mimi_mem_stmt_cache_t *stmts)
{
    if (stmts == NULL) return;
    
    if (stmts->stmt_session_create) sqlite3_finalize(stmts->stmt_session_create);
    if (stmts->stmt_session_delete) sqlite3_finalize(stmts->stmt_session_delete);
    if (stmts->stmt_session_list) sqlite3_finalize(stmts->stmt_session_list);
    if (stmts->stmt_session_exists) sqlite3_finalize(stmts->stmt_session_exists);
    if (stmts->stmt_message_append) sqlite3_finalize(stmts->stmt_message_append);
    if (stmts->stmt_message_query) sqlite3_finalize(stmts->stmt_message_query);
    if (stmts->stmt_message_clear) sqlite3_finalize(stmts->stmt_message_clear);
    if (stmts->stmt_memory_write) sqlite3_finalize(stmts->stmt_memory_write);
    if (stmts->stmt_memory_read) sqlite3_finalize(stmts->stmt_memory_read);
    if (stmts->stmt_memory_delete) sqlite3_finalize(stmts->stmt_memory_delete);
    if (stmts->stmt_memory_search) sqlite3_finalize(stmts->stmt_memory_search);
}

static int init_database(sqlite3 *db)
{
    char *err_msg = NULL;
    int ret;
    
    /* SQLite 性能优化 */
    
    /* 1. WAL 模式 - 提升并发写入性能 */
    ret = sqlite3_exec(db, "PRAGMA journal_mode=WAL", NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        sqlite3_free(err_msg);
        return -1;
    }
    
    /* 2. 同步模式优化 - NORMAL 平衡性能与安全 */
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL", NULL, NULL, NULL);
    
    /* 3. 缓存大小 - 2000 页 (约 8MB) */
    sqlite3_exec(db, "PRAGMA cache_size=-2000", NULL, NULL, NULL);
    
    /* 4. 临时存储内存中 */
    sqlite3_exec(db, "PRAGMA temp_store=MEMORY", NULL, NULL, NULL);
    
    /* 5. 批量写入时关闭自动提交 (由事务控制) */
    sqlite3_exec(db, "PRAGMA locking_mode=EXCLUSIVE", NULL, NULL, NULL);
    
    /* Create sessions table */
    const char *sql_sessions = 
        "CREATE TABLE IF NOT EXISTS sessions ("
        "  session_id TEXT PRIMARY KEY,"
        "  created_at INTEGER DEFAULT (strftime('%s', 'now') * 1000)"
        ")";
    
    ret = sqlite3_exec(db, sql_sessions, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        sqlite3_free(err_msg);
        return -1;
    }
    
    /* Create messages table */
    const char *sql_messages =
        "CREATE TABLE IF NOT EXISTS messages ("
        "  msg_id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  session_id TEXT NOT NULL,"
        "  role TEXT NOT NULL,"
        "  content TEXT NOT NULL,"
        "  timestamp INTEGER DEFAULT (strftime('%s', 'now') * 1000),"
        "  FOREIGN KEY (session_id) REFERENCES sessions(session_id)"
        ")";
    
    ret = sqlite3_exec(db, sql_messages, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        sqlite3_free(err_msg);
        return -1;
    }
    
    /* Create messages index */
    const char *sql_index =
        "CREATE INDEX IF NOT EXISTS idx_messages_session "
        "ON messages(session_id, timestamp)";
    
    ret = sqlite3_exec(db, sql_index, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        sqlite3_free(err_msg);
        return -1;
    }
    
    /* Create memory table */
    const char *sql_memory =
        "CREATE TABLE IF NOT EXISTS memory ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL,"
        "  created_at INTEGER DEFAULT (strftime('%s', 'now') * 1000),"
        "  updated_at INTEGER DEFAULT (strftime('%s', 'now') * 1000)"
        ")";
    
    ret = sqlite3_exec(db, sql_memory, NULL, NULL, &err_msg);
    if (ret != SQLITE_OK) {
        sqlite3_free(err_msg);
        return -1;
    }
    
    return 0;
}

static mimi_mem_conn_slot_t* get_connection(mimi_mem_pool_t *pool)
{
    if (pool == NULL) {
        return NULL;
    }
    
    int64_t now = get_timestamp_ms();
    
    /* Find an available connection */
    for (int i = 0; i < pool->pool_size; i++) {
        if (!pool->connections[i].in_use) {
            pool->connections[i].in_use = 1;
            pool->connections[i].last_used = now;
            return &pool->connections[i];
        }
    }
    
    return NULL; /* Pool exhausted */
}

static void release_connection(mimi_mem_conn_slot_t *slot)
{
    if (slot != NULL) {
        slot->in_use = 0;
        slot->last_used = get_timestamp_ms();
    }
}

static void set_error(mimi_mem_pool_t *pool, const char *msg)
{
    if (pool != NULL && msg != NULL) {
        strncpy(pool->last_error, msg, sizeof(pool->last_error) - 1);
        pool->last_error[sizeof(pool->last_error) - 1] = '\0';
    }
}

/* ============================================================================
 * Lifecycle Management
 * ============================================================================ */

mimi_mem_pool_t* mimi_mem_pool_create(const mimi_mem_config_t *config)
{
    if (config == NULL) {
        return NULL;
    }
    
    if (config->pool_size < 1 || config->pool_size > 16) {
        return NULL;
    }
    
    mimi_mem_pool_t *pool = (mimi_mem_pool_t*)calloc(1, sizeof(mimi_mem_pool_t));
    if (pool == NULL) {
        return NULL;
    }
    
    memcpy(&pool->config, config, sizeof(mimi_mem_config_t));
    pool->pool_size = config->pool_size;
    
    /* Allocate connection slots */
    pool->connections = (mimi_mem_conn_slot_t*)calloc(pool->pool_size, sizeof(mimi_mem_conn_slot_t));
    if (pool->connections == NULL) {
        free(pool);
        return NULL;
    }
    
    /* Initialize connections */
    for (int i = 0; i < pool->pool_size; i++) {
        int ret = sqlite3_open(config->db_path, &pool->connections[i].db);
        if (ret != SQLITE_OK) {
            set_error(pool, sqlite3_errmsg(pool->connections[i].db));
            sqlite3_close(pool->connections[i].db);
            /* Cleanup already opened connections */
            for (int j = 0; j < i; j++) {
                finalize_statements(&pool->connections[j].stmts);
                sqlite3_close(pool->connections[j].db);
            }
            free(pool->connections);
            free(pool);
            return NULL;
        }
        
        /* Apply SQLite optimizations to each connection */
        if (init_database(pool->connections[i].db) != 0) {
            set_error(pool, "Failed to initialize database");
            for (int j = 0; j <= i; j++) {
                finalize_statements(&pool->connections[j].stmts);
                sqlite3_close(pool->connections[j].db);
            }
            free(pool->connections);
            free(pool);
            return NULL;
        }
        
        /* Prepare statements for this connection */
        if (init_statements(pool->connections[i].db, &pool->connections[i].stmts) != 0) {
            set_error(pool, "Failed to prepare statements");
            for (int j = 0; j <= i; j++) {
                finalize_statements(&pool->connections[j].stmts);
                sqlite3_close(pool->connections[j].db);
            }
            free(pool->connections);
            free(pool);
            return NULL;
        }
        
        pool->connections[i].in_use = 0;
        pool->connections[i].last_used = get_timestamp_ms();
    }
    
    return pool;
}

void mimi_mem_pool_destroy(mimi_mem_pool_t *pool)
{
    if (pool == NULL) {
        return;
    }
    
    for (int i = 0; i < pool->pool_size; i++) {
        if (pool->connections[i].db != NULL) {
            /* Finalize prepared statements */
            finalize_statements(&pool->connections[i].stmts);
            sqlite3_close(pool->connections[i].db);
        }
    }
    
    free(pool->connections);
    free(pool);
}

/* ============================================================================
 * Session Management
 * ============================================================================ */

int mimi_mem_session_create(mimi_mem_pool_t *pool, const char *session_id)
{
    if (pool == NULL || session_id == NULL) {
        return MIMI_MEMORY_ERR_INVALID_ARG;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return MIMI_MEMORY_ERR_POOL_FULL;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = "INSERT OR IGNORE INTO sessions (session_id) VALUES (?)";
    
    int ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    release_connection(slot);
    
    if (ret == SQLITE_DONE) {
        return MIMI_MEMORY_OK;
    } else if (ret == SQLITE_CONSTRAINT) {
        return MIMI_MEMORY_ERR_EXISTS;
    } else {
        set_error(pool, sqlite3_errmsg(slot->db));
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
}

int mimi_mem_session_delete(mimi_mem_pool_t *pool, const char *session_id)
{
    if (pool == NULL || session_id == NULL) {
        return MIMI_MEMORY_ERR_INVALID_ARG;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return MIMI_MEMORY_ERR_POOL_FULL;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM sessions WHERE session_id = ?";
    
    int ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    int changes = sqlite3_changes(slot->db);
    sqlite3_finalize(stmt);
    release_connection(slot);
    
    return (changes > 0) ? MIMI_MEMORY_OK : MIMI_MEMORY_ERR_NOT_FOUND;
}

int mimi_mem_session_list(mimi_mem_pool_t *pool, char ***sessions, int *count)
{
    if (pool == NULL || sessions == NULL || count == NULL) {
        return MIMI_MEMORY_ERR_INVALID_ARG;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return MIMI_MEMORY_ERR_POOL_FULL;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = "SELECT session_id FROM sessions ORDER BY created_at DESC";
    
    int ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    /* Count results */
    int session_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        session_count++;
    }
    
    sqlite3_finalize(stmt);
    
    if (session_count == 0) {
        *sessions = NULL;
        *count = 0;
        release_connection(slot);
        return MIMI_MEMORY_OK;
    }
    
    /* Re-prepare for fetching */
    ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    /* Allocate array */
    *sessions = (char**)malloc(session_count * sizeof(char*));
    if (*sessions == NULL) {
        sqlite3_finalize(stmt);
        release_connection(slot);
        return MIMI_MEMORY_ERR_NO_MEMORY;
    }
    
    /* Fetch results */
    for (int i = 0; i < session_count; i++) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *text = (const char*)sqlite3_column_text(stmt, 0);
            (*sessions)[i] = strdup(text ? text : "");
            if ((*sessions)[i] == NULL) {
                /* Cleanup on failure */
                for (int j = 0; j < i; j++) {
                    free((*sessions)[j]);
                }
                free(*sessions);
                *sessions = NULL;
                sqlite3_finalize(stmt);
                release_connection(slot);
                return MIMI_MEMORY_ERR_NO_MEMORY;
            }
        }
    }
    
    sqlite3_finalize(stmt);
    *count = session_count;
    release_connection(slot);
    return MIMI_MEMORY_OK;
}

int mimi_mem_session_exists(mimi_mem_pool_t *pool, const char *session_id)
{
    if (pool == NULL || session_id == NULL) {
        return -1;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return -1;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = "SELECT 1 FROM sessions WHERE session_id = ? LIMIT 1";
    
    int ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        release_connection(slot);
        return -1;
    }
    
    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    release_connection(slot);
    
    return (ret == SQLITE_ROW) ? 1 : 0;
}

/* ============================================================================
 * Message Storage
 * ============================================================================ */

int mimi_mem_message_append(mimi_mem_pool_t *pool, const char *session_id,
                            const char *role, const char *content, int64_t *msg_id)
{
    if (pool == NULL || session_id == NULL || role == NULL || content == NULL) {
        return MIMI_MEMORY_ERR_INVALID_ARG;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return MIMI_MEMORY_ERR_POOL_FULL;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO messages (session_id, role, content) VALUES (?, ?, ?)";
    
    int ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, role, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, content, -1, SQLITE_STATIC);
    
    ret = sqlite3_step(stmt);
    
    if (ret == SQLITE_DONE && msg_id != NULL) {
        *msg_id = sqlite3_last_insert_rowid(slot->db);
    }
    
    sqlite3_finalize(stmt);
    release_connection(slot);
    
    return (ret == SQLITE_DONE) ? MIMI_MEMORY_OK : MIMI_MEMORY_ERR_DB_ERROR;
}

int mimi_mem_message_query(mimi_mem_pool_t *pool, const char *session_id,
                           int limit, mimi_mem_message_t ***messages, int *count)
{
    if (pool == NULL || session_id == NULL || messages == NULL || count == NULL) {
        return MIMI_MEMORY_ERR_INVALID_ARG;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return MIMI_MEMORY_ERR_POOL_FULL;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = (limit > 0) ?
        "SELECT msg_id, session_id, role, content, timestamp FROM messages "
        "WHERE session_id = ? ORDER BY timestamp ASC LIMIT ?" :
        "SELECT msg_id, session_id, role, content, timestamp FROM messages "
        "WHERE session_id = ? ORDER BY timestamp ASC";
    
    int ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    if (limit > 0) {
        sqlite3_bind_int(stmt, 2, limit);
    }
    
    /* Count results */
    int msg_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        msg_count++;
    }
    
    sqlite3_finalize(stmt);
    
    if (msg_count == 0) {
        *messages = NULL;
        *count = 0;
        release_connection(slot);
        return MIMI_MEMORY_OK;
    }
    
    /* Re-prepare for fetching */
    ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    if (limit > 0) {
        sqlite3_bind_int(stmt, 2, limit);
    }
    
    /* Allocate array */
    *messages = (mimi_mem_message_t**)malloc(msg_count * sizeof(mimi_mem_message_t*));
    if (*messages == NULL) {
        sqlite3_finalize(stmt);
        release_connection(slot);
        return MIMI_MEMORY_ERR_NO_MEMORY;
    }
    
    /* Fetch results */
    for (int i = 0; i < msg_count; i++) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            mimi_mem_message_t *msg = (mimi_mem_message_t*)calloc(1, sizeof(mimi_mem_message_t));
            if (msg == NULL) {
                /* Cleanup on failure */
                for (int j = 0; j < i; j++) {
                    mimi_mem_message_free((*messages)[j]);
                }
                free(*messages);
                sqlite3_finalize(stmt);
                release_connection(slot);
                return MIMI_MEMORY_ERR_NO_MEMORY;
            }
            
            msg->msg_id = sqlite3_column_int64(stmt, 0);
            
            const char *text;
            text = (const char*)sqlite3_column_text(stmt, 1);
            msg->session_id = strdup(text ? text : "");
            
            text = (const char*)sqlite3_column_text(stmt, 2);
            msg->role = strdup(text ? text : "");
            
            text = (const char*)sqlite3_column_text(stmt, 3);
            msg->content = strdup(text ? text : "");
            
            msg->timestamp = sqlite3_column_int64(stmt, 4);
            
            (*messages)[i] = msg;
        }
    }
    
    sqlite3_finalize(stmt);
    *count = msg_count;
    release_connection(slot);
    return MIMI_MEMORY_OK;
}

void mimi_mem_message_free(mimi_mem_message_t *msg)
{
    if (msg == NULL) {
        return;
    }
    
    free(msg->session_id);
    free(msg->role);
    free(msg->content);
    free(msg);
}

int mimi_mem_message_clear(mimi_mem_pool_t *pool, const char *session_id)
{
    if (pool == NULL || session_id == NULL) {
        return MIMI_MEMORY_ERR_INVALID_ARG;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return MIMI_MEMORY_ERR_POOL_FULL;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM messages WHERE session_id = ?";
    
    int ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    release_connection(slot);
    
    return MIMI_MEMORY_OK;
}

/**
 * @brief 批量插入消息 (事务优化)
 * 
 * 使用事务批量写入多条消息，显著提升性能
 */
int mimi_mem_message_batch_append(mimi_mem_pool_t *pool, const char *session_id,
                                  const char **roles, const char **contents, int count)
{
    if (pool == NULL || session_id == NULL || roles == NULL || contents == NULL || count <= 0) {
        return MIMI_MEMORY_ERR_INVALID_ARG;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return MIMI_MEMORY_ERR_POOL_FULL;
    }
    
    sqlite3 *db = slot->db;
    int ret;
    
    /* 开始事务 */
    ret = sqlite3_exec(db, "BEGIN TRANSACTION", NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    /* 批量插入 */
    int success_count = 0;
    for (int i = 0; i < count; i++) {
        sqlite3_stmt *stmt = slot->stmts.stmt_message_append;
        sqlite3_reset(stmt);
        
        sqlite3_bind_text(stmt, 1, session_id, -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, roles[i], -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 3, contents[i], -1, SQLITE_STATIC);
        
        ret = sqlite3_step(stmt);
        if (ret == SQLITE_DONE) {
            success_count++;
        } else {
            break;
        }
    }
    
    /* 提交或回滚 */
    if (success_count == count) {
        ret = sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
        release_connection(slot);
        return (ret == SQLITE_OK) ? MIMI_MEMORY_OK : MIMI_MEMORY_ERR_DB_ERROR;
    } else {
        sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
        set_error(pool, "Batch insert failed");
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
}

/* ============================================================================
 * Memory Operations
 * ============================================================================ */

int mimi_mem_write(mimi_mem_pool_t *pool, const char *key, const char *value)
{
    if (pool == NULL || key == NULL || value == NULL) {
        return MIMI_MEMORY_ERR_INVALID_ARG;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return MIMI_MEMORY_ERR_POOL_FULL;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = 
        "INSERT INTO memory (key, value, created_at, updated_at) "
        "VALUES (?, ?, strftime('%s', 'now') * 1000, strftime('%s', 'now') * 1000) "
        "ON CONFLICT(key) DO UPDATE SET "
        "value = excluded.value, updated_at = strftime('%s', 'now') * 1000";
    
    int ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, value, -1, SQLITE_STATIC);
    
    ret = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    release_connection(slot);
    
    return (ret == SQLITE_DONE) ? MIMI_MEMORY_OK : MIMI_MEMORY_ERR_DB_ERROR;
}

int mimi_mem_read(mimi_mem_pool_t *pool, const char *key, char **value)
{
    if (pool == NULL || key == NULL || value == NULL) {
        return MIMI_MEMORY_ERR_INVALID_ARG;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return MIMI_MEMORY_ERR_POOL_FULL;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = "SELECT value FROM memory WHERE key = ?";
    
    int ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
    ret = sqlite3_step(stmt);
    
    if (ret == SQLITE_ROW) {
        const char *text = (const char*)sqlite3_column_text(stmt, 0);
        *value = strdup(text ? text : "");
        sqlite3_finalize(stmt);
        release_connection(slot);
        return (*value != NULL) ? MIMI_MEMORY_OK : MIMI_MEMORY_ERR_NO_MEMORY;
    } else {
        sqlite3_finalize(stmt);
        release_connection(slot);
        return MIMI_MEMORY_ERR_NOT_FOUND;
    }
}

int mimi_mem_delete(mimi_mem_pool_t *pool, const char *key)
{
    if (pool == NULL || key == NULL) {
        return MIMI_MEMORY_ERR_INVALID_ARG;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return MIMI_MEMORY_ERR_POOL_FULL;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM memory WHERE key = ?";
    
    int ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, key, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    int changes = sqlite3_changes(slot->db);
    sqlite3_finalize(stmt);
    release_connection(slot);
    
    return (changes > 0) ? MIMI_MEMORY_OK : MIMI_MEMORY_ERR_NOT_FOUND;
}

int mimi_mem_search(mimi_mem_pool_t *pool, const char *query,
                    mimi_mem_entry_t ***results, int *count)
{
    if (pool == NULL || query == NULL || results == NULL || count == NULL) {
        return MIMI_MEMORY_ERR_INVALID_ARG;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return MIMI_MEMORY_ERR_POOL_FULL;
    }
    
    sqlite3_stmt *stmt;
    const char *sql = 
        "SELECT key, value, created_at, updated_at FROM memory "
        "WHERE key LIKE ? OR value LIKE ? ORDER BY updated_at DESC";
    
    int ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    char pattern[512];
    snprintf(pattern, sizeof(pattern), "%%%s%%", query);
    sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pattern, -1, SQLITE_STATIC);
    
    /* Count results */
    int entry_count = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        entry_count++;
    }
    
    sqlite3_finalize(stmt);
    
    if (entry_count == 0) {
        *results = NULL;
        *count = 0;
        release_connection(slot);
        return MIMI_MEMORY_OK;
    }
    
    /* Re-prepare for fetching */
    ret = sqlite3_prepare_v2(slot->db, sql, -1, &stmt, NULL);
    if (ret != SQLITE_OK) {
        set_error(pool, sqlite3_errmsg(slot->db));
        release_connection(slot);
        return MIMI_MEMORY_ERR_DB_ERROR;
    }
    
    sqlite3_bind_text(stmt, 1, pattern, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, pattern, -1, SQLITE_STATIC);
    
    /* Allocate array */
    *results = (mimi_mem_entry_t**)malloc(entry_count * sizeof(mimi_mem_entry_t*));
    if (*results == NULL) {
        sqlite3_finalize(stmt);
        release_connection(slot);
        return MIMI_MEMORY_ERR_NO_MEMORY;
    }
    
    /* Fetch results */
    for (int i = 0; i < entry_count; i++) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            mimi_mem_entry_t *entry = (mimi_mem_entry_t*)calloc(1, sizeof(mimi_mem_entry_t));
            if (entry == NULL) {
                /* Cleanup on failure */
                for (int j = 0; j < i; j++) {
                    mimi_mem_entry_free((*results)[j]);
                }
                free(*results);
                sqlite3_finalize(stmt);
                release_connection(slot);
                return MIMI_MEMORY_ERR_NO_MEMORY;
            }
            
            const char *text;
            text = (const char*)sqlite3_column_text(stmt, 0);
            entry->key = strdup(text ? text : "");
            
            text = (const char*)sqlite3_column_text(stmt, 1);
            entry->value = strdup(text ? text : "");
            
            entry->created_at = sqlite3_column_int64(stmt, 2);
            entry->updated_at = sqlite3_column_int64(stmt, 3);
            
            (*results)[i] = entry;
        }
    }
    
    sqlite3_finalize(stmt);
    *count = entry_count;
    release_connection(slot);
    return MIMI_MEMORY_OK;
}

void mimi_mem_entry_free(mimi_mem_entry_t *entry)
{
    if (entry == NULL) {
        return;
    }
    
    free(entry->key);
    free(entry->value);
    free(entry);
}

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

const char* mimi_mem_version(void)
{
    return MIMI_MEMORY_VERSION;
}

const char* mimi_mem_strerror(int error)
{
    switch (error) {
        case MIMI_MEMORY_OK:
            return "Success";
        case MIMI_MEMORY_ERR_INVALID_ARG:
            return "Invalid argument";
        case MIMI_MEMORY_ERR_NO_MEMORY:
            return "Memory allocation failed";
        case MIMI_MEMORY_ERR_DB_ERROR:
            return "Database error";
        case MIMI_MEMORY_ERR_NOT_FOUND:
            return "Resource not found";
        case MIMI_MEMORY_ERR_EXISTS:
            return "Resource already exists";
        case MIMI_MEMORY_ERR_POOL_FULL:
            return "Connection pool is full";
        case MIMI_MEMORY_ERR_INIT_FAILED:
            return "Initialization failed";
        default:
            return "Unknown error";
    }
}

const char* mimi_mem_last_error(mimi_mem_pool_t *pool)
{
    if (pool == NULL) {
        return "Invalid pool";
    }
    return pool->last_error;
}

int mimi_mem_vacuum(mimi_mem_pool_t *pool)
{
    if (pool == NULL) {
        return MIMI_MEMORY_ERR_INVALID_ARG;
    }
    
    mimi_mem_conn_slot_t *slot = get_connection(pool);
    if (slot == NULL) {
        return MIMI_MEMORY_ERR_POOL_FULL;
    }
    
    int ret = sqlite3_exec(slot->db, "VACUUM", NULL, NULL, NULL);
    release_connection(slot);
    
    return (ret == SQLITE_OK) ? MIMI_MEMORY_OK : MIMI_MEMORY_ERR_DB_ERROR;
}
