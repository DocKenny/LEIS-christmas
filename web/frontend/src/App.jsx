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
  const [undoSignal, setUndoSignal] = useState(0);
  const [redoSignal, setRedoSignal] = useState(0);

  return (
    <div className='container'>
      <div className='grid-container'>
        <div className='grid'>
          <PixelGrid color={color} clearSignal={clearSignal} saveSignal={saveSignal} loadSignal={loadSignal} undoSign={undoSignal} redoSign={redoSignal}/>
        </div>

        <div className='tool-bar'>
          <div>
            <ColorPicker color={color} onChange={setColor} />
          </div>
          <div className='control'
            style={{
              display: "flex",
              flexDirection: "column",
              justifyContent: "center",
              alignContent: "center",
              alignItems: "center",
              gap: 10
            }}
          >
            <a className='button' onClick={() => setUndoSignal(v => v + 1)}>
              Undo
            </a>
            <a className='button' onClick={() => setRedoSignal(v => v + 1)}>
              Redo
            </a>
          </div>
          <div className='btns'>

            <a className="button" onClick={() => setClearSignal(v => v + 1)}>
              Clear
            </a>
            <a className="button" onClick={() => setSaveSignal(v => v + 1)}>
              Save
            </a>
            <a className="button" onClick={() => setLoadSignal(v => v + 1)}>
              load
            </a>
          </div>
        </div>
      </div>
    </div>
  )
}

export default App
