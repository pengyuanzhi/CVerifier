import { Routes, Route, Navigate } from 'react-router-dom'
import { Layout } from 'antd'
import MainLayout from './components/Layout/MainLayout'
import Dashboard from './pages/Dashboard'
import Analysis from './pages/Analysis'
import Results from './pages/Results'

const App: React.FC = () => {
  return (
    <Layout style={{ minHeight: '100vh' }}>
      <MainLayout>
        <Routes>
          <Route path="/" element={<Navigate to="/dashboard" replace />} />
          <Route path="/dashboard" element={<Dashboard />} />
          <Route path="/analysis" element={<Analysis />} />
          <Route path="/analysis/:taskId" element={<Analysis />} />
          <Route path="/results/:taskId" element={<Results />} />
        </Routes>
      </MainLayout>
    </Layout>
  )
}

export default App
