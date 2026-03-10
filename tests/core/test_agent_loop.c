/**
 * @file test_agent_loop.c
 * @brief AI Agent 循环单元测试
 * 
 * 测试模块：libmimi-core
 * 测试框架：CMocka
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <pthread.h>
#include <unistd.h>

#include "mimi/agent.h"
#include "mimi/context.h"
#include "mimi/event.h"
#include "mocks/mock_external_api.h"

// ============================================================================
// 测试夹具 (Setup/Teardown)
// ============================================================================

static MimiAgent* agent = NULL;
static MimiContext* ctx = NULL;

static int setup_agent(void **state) {
    (void) state;
    
    // 初始化 Agent
    agent = agent_create("test-agent");
    assert_non_null(agent);
    
    // 初始化上下文
    ctx = context_create("test-session-001");
    assert_non_null(ctx);
    
    // 初始化 Mock
    mock_api_init();
    
    return 0;
}

static int teardown_agent(void **state) {
    (void) state;
    
    // 清理资源
    if (ctx) {
        context_destroy(ctx);
        ctx = NULL;
    }
    
    if (agent) {
        agent_destroy(agent);
        agent = NULL;
    }
    
    // 清理 Mock
    mock_api_cleanup();
    
    return 0;
}

// ============================================================================
// 测试用例：Agent 循环初始化
// ============================================================================

static void test_agent_loop_initialization(void **state) {
    (void) state;
    
    // 验证 Agent 初始状态
    assert_int_equal(agent_get_state(agent), AGENT_STATE_IDLE);
    assert_int_equal(agent_get_session_count(agent), 0);
    
    // 启动 Agent 循环
    int ret = agent_start(agent);
    assert_int_equal(ret, 0);
    
    // 等待短暂时间让循环启动
    usleep(100000);  // 100ms
    
    // 验证 Agent 已启动
    assert_true(agent_is_running(agent));
    
    // 停止 Agent 循环
    ret = agent_stop(agent);
    assert_int_equal(ret, 0);
    
    // 验证 Agent 已停止
    assert_false(agent_is_running(agent));
}

// ============================================================================
// 测试用例：事件处理流程
// ============================================================================

static void test_agent_loop_process_event(void **state) {
    (void) state;
    
    // 启动 Agent
    agent_start(agent);
    usleep(100000);
    
    // 创建测试事件
    MimiEvent* event = event_create(EVENT_TYPE_MESSAGE, ctx);
    assert_non_null(event);
    
    event_set_payload(event, "Hello, MimiClaw!", strlen("Hello, MimiClaw!"));
    
    // 发送事件到 Agent
    int ret = agent_send_event(agent, event);
    assert_int_equal(ret, 0);
    
    // 等待事件处理
    usleep(500000);  // 500ms
    
    // 验证事件已处理
    assert_int_equal(event_get_status(event), EVENT_STATUS_PROCESSED);
    
    // 清理
    event_destroy(event);
    agent_stop(agent);
}

// ============================================================================
// 测试用例：状态转换正确性
// ============================================================================

static void test_agent_loop_state_transition(void **state) {
    (void) state;
    
    // 初始状态：IDLE
    assert_int_equal(agent_get_state(agent), AGENT_STATE_IDLE);
    
    // 启动 Agent：IDLE -> PROCESSING
    agent_start(agent);
    usleep(100000);
    
    // 发送事件触发处理
    MimiEvent* event = event_create(EVENT_TYPE_MESSAGE, ctx);
    agent_send_event(agent, event);
    
    // 短暂等待，应该在 PROCESSING 或 WAITING 状态
    usleep(50000);
    AgentState state = agent_get_state(agent);
    assert_true(state == AGENT_STATE_PROCESSING || 
                state == AGENT_STATE_WAITING);
    
    // 等待处理完成：应该回到 IDLE
    usleep(1000000);  // 1s
    state = agent_get_state(agent);
    assert_true(state == AGENT_STATE_IDLE || 
                state == AGENT_STATE_WAITING);
    
    // 停止 Agent
    agent_stop(agent);
    event_destroy(event);
}

// ============================================================================
// 测试用例：错误恢复机制
// ============================================================================

static void test_agent_loop_error_recovery(void **state) {
    (void) state;
    
    // 配置 Mock API 返回错误
    mock_api_set_error(MOCK_API_ERROR_TIMEOUT);
    
    // 启动 Agent
    agent_start(agent);
    usleep(100000);
    
    // 发送事件 (将触发错误)
    MimiEvent* event = event_create(EVENT_TYPE_MESSAGE, ctx);
    agent_send_event(agent, event);
    
    // 等待错误处理
    usleep(1000000);  // 1s
    
    // 验证 Agent 仍在运行 (错误恢复)
    assert_true(agent_is_running(agent));
    
    // 验证错误已记录
    assert_int_not_equal(agent_get_error_count(agent), 0);
    
    // 重置 Mock
    mock_api_set_error(MOCK_API_ERROR_NONE);
    
    // 发送正常事件验证恢复
    MimiEvent* event2 = event_create(EVENT_TYPE_MESSAGE, ctx);
    agent_send_event(agent, event2);
    usleep(500000);
    
    assert_int_equal(event_get_status(event2), EVENT_STATUS_PROCESSED);
    
    // 清理
    agent_stop(agent);
    event_destroy(event);
    event_destroy(event2);
}

// ============================================================================
// 测试用例：优雅关闭
// ============================================================================

static void test_agent_loop_graceful_shutdown(void **state) {
    (void) state;
    
    // 启动 Agent
    agent_start(agent);
    usleep(100000);
    
    // 发送多个事件
    for (int i = 0; i < 5; i++) {
        MimiEvent* event = event_create(EVENT_TYPE_MESSAGE, ctx);
        char msg[64];
        snprintf(msg, sizeof(msg), "Message %d", i);
        event_set_payload(event, msg, strlen(msg));
        agent_send_event(agent, event);
        event_destroy(event);
    }
    
    // 立即停止 Agent (应等待事件处理完成)
    usleep(100000);  // 100ms
    int ret = agent_stop(agent);
    
    // 验证优雅关闭
    assert_int_equal(ret, 0);
    assert_false(agent_is_running(agent));
    
    // 验证所有事件已处理 (或正确清理)
    assert_int_equal(agent_get_pending_event_count(agent), 0);
}

// ============================================================================
// 测试用例：并发会话处理
// ============================================================================

typedef struct {
    MimiAgent* agent;
    int session_id;
    int success_count;
} ConcurrentTestArgs;

static void* concurrent_session_thread(void* arg) {
    ConcurrentTestArgs* args = (ConcurrentTestArgs*)arg;
    
    // 创建会话上下文
    char session_name[64];
    snprintf(session_name, sizeof(session_name), "session-%d", args->session_id);
    MimiContext* ctx = context_create(session_name);
    
    // 发送 10 个消息
    for (int i = 0; i < 10; i++) {
        MimiEvent* event = event_create(EVENT_TYPE_MESSAGE, ctx);
        char msg[64];
        snprintf(msg, sizeof(msg), "Session %d, Message %d", args->session_id, i);
        event_set_payload(event, msg, strlen(msg));
        
        if (agent_send_event(args->agent, event) == 0) {
            args->success_count++;
        }
        
        event_destroy(event);
        usleep(10000);  // 10ms
    }
    
    context_destroy(ctx);
    return NULL;
}

static void test_agent_loop_concurrent_sessions(void **state) {
    (void) state;
    
    const int NUM_THREADS = 10;
    pthread_t threads[NUM_THREADS];
    ConcurrentTestArgs args[NUM_THREADS];
    
    // 启动 Agent
    agent_start(agent);
    usleep(100000);
    
    // 创建并发线程
    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].agent = agent;
        args[i].session_id = i;
        args[i].success_count = 0;
        
        pthread_create(&threads[i], NULL, concurrent_session_thread, &args[i]);
    }
    
    // 等待所有线程完成
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // 验证所有事件发送成功
    int total_success = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_success += args[i].success_count;
    }
    
    assert_int_equal(total_success, NUM_THREADS * 10);  // 100 个事件
    
    // 停止 Agent
    agent_stop(agent);
}

// ============================================================================
// 测试用例：超时处理
// ============================================================================

static void test_agent_loop_timeout_handling(void **state) {
    (void) state;
    
    // 配置 Mock API 延迟 (模拟超时)
    mock_api_set_delay(5000000);  // 5s 延迟，超过默认超时
    
    // 启动 Agent
    agent_start(agent);
    usleep(100000);
    
    // 发送事件 (将超时)
    MimiEvent* event = event_create(EVENT_TYPE_MESSAGE, ctx);
    agent_send_event(agent, event);
    
    // 等待超时处理
    usleep(6000000);  // 6s
    
    // 验证事件状态为超时
    assert_int_equal(event_get_status(event), EVENT_STATUS_TIMEOUT);
    
    // 验证 Agent 仍在运行
    assert_true(agent_is_running(agent));
    
    // 清理
    mock_api_set_delay(0);
    agent_stop(agent);
    event_destroy(event);
}

// ============================================================================
// 测试用例：内存泄漏检测
// ============================================================================

static void test_agent_loop_memory_leak(void **state) {
    (void) state;
    
    // 运行 1000 次事件循环
    agent_start(agent);
    
    for (int i = 0; i < 1000; i++) {
        MimiEvent* event = event_create(EVENT_TYPE_MESSAGE, ctx);
        char msg[64];
        snprintf(msg, sizeof(msg), "Leak test message %d", i);
        event_set_payload(event, msg, strlen(msg));
        
        agent_send_event(agent, event);
        event_destroy(event);
        
        usleep(1000);  // 1ms
    }
    
    // 等待所有事件处理完成
    usleep(2000000);  // 2s
    
    // 停止 Agent
    agent_stop(agent);
    
    // 如果启用了 valgrind，这里会报告内存泄漏
    // 通过 CI/CD 中的 valgrind 检查来验证
    assert_true(agent_is_running(agent) == false);
}

// ============================================================================
// 测试主函数
// ============================================================================

int main(void) {
    // 定义测试套件
    const struct CMUnitTest core_tests[] = {
        cmocka_unit_test_setup_teardown(
            test_agent_loop_initialization,
            setup_agent, teardown_agent
        ),
        cmocka_unit_test_setup_teardown(
            test_agent_loop_process_event,
            setup_agent, teardown_agent
        ),
        cmocka_unit_test_setup_teardown(
            test_agent_loop_state_transition,
            setup_agent, teardown_agent
        ),
        cmocka_unit_test_setup_teardown(
            test_agent_loop_error_recovery,
            setup_agent, teardown_agent
        ),
        cmocka_unit_test_setup_teardown(
            test_agent_loop_graceful_shutdown,
            setup_agent, teardown_agent
        ),
        cmocka_unit_test_setup_teardown(
            test_agent_loop_concurrent_sessions,
            setup_agent, teardown_agent
        ),
        cmocka_unit_test_setup_teardown(
            test_agent_loop_timeout_handling,
            setup_agent, teardown_agent
        ),
        cmocka_unit_test_setup_teardown(
            test_agent_loop_memory_leak,
            setup_agent, teardown_agent
        ),
    };
    
    // 运行测试
    return cmocka_run_group_tests(core_tests, NULL, NULL);
}
