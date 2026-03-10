-- wrk_chat.lua
-- MimiClaw v2.0 wrk 压测脚本

-- 请求设置
wrk.method = "POST"
wrk.scheme = "http"
wrk.host = "localhost"
wrk.port = 8080
wrk.path = "/api/v1/chat"

-- 请求头
wrk.headers["Content-Type"] = "application/json"
wrk.headers["Accept"] = "application/json"
wrk.headers["User-Agent"] = "wrk-mimiclaw-perf-test"

-- 请求体模板
local messages = {
    "Hello",
    "What is the weather today?",
    "Tell me a joke",
    "Calculate 123 * 456",
    "Search for OpenClaw",
    "Help me write code",
    "What is machine learning?",
    "Explain quantum computing",
    "Write a poem about AI",
    "What's the capital of France?"
}

local counter = 0

-- 初始化
function init(args)
    -- 可以从命令行参数获取配置
    session_id = args.session_id or "perf_test_session"
end

-- 每个线程启动时调用
function thread_setup(thread)
    thread.session_id = session_id .. "_" .. thread.id
end

-- 生成请求
function request()
    counter = counter + 1
    local msg_index = (counter % #messages) + 1
    local message = messages[msg_index]
    
    local body = string.format(
        '{"message": "%s", "session_id": "%s"}',
        message,
        wrk.thread.session_id
    )
    
    return wrk.format(nil, wrk.path, wrk.headers, body)
end

-- 响应处理
function response(status, headers, body)
    -- 可以在此添加响应验证逻辑
    if status ~= 200 then
        print("Error: Status " .. status)
    end
end

-- 所有线程完成时调用
function done(summary)
    print("==========================================")
    print("压测完成")
    print("==========================================")
    print("总请求数：" .. summary.requests)
    print("总错误数：" .. summary.errors)
    print("持续时间：" .. summary.duration .. "s")
    print("==========================================")
end
