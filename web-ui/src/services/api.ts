/**
 * API Service - REST API 客户端
 *
 * 与 CVerifier 后端的 REST API 通信
 */

import axios, { AxiosInstance, AxiosResponse } from 'axios'

// API 基础 URL
const BASE_URL = import.meta.env.VITE_API_URL || '/api'

// 创建 axios 实例
const apiClient: AxiosInstance = axios.create({
  baseURL: BASE_URL,
  timeout: 30000,
  headers: {
    'Content-Type': 'application/json',
  },
})

// 请求拦截器
apiClient.interceptors.request.use(
  (config) => {
    // 可以在这里添加认证 token
    const token = localStorage.getItem('token')
    if (token) {
      config.headers.Authorization = `Bearer ${token}`
    }
    return config
  },
  (error) => {
    return Promise.reject(error)
  }
)

// 响应拦截器
apiClient.interceptors.response.use(
  (response: AxiosResponse) => {
    return response.data
  },
  (error) => {
    // 统一错误处理
    if (error.response) {
      const { status, data } = error.response
      console.error(`API Error [${status}]:`, data.message || data.error)
    } else if (error.request) {
      console.error('Network Error:', error.request)
    } else {
      console.error('Error:', error.message)
    }
    return Promise.reject(error)
  }
)

// ============================================================================
// 类型定义
// ============================================================================

export interface Task {
  taskId: string
  status: 'pending' | 'running' | 'completed' | 'failed' | 'cancelled'
  progress: number
  filesAnalyzed: number
  totalFiles: number
  vulnerabilitiesFound: number
  createdAt: string
  startedAt?: string
  completedAt?: string
  estimatedCompletion?: string
}

export interface CreateTaskRequest {
  files: string[]
  checkers: string[]
  config?: {
    timeout?: number
    maxDepth?: number
    parallelThreads?: number
  }
  entryFunction?: string
}

export interface Vulnerability {
  id: string
  type: string
  severity: 'error' | 'warning' | 'info'
  file: string
  line: number
  column: number
  message: string
  description: string
  trace?: Array<{
    file: string
    line: number
    function: string
  }>
  counterExample?: Record<string, any>
}

// ============================================================================
// API 方法
// ============================================================================

/**
 * 任务管理 API
 */
export const taskApi = {
  /**
   * 获取所有任务
   */
  getAllTasks: (): Promise<Task[]> => {
    return apiClient.get('/tasks')
  },

  /**
   * 创建新任务
   */
  createTask: (request: CreateTaskRequest): Promise<{ taskId: string }> => {
    return apiClient.post('/tasks', request)
  },

  /**
   * 获取任务详情
   */
  getTask: (taskId: string): Promise<Task> => {
    return apiClient.get(`/tasks/${taskId}`)
  },

  /**
   * 删除任务
   */
  deleteTask: (taskId: string): Promise<void> => {
    return apiClient.delete(`/tasks/${taskId}`)
  },

  /**
   * 暂停任务
   */
  pauseTask: (taskId: string): Promise<void> => {
    return apiClient.put(`/tasks/${taskId}/pause`)
  },

  /**
   * 恢复任务
   */
  resumeTask: (taskId: string): Promise<void> => {
    return apiClient.put(`/tasks/${taskId}/resume`)
  },

  /**
   * 取消任务
   */
  cancelTask: (taskId: string): Promise<void> => {
    return apiClient.delete(`/tasks/${taskId}`)
  },
}

/**
 * 结果查询 API
 */
export const resultApi = {
  /**
   * 获取任务结果
   */
  getResults: (taskId: string): Promise<any> => {
    return apiClient.get(`/tasks/${taskId}/results`)
  },

  /**
   * 获取漏洞列表
   */
  getVulnerabilities: (
    taskId: string,
    filters?: {
      type?: string
      severity?: string
      file?: string
    }
  ): Promise<{ total: number; vulnerabilities: Vulnerability[] }> => {
    return apiClient.get(`/tasks/${taskId}/vulnerabilities`, {
      params: filters,
    })
  },

  /**
   * 获取任务日志
   */
  getLogs: (
    taskId: string,
    filters?: {
      level?: string
      limit?: number
    }
  ): Promise<any[]> => {
    return apiClient.get(`/tasks/${taskId}/logs`, {
      params: filters,
    })
  },

  /**
   * 下载报告
   */
  downloadReport: (
    taskId: string,
    format: 'sarif' | 'json' | 'pdf'
  ): Promise<Blob> => {
    return apiClient.get(`/reports/${taskId}/download`, {
      params: { format },
      responseType: 'blob',
    })
  },
}

/**
 * 代码分析 API
 */
export const analysisApi = {
  /**
   * 同步分析（小代码片段）
   */
  analyze: (code: string, options?: any): Promise<any> => {
    return apiClient.post('/analyze', { code, ...options })
  },

  /**
   * 解析代码，返回 AST
   */
  parse: (code: string): Promise<any> => {
    return apiClient.post('/parse', { code })
  },
}

export default apiClient
