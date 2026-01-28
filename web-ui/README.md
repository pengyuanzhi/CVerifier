# CVerifier Web UI

CVerifier 形式化验证工具的 Web 前端界面。

## 技术栈

- **框架**: React 18 + TypeScript
- **构建工具**: Vite 5
- **UI 库**: Ant Design 5
- **代码编辑器**: Monaco Editor
- **路由**: React Router 6
- **状态管理**: Zustand
- **HTTP 客户端**: Axios
- **实时通信**: WebSocket

## 开发指南

### 安装依赖

```bash
npm install
# 或
pnpm install
```

### 启动开发服务器

```bash
npm run dev
```

访问 http://localhost:3000

### 构建生产版本

```bash
npm run build
```

### 类型检查

```bash
npm run type-check
```

### 代码检查

```bash
npm run lint
```

## 项目结构

```
web-ui/
├── public/              # 静态资源
│   └── index.html
├── src/
│   ├── components/      # React 组件
│   │   ├── Layout/      # 布局组件
│   │   ├── TaskManager/ # 任务管理
│   │   ├── CodeEditor/  # 代码编辑器（Monaco）
│   │   ├── ProgressMonitor/ # 进度监控
│   │   └── ResultsView/ # 结果查看
│   ├── pages/           # 页面组件
│   │   ├── Dashboard.tsx
│   │   ├── Analysis.tsx
│   │   └── Results.tsx
│   ├── services/        # API 服务
│   │   ├── api.ts       # REST API
│   │   └── websocket.ts # WebSocket
│   ├── stores/          # 状态管理
│   ├── hooks/           # 自定义 Hooks
│   ├── utils/           # 工具函数
│   ├── types/           # TypeScript 类型
│   ├── App.tsx          # 主应用
│   └── main.tsx         # 入口
├── package.json
├── tsconfig.json
├── vite.config.ts
└── README.md
```

## 环境变量

创建 `.env.local` 文件：

```bash
# API 地址
VITE_API_URL=http://localhost:8080/api

# WebSocket 地址
VITE_WS_HOST=localhost:8080
```

## 功能特性

- ✅ 任务管理（创建、删除、暂停、恢复）
- ✅ 实时进度监控
- ✅ 代码编辑器（Monaco Editor）
- ✅ 漏洞结果可视化
- ✅ 日志流式查看
- ✅ 报告导出（SARIF/JSON/PDF）

## 开发规范

### 命名规范

- 组件文件：PascalCase（如 `TaskManager.tsx`）
- 工具文件：camelCase（如 `api.ts`）
- 常量：UPPER_SNAKE_CASE（如 `API_BASE_URL`）

### 组件规范

- 使用函数组件 + Hooks
- Props 接口使用 `interface` 定义
- 必须定义 TypeScript 类型
- 导出使用 `export default`

### 样式规范

- 优先使用 Ant Design 组件
- 自定义样式使用 CSS Modules 或 styled-components
- 遵循响应式设计原则

## 与后端通信

### REST API

```typescript
import { taskApi } from '@services/api'

// 获取所有任务
const tasks = await taskApi.getAllTasks()

// 创建新任务
const result = await taskApi.createTask({
  files: ['file1.c', 'file2.c'],
  checkers: ['buffer-overflow', 'null-pointer'],
})
```

### WebSocket

```typescript
import WebSocketClient from '@services/websocket'

const ws = new WebSocketClient(taskId)

// 订阅进度更新
ws.on('progress', (message) => {
  console.log('Progress:', message.data.progress)
})

// 订阅所有消息
ws.on('*', (message) => {
  console.log('Message:', message)
})

ws.connect()
```

## 性能优化

- 使用 React.memo 避免不必要的重渲染
- 使用 useMemo 缓存计算结果
- 使用 useCallback 缓存回调函数
- 代码分割（React.lazy + Suspense）
- Monaco Editor 按需加载

## 浏览器支持

- Chrome >= 90
- Firefox >= 88
- Edge >= 90
- Safari >= 14

## 许可证

MIT
