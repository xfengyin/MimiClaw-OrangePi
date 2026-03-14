/**
 * @file test_gateway_perf.c
 * @brief Gateway 性能测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

/* 获取微秒级时间戳 */
static long get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000L + tv.tv_usec;
}

/* 测试结果结构 */
typedef struct {
    const char *name;
    long min_us;
    long max_us;
    long avg_us;
    int iterations;
} perf_result_t;

/* 性能测试宏 */
#define PERF_TEST(name) void perf_##name(perf_result_t *result)
#define RUN_PERF(name, iter) do { \
    result->name = #name; \
    result->iterations = iter; \
    long start, end; \
    long total = 0; \
    result->min_us = __LONG_MAX__; \
    result->max_us = 0; \
    for (int i = 0; i < iter; i++) { \
        start = get_timestamp_us(); \
        perf_##name(result); \
        end = get_timestamp_us(); \
        long elapsed = end - start; \
        total += elapsed; \
        if (elapsed < result->min_us) result->min_us = elapsed; \
        if (elapsed > result->max_us) result->max_us = elapsed; \
    } \
    result->avg_us = total / iter; \
} while(0)

/* ============================================================================
 * 会话 ID 生成性能测试
 * ============================================================================

void perf_session_generation(perf_result_t *result) {
    char session_id[64];
    for (int i = 0; i < 1000; i++) {
        snprintf(session_id, sizeof(session_id), "session_%ld_%d", 
                 (long)time(NULL), i);
    }
}

/* ============================================================================
 * 命令解析性能测试
 * ============================================================================

void perf_command_parse(perf_result_t *result) {
    const char *input = "/chat Hello world this is a test message";
    
    for (int i = 0; i < 1000; i++) {
        /* 模拟命令解析 */
        if (strncmp(input, "/chat ", 6) == 0) {
            const char *msg = input + 6;
            /* 简单处理 */
            int len = strlen(msg);
        }
    }
}

/* ============================================================================
 * 响应格式化性能测试
 * ============================================================================

void perf_response_format(perf_result_t *result) {
    char response[4096];
    const char *templates[] = {
        "Hello, %s!",
        "Response %d: %s",
        "Result:\n- Item 1\n- Item 2\n- Item 3",
        "Status: %s\nTime: %ldms\nData: %s"
    };
    
    for (int i = 0; i < 1000; i++) {
        int idx = i % 4;
        snprintf(response, sizeof(response), templates[idx], "User", i, "data");
    }
}

/* ============================================================================
 * 字符串处理性能测试
 * ============================================================================

void perf_string_trim(perf_result_t *result) {
    const char *input = "   Hello World   ";
    char output[64];
    
    for (int i = 0; i < 1000; i++) {
        /* 模拟去空格 */
        int j = 0;
        for (const char *p = input; *p; p++) {
            if (*p != ' ') {
                output[j++] = *p;
            }
        }
        output[j] = '\0';
    }
}

/* ============================================================================
 * JSON 构建性能测试
 * ============================================================================

void perf_json_build(perf_result_t *result) {
    char json[1024];
    
    for (int i = 0; i < 1000; i++) {
        snprintf(json, sizeof(json), 
            "{\"session\":\"%s\",\"message\":\"%s\",\"timestamp\":%ld,\"code\":%d}",
            "sess_123", "hello world", (long)time(NULL), 200);
    }
}

/* ============================================================================
 * 主函数
 * ============================================================================

int main(int argc, char *argv[]) {
    printf("\n=== Gateway Performance Tests ===\n\n");
    
    int iterations = 10000;
    if (argc > 1) {
        iterations = atoi(argv[1]);
    }
    
    printf("Iterations per test: %d\n\n", iterations);
    
    /* 测试结果数组 */
    perf_result_t results[] = {
        {"Session Generation", 0, 0, 0, iterations},
        {"Command Parse", 0, 0, 0, iterations},
        {"Response Format", 0, 0, 0, iterations},
        {"String Trim", 0, 0, 0, iterations},
        {"JSON Build", 0, 0, 0, iterations},
    };
    
    int test_count = sizeof(results) / sizeof(results[0]);
    
    /* 运行测试 */
    printf("Running tests...\n\n");
    
    long start = get_timestamp_us();
    
    perf_session_generation(&results[0]);
    perf_command_parse(&results[1]);
    perf_response_format(&results[2]);
    perf_string_trim(&results[3]);
    perf_json_build(&results[4]);
    
    long end = get_timestamp_us();
    
    /* 输出结果 */
    printf("%-20s %12s %12s %12s\n", "Test", "Min (μs)", "Max (μs)", "Avg (μs)");
    printf("%-20s %12s %12s %12s\n", "----", "--------", "--------", "--------");
    
    for (int i = 0; i < test_count; i++) {
        printf("%-20s %12ld %12ld %12ld\n",
               results[i].name,
               results[i].min_us,
               results[i].max_us,
               results[i].avg_us);
    }
    
    printf("\nTotal time: %ld ms\n", (end - start) / 1000);
    
    /* 性能评估 */
    printf("\n=== Performance Assessment ===\n");
    for (int i = 0; i < test_count; i++) {
        if (results[i].avg_us < 100) {
            printf("✅ %s: Excellent (< 100μs)\n", results[i].name);
        } else if (results[i].avg_us < 1000) {
            printf("✅ %s: Good (< 1ms)\n", results[i].name);
        } else if (results[i].avg_us < 10000) {
            printf("⚠️  %s: Fair (< 10ms)\n", results[i].name);
        } else {
            printf("❌ %s: Needs optimization (> 10ms)\n", results[i].name);
        }
    }
    
    return 0;
}