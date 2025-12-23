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

  // Handle file upload for Load button
  const handleLoadFile = () => {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = 'image/*,video/*';
    
    input.onchange = async (e) => {
      const file = e.target.files[0];
      if (!file) return;

      const formData = new FormData();
      formData.append('file', file);

      // Determine if it's an image or video
      const isVideo = file.type.startsWith('video/');
      const endpoint = isVideo ? '/api/upload/video' : '/api/upload/image';

      try {
        const response = await fetch(`http://localhost:5000${endpoint}`, {
          method: 'POST',
          body: formData
        });

        const data = await response.json();
        
        if (response.ok) {
          console.log('File uploaded successfully:', data);
          alert(`${isVideo ? 'Video' : 'Image'} uploaded and processing started!`);
          // Trigger the load signal to update the grid
          setLoadSignal(v => v + 1);
        } else {
          console.error('Upload failed:', data);
          alert(`Upload failed: ${data.error}`);
        }
      } catch (error) {
        console.error('Error uploading file:', error);
        alert('Error uploading file. Make sure the backend is running.');
      }
    };

    input.click();
  };

  // // Handle save - this would typically save the current grid state
  // const handleSave = async () => {
  //   try {
  //     const response = await fetch('http://localhost:5000/api/send/custom', {
  //       method: 'POST',
  //       headers: {
  //         'Content-Type': 'application/json',
  //       },
  //       body: JSON.stringify({
  //         topic: 'grid/save',
  //         payload: {
  //           timestamp: new Date().toISOString(),
  //           // Add your grid data here
  //         }
  //       })
  //     });

  //     const data = await response.json();
      
  //     if (response.ok) {
  //       console.log('Grid saved successfully:', data);
  //       alert('Grid saved successfully!');
  //     } else {
  //       console.error('Save failed:', data);
  //       alert(`Save failed: ${data.error}`);
  //     }
  //   } catch (error) {
  //     console.error('Error saving:', error);
  //     alert('Error saving. Make sure the backend is running.');
  //   }
    
  //   setSaveSignal(v => v + 1);
  // };

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
            <a className="button" onClick={handleLoadFile}>
              Load
            </a>
          </div>
        </div>
      </div>
    </div>
  )
}

export default App