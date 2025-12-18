import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import App from './App.jsx'
import '../public/styles/index.css'
import LightRope from './lightRope.jsx'
createRoot(document.getElementById('root')).render(
  <StrictMode>
    <LightRope />
    <div className='main-conteiner'>
      <App />
    </div>
  </StrictMode>,
)
