# CVerifier Web 架构文档

| 文档版本 | 日期 | 作者 | 变更说明 |
|---------|------|------|---------|
| 0.1.0 | 2025-01-28 | CVerifier Team | 初始版本 |

---

## 目录

- [1. 系统架构](#1-系统架构)
- [2. 前端架构](#2-前端架构)
- [3. 后端架构](#3-后端架构)
- [4. API 设计](#4-api-设计)
- [5. 部署](#5-部署)

---

## 1. 系统架构

### 1.1 架构图

```
┌─────────────────────────────────────┐
│         用户浏览器                    │
│  (Chrome, Firefox, Edge, Safari)    │
└─────────────────────────────────────┘
              ↓ HTTP/WebSocket
┌─────────────────────────────────────┐
│        React 前端 (Vite)             │
│  - 任务管理界面                      │
│  - Monaco Editor                     │
│  - 结果可视化                        │
└─────────────────────────────────────┘
              ↓ REST/WebSocket
┌─────────────────────────────────────┐
│        Crow 后端 (C++)               │
│  - REST API                          │
│  - WebSocket 服务                    │
│  - 任务调度器                        │
└─────────────────────────────────────┘
              ↓
┌─────────────────────────────────────┐
│       CVerifier 核心引擎             │
│  - 符号执行引擎                      │
│  - 抽象解释器                        │
│  - 漏洞检测器                        │
└─────────────────────────────────────┘
```

### 1.2 技术栈

**前端**：
- React 18 + TypeScript
- Vite 5（构建工具）
- Ant Design 5（UI 库）
- Monaco Editor（代码编辑器）
- React Router 6（路由）
- Zustand（状态管理）
- Axios（HTTP）
- WebSocket（实时通信）

**后端**：
- Crow C++ Framework
- WebSocket++
- nlohmann/json（JSON 处理）

---

## 2. 前端架构

### 2.1 目录结构

```
web-ui/
├── public/
│   └── index.html
├── src/
│   ├── components/          # React 组件
│   │   ├── Layout/          # 布局组件
│   │   ├── TaskManager/     # 任务管理
│   │   ├── CodeEditor/      # 代码编辑器
│   │   ├── ProgressMonitor/ # 进度监控
│   │   └── ResultsView/     # 结果查看
│   ├── pages/               # 页面
│   │   ├── Dashboard.tsx
│   │   ├── Analysis.tsx
│   │   └── Results.tsx
│   ├── services/            # API 服务
│   │   ├── api.ts           # REST API
│   │   └── websocket.ts     # WebSocket
│   ├── stores/              # 状态管理
│   ├── hooks/               # 自定义 Hooks
│   ├── utils/               # 工具函数
│   ├── types/               # TypeScript 类型
│   ├── App.tsx
│   └── main.tsx
├── package.json
├── tsconfig.json
└── vite.config.ts
```

### 2.2 核心组件

#### TaskManager

任务管理组件，支持：
- 创建任务
- 删除任务
- 暂停/恢复任务
- 批量操作

#### CodeEditor

基于 Monaco Editor 的代码编辑器：
- 语法高亮（C 语言）
- 错误标注
- 代码导航
- 实时标注

#### ProgressMonitor

实时进度监控：
- 进度条
- 日志流
- 资源监控

#### ResultsView

结果查看器：
- 漏洞列表
- 错误详情
- 反例展示
- 修复建议

### 2.3 状态管理

使用 Zustand 管理全局状态：

```typescript
interface TaskStore {
  tasks: Task[]
  selectedTask: Task | null

  // Actions
  fetchTasks: () => Promise<void>
  createTask: (request: CreateTaskRequest) => Promise<void>
  deleteTask: (taskId: string) => Promise<void>
  selectTask: (taskId: string) => void
}

const useTaskStore = create<TaskStore>((set, get) => ({
  tasks: [],
  selectedTask: null,

  fetchTasks: async () => {
    const tasks = await taskApi.getAllTasks()
    set({ tasks })
  },

  // ...
}))
```

---

## 3. 后端架构

### 3.1 目录结构

```
src/web/
├── api/                  # REST API 处理
│   ├── task_api.cpp
│   ├── result_api.cpp
│   └── analysis_api.cpp
├── ws/                   # WebSocket 服务
│   └── websocket_server.cpp
├── scheduler/            # 任务调度器
│   └── task_scheduler.cpp
├── WebServer.h/cpp       # 主服务器
└── routes.cpp            # 路由配置
```

### 3.2 REST API 端点

#### 任务管理

```
GET    /api/tasks              # 获取所有任务
POST   /api/tasks              # 创建任务
GET    /api/tasks/{id}         # 获取任务详情
DELETE /api/tasks/{id}         # 删除任务
PUT    /api/tasks/{id}/pause   # 暂停任务
PUT    /api/tasks/{id}/resume  # 恢复任务
```

#### 结果查询

```
GET /api/tasks/{id}/results           # 获取结果
GET /api/tasks/{id}/vulnerabilities   # 获取漏洞列表
GET /api/tasks/{id}/logs              # 获取日志
GET /api/reports/{id}/download        # 下载报告
```

#### 代码分析

```
POST /api/analyze         # 同步分析
POST /api/parse           # 解析代码
```

### 3.3 WebSocket 端点

```
/ws/tasks/{id}/progress   # 任务进度推送
/ws/tasks/{id}/logs       # 日志流推送
```

### 3.4 消息格式

#### 进度更新

```json
{
  "type": "progress",
  "taskId": "uuid",
  "timestamp": "2025-01-28T10:00:00Z",
  "data": {
    "progress": 45,
    "message": "Analyzing function foo...",
    "filesAnalyzed": 10,
    "totalFiles": 22
  }
}
```

#### 漏洞发现

```json
{
  "type": "vulnerability",
  "taskId": "uuid",
  "timestamp": "2025-01-28T10:00:00Z",
  "data": {
    "vulnerability": {
      "type": "buffer-overflow",
      "severity": "error",
      "file": "file.c",
      "line": 42,
      "message": "Buffer overflow detected"
    }
  }
}
```

---

## 4. API 设计

### 4.1 REST API

详见 `design.md` 第 9 节

### 4.2 WebSocket API

详见 `web-ui/src/services/websocket.ts`

---

## 5. 部署

### 5.1 前端部署

```bash
cd web-ui
npm run build
# 生成 dist/ 目录
```

### 5.2 后端部署

```bash
mkdir build && cd build
cmake ..
make
# 生成 cverifier-web 可执行文件
```

### 5.3 Docker 部署

```dockerfile
# Dockerfile
FROM node:18 AS frontend
WORKDIR /app
COPY web-ui/package.json web-ui/
RUN npm install
COPY web-ui/ .
RUN npm run build

FROM ubuntu:22.04 AS backend
RUN apt-get update && apt-get install -y \
    clang-15 llvm-15-dev libz3-dev cmake
COPY . /app
WORKDIR /app/build
RUN cmake .. && make
COPY --from=frontend /app/dist /var/www/html

CMD ["./cverifier-web", "--port", "8080"]
```

---

## 附录

- [前端 README](../web-ui/README.md)
- [API 设计文档](design.md#9-web-api-设计)

---

**文档结束**
