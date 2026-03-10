# Web Search Plugin

## 功能
使用 Brave Search API 进行网络搜索。

## 输入格式
```
搜索关键词
```

## 输出格式
```json
{
  "query": "搜索词",
  "results": [
    {
      "title": "结果标题",
      "url": "https://...",
      "snippet": "摘要..."
    }
  ],
  "count": 5
}
```

## 配置

### API Key
需要配置 Brave Search API Key。可以通过以下方式：

1. **环境变量:**
   ```bash
   export BRAVE_SEARCH_API_KEY=your_api_key_here
   ```

2. **MimiClaw 配置文件:**
   在 `~/.mimiclaw/config.json` 中添加:
   ```json
   {
     "plugins": {
       "web-search": {
         "api_key": "your_api_key_here",
         "endpoint": "https://api.search.brave.com/res/v1/web/search"
       }
     }
   }
   ```

## 获取 API Key
1. 访问 https://brave.com/search/api/
2. 注册账号
3. 创建 API Key

## 依赖
- libcurl

## 示例

```bash
# 使用插件
./test_search "OpenClaw project"
```

输出:
```json
{
  "query": "OpenClaw project",
  "results": [
    {
      "title": "OpenClaw - AI Agent Framework",
      "url": "https://github.com/openclaw/...",
      "snippet": "OpenClaw is a powerful AI agent framework..."
    }
  ],
  "count": 1
}
```
