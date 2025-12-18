import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import App from './App.jsx'
import LightRope from './lightRope.jsx'
import SnowCanvas from './components/snowCanvas.jsx'

import '../public/styles/index.css'

createRoot(document.getElementById('root')).render(
  <StrictMode>
    <div className="canvas-wrapper">
      <LightRope />
      <SnowCanvas maxSnowflakes={100} color={"#fff"} />
      <SnowCanvas maxSnowflakes={50} inFront={true} color={"#fff"} />
      <div id='foreground'>
        <div className='main-conteiner'>
          <App />
        </div>
      </div>
    </div>
  </StrictMode>,
)
