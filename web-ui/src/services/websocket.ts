/**
 * WebSocket Service - 实时通信
 *
 * 与 CVerifier 后端的 WebSocket 服务通信
 * 用于实时接收任务进度、日志和漏洞发现
 */

// WebSocket 消息类型
export type WebSocketMessage =
  | ProgressMessage
  | LogMessage
  | VulnerabilityMessage
  | CompleteMessage
  | ErrorMessage

export interface ProgressMessage {
  type: 'progress'
  taskId: string
  timestamp: string
  data: {
    progress: number
    message: string
    filesAnalyzed: number
    totalFiles: number
  }
}

export interface LogMessage {
  type: 'log'
  taskId: string
  timestamp: string
  data: {
    level: 'DEBUG' | 'INFO' | 'WARN' | 'ERROR'
    message: string
    source: string
  }
}

export interface VulnerabilityMessage {
  type: 'vulnerability'
  taskId: string
  timestamp: string
  data: {
    vulnerability: any
  }
}

export interface CompleteMessage {
  type: 'complete'
  taskId: string
  timestamp: string
  data: {
    status: 'completed' | 'failed'
    vulnerabilitiesFound: number
    durationSeconds: number
  }
}

export interface ErrorMessage {
  type: 'error'
  taskId: string
  timestamp: string
  data: {
    error: string
    message: string
  }
}

// WebSocket 事件处理器类型
type MessageHandler = (message: WebSocketMessage) => void
type ConnectionHandler = () => void
type ErrorHandler = (error: Event) => void

/**
 * WebSocket 客户端类
 */
class WebSocketClient {
  private ws: WebSocket | null = null
  private url: string
  private reconnectAttempts = 0
  private maxReconnectAttempts = 5
  private reconnectDelay = 3000
  private reconnectTimer: NodeJS.Timeout | null = null

  // 事件处理器
  private onMessageHandlers: Map<string, Set<MessageHandler>> = new Map()
  private onOpenHandlers: Set<ConnectionHandler> = new Set()
  private onCloseHandlers: Set<ConnectionHandler> = new Set()
  private onErrorHandlers: Set<ErrorHandler> = new Set()

  constructor(taskId: string) {
    const wsProtocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:'
    const wsHost = import.meta.env.VITE_WS_HOST || window.location.host
    this.url = `${wsProtocol}//${wsHost}/ws/tasks/${taskId}/progress`
  }

  /**
   * 连接 WebSocket
   */
  connect(): void {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      console.warn('WebSocket is already connected')
      return
    }

    console.log(`Connecting to WebSocket: ${this.url}`)

    try {
      this.ws = new WebSocket(this.url)

      this.ws.onopen = this.handleOpen.bind(this)
      this.ws.onmessage = this.handleMessage.bind(this)
      this.ws.onclose = this.handleClose.bind(this)
      this.ws.onerror = this.handleError.bind(this)
    } catch (error) {
      console.error('Failed to create WebSocket:', error)
      this.scheduleReconnect()
    }
  }

  /**
   * 断开连接
   */
  disconnect(): void {
    if (this.reconnectTimer) {
      clearTimeout(this.reconnectTimer)
      this.reconnectTimer = null
    }

    if (this.ws) {
      this.ws.close()
      this.ws = null
    }
  }

  /**
   * 发送消息
   */
  send(data: any): void {
    if (this.ws && this.ws.readyState === WebSocket.OPEN) {
      this.ws.send(JSON.stringify(data))
    } else {
      console.warn('WebSocket is not connected. Message not sent:', data)
    }
  }

  /**
   * 订阅消息
   */
  on(messageType: string, handler: MessageHandler): () => void {
    if (!this.onMessageHandlers.has(messageType)) {
      this.onMessageHandlers.set(messageType, new Set())
    }
    this.onMessageHandlers.get(messageType)!.add(handler)

    // 返回取消订阅函数
    return () => {
      this.off(messageType, handler)
    }
  }

  /**
   * 取消订阅
   */
  off(messageType: string, handler: MessageHandler): void {
    const handlers = this.onMessageHandlers.get(messageType)
    if (handlers) {
      handlers.delete(handler)
    }
  }

  /**
   * 订阅连接打开事件
   */
  onOpen(handler: ConnectionHandler): () => void {
    this.onOpenHandlers.add(handler)
    return () => {
      this.onOpenHandlers.delete(handler)
    }
  }

  /**
   * 订阅连接关闭事件
   */
  onClose(handler: ConnectionHandler): () => void {
    this.onCloseHandlers.add(handler)
    return () => {
      this.onCloseHandlers.delete(handler)
    }
  }

  /**
   * 订阅错误事件
   */
  onError(handler: ErrorHandler): () => void {
    this.onErrorHandlers.add(handler)
    return () => {
      this.onErrorHandlers.delete(handler)
    }
  }

  /**
   * 处理连接打开
   */
  private handleOpen(): void {
    console.log('WebSocket connected')
    this.reconnectAttempts = 0

    this.onOpenHandlers.forEach((handler) => {
      handler()
    })
  }

  /**
   * 处理消息接收
   */
  private handleMessage(event: MessageEvent): void {
    try {
      const message: WebSocketMessage = JSON.parse(event.data)
      const { type } = message

      // 调用特定类型的处理器
      const handlers = this.onMessageHandlers.get(type)
      if (handlers) {
        handlers.forEach((handler) => {
          handler(message)
        })
      }

      // 调用通用处理器（使用 '*' 通配符）
      const allHandlers = this.onMessageHandlers.get('*')
      if (allHandlers) {
        allHandlers.forEach((handler) => {
          handler(message)
        })
      }
    } catch (error) {
      console.error('Failed to parse WebSocket message:', error)
    }
  }

  /**
   * 处理连接关闭
   */
  private handleClose(event: CloseEvent): void {
    console.log('WebSocket closed:', event.code, event.reason)

    this.onCloseHandlers.forEach((handler) => {
      handler()
    })

    // 如果不是正常关闭，尝试重连
    if (event.code !== 1000) {
      this.scheduleReconnect()
    }
  }

  /**
   * 处理错误
   */
  private handleError(event: Event): void {
    console.error('WebSocket error:', event)

    this.onErrorHandlers.forEach((handler) => {
      handler(event)
    })
  }

  /**
   * 安排重连
   */
  private scheduleReconnect(): void {
    if (this.reconnectAttempts >= this.maxReconnectAttempts) {
      console.error('Max reconnect attempts reached')
      return
    }

    this.reconnectAttempts++
    const delay = this.reconnectDelay * this.reconnectAttempts

    console.log(`Scheduling reconnect in ${delay}ms (attempt ${this.reconnectAttempts})`)

    this.reconnectTimer = setTimeout(() => {
      this.connect()
    }, delay)
  }

  /**
   * 获取连接状态
   */
  get readyState(): number {
    return this.ws?.readyState ?? WebSocket.CLOSED
  }

  /**
   * 是否已连接
   */
  get isConnected(): boolean {
    return this.ws?.readyState === WebSocket.OPEN
  }
}

export default WebSocketClient
