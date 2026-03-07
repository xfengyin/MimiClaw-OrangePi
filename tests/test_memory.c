/*
 * Memory Store Test
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include "../src/memory/memory_store.h"

void test_memory_init() {
    memory_store_t store;
    
    // Test with temporary database
    int ret = memory_store_init(&store, "/tmp/test_memory.db");
    assert(ret == 0);
    assert(store.initialized == true);
    
    memory_store_close(&store);
    unlink("/tmp/test_memory.db");
    
    printf("✓ Memory init test passed\n");
}

void test_memory_add_and_get() {
    memory_store_t store;
    memory_store_init(&store, "/tmp/test_memory2.db");
    
    // Add memory entry
    memory_entry_t entry = {
        .type = MEMORY_TYPE_LONG_TERM,
        .session_id = "test_session",
        .user_id = "test_user",
        .content = "Test memory content"
    };
    
    int ret = memory_add(&store, &entry);
    assert(ret == 0);
    assert(entry.id > 0);
    
    // Retrieve memories
    memory_entry_t *entries = NULL;
    int count = 0;
    ret = memory_get_by_type(&store, MEMORY_TYPE_LONG_TERM, &entries, &count);
    assert(ret == 0);
    assert(count == 1);
    assert(strcmp(entries[0].content, "Test memory content") == 0);
    
    memory_free_entries(entries, count);
    memory_store_close(&store);
    unlink("/tmp/test_memory2.db");
    
    printf("✓ Memory add and get test passed\n");
}

void test_session_operations() {
    memory_store_t store;
    memory_store_init(&store, "/tmp/test_memory3.db");
    
    // Create session
    int ret = session_create(&store, "session_123", "user_456");
    assert(ret == 0);
    
    // Add messages
    ret = session_add_message(&store, "session_123", "user", "Hello");
    assert(ret == 0);
    
    ret = session_add_message(&store, "session_123", "assistant", "Hi there!");
    assert(ret == 0);
    
    // Get history
    char *history = NULL;
    int msg_count = 0;
    ret = session_get_history(&store, "session_123", &history, &msg_count);
    assert(ret == 0);
    assert(msg_count == 2);
    assert(history != NULL);
    assert(strstr(history, "Hello") != NULL);
    assert(strstr(history, "Hi there!") != NULL);
    
    free(history);
    memory_store_close(&store);
    unlink("/tmp/test_memory3.db");
    
    printf("✓ Session operations test passed\n");
}

int main() {
    printf("Running memory store tests...\n");
    
    test_memory_init();
    test_memory_add_and_get();
    test_session_operations();
    
    printf("\nAll tests passed!\n");
    return 0;
}