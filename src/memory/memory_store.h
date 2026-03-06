/*
 * Memory Store Header - Long-term memory management
 */

#ifndef MEMORY_STORE_H
#define MEMORY_STORE_H

#include <sqlite3.h>
#include <stdbool.h>
#include <time.h>

#define MAX_MEMORY_CONTENT 4096
#define MAX_SESSION_ID 64
#define MAX_USER_ID 64

// Memory types
typedef enum {
    MEMORY_TYPE_SOUL = 0,      // Bot personality
    MEMORY_TYPE_USER,          // User profile
    MEMORY_TYPE_LONG_TERM,     // Long-term memories
    MEMORY_TYPE_DAILY,         // Daily notes
    MEMORY_TYPE_SESSION        // Session history
} memory_type_t;

// Memory entry
typedef struct {
    int id;
    memory_type_t type;
    char session_id[MAX_SESSION_ID];
    char user_id[MAX_USER_ID];
    char content[MAX_MEMORY_CONTENT];
    time_t created_at;
    time_t updated_at;
} memory_entry_t;

// Session info
typedef struct {
    char session_id[MAX_SESSION_ID];
    char user_id[MAX_USER_ID];
    time_t created_at;
    time_t last_activity;
    int message_count;
} session_info_t;

// Database handle
typedef struct {
    sqlite3 *db;
    char db_path[256];
    bool initialized;
} memory_store_t;

// Function prototypes
int memory_store_init(memory_store_t *store, const char *db_path);
void memory_store_close(memory_store_t *store);

// Memory operations
int memory_add(memory_store_t *store, memory_entry_t *entry);
int memory_update(memory_store_t *store, int id, const char *content);
int memory_delete(memory_store_t *store, int id);
int memory_get_by_type(memory_store_t *store, memory_type_t type, 
                       memory_entry_t **entries, int *count);
int memory_search(memory_store_t *store, const char *keyword, 
                  memory_entry_t **entries, int *count);

// Session operations
int session_create(memory_store_t *store, const char *session_id, const char *user_id);
int session_update_activity(memory_store_t *store, const char *session_id);
int session_get(memory_store_t *store, const char *session_id, session_info_t *info);
int session_list(memory_store_t *store, session_info_t **sessions, int *count);
int session_delete(memory_store_t *store, const char *session_id);
int session_add_message(memory_store_t *store, const char *session_id, 
                        const char *role, const char *content);
int session_get_history(memory_store_t *store, const char *session_id,
                        char **history, int *message_count);

// Utility
void memory_free_entries(memory_entry_t *entries, int count);
void memory_free_sessions(session_info_t *sessions, int count);
const char* memory_type_to_string(memory_type_t type);

#endif // MEMORY_STORE_H