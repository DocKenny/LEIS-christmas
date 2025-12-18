import { useState } from 'react'
import PixelGrid from './components/pixelGrid'
import ColorPicker from './components/colorPicker';

import '../public/styles/App.css'
import '../public/styles/button.css'

function App() {

  const [color, setColor] = useState("#000000");
  const [clearSignal, setClearSignal] = useState(0);
  const [saveSignal, setSaveSignal] = useState(0);
  const [loadSignal, setLoadSignal] = useState(0);

  return (
    <div className='container'>
      <div className='grid-container'>
        <div className='grid'>
          <PixelGrid color={color} clearSignal={clearSignal} saveSignal={saveSignal} loadSignal={loadSignal} />
        </div>

        <div style={{
          height: "97%",
          padding: "10px",
          borderRadius: "20px",
          boxShadow: "0 10px 10px rgba(0, 0, 0, 0.4)",

          display: "flex",
          flexDirection: 'column',
          justifyContent: 'space-between'
        }}>
          <div>
            <ColorPicker color={color} onChange={setColor} />
          </div>
          <div className='btns'>
            <a class="button" onClick={() => setClearSignal(v => v + 1)}>
              Clear
            </a>
            <a class="button" onClick={() => setSaveSignal(v => v + 1)}>
              Save
            </a>
            <a class="button" onClick={() => setLoadSignal(v => v + 1)}>
              load
            </a>
          </div>
        </div>
      </div>
    </div>
  )
}

export default App
