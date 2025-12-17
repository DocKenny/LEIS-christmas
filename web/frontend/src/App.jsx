import { useState } from 'react'
import PixelGrid from './components/pixelGrid'
import ColorPicker from './components/colorPicker';

function App() {

  const [color, setColor] = useState("#000000");

  return (
    <>
      <ColorPicker color={color} onChange={setColor} />
      <PixelGrid color={color} />
    </>
  )
}

export default App
