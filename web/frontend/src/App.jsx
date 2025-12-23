import { useState } from 'react'
import PixelGrid, { SIZE, SCALE } from './components/pixelGrid'
import ColorPicker from './components/colorPicker';

import '../public/styles/App.css'
import '../public/styles/button.css'

function App() {
  const API_BASE = 'http://localhost:5000';

  const [color, setColor] = useState("#000000");
  const [clearSignal, setClearSignal] = useState(0);
  const [undoSignal, setUndoSignal] = useState(0);
  const [redoSignal, setRedoSignal] = useState(0);
  const [pixels, setPixels] = useState(
    Array(SIZE * SIZE).fill('#ffffff')
  );
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

      console.log(formData);

      // Determine if it's an image or video
      // GIFs are images by MIME type but should be processed as videos
      const isGif = file.type === 'image/gif' || file.name.toLowerCase().endsWith('.gif');
      const isVideo = file.type.startsWith('video/') || isGif;
      const endpoint = isVideo ? '/api/upload/video' : '/api/upload/image';

      try {
        const response = await fetch(`${API_BASE}${endpoint}`, {
          method: 'POST',
          body: formData
        });

        const data = await response.json();

        if (response.ok) {
          console.log('File uploaded successfully:', data);
          alert(`${isVideo ? 'Video' : 'Image'} uploaded and processing started!`);
          // Trigger the load signal to update the grid
          // setLoadSignal(v => v + 1);
          // Safely apply pixels from server if present and valid
          if (Array.isArray(data.pixelData) && data.pixelData.length === SIZE * SIZE) {
            setPixels(data.pixelData);
          } else {
            console.warn('Server did not return valid pixelData; keeping current grid.', data);
          }
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


  const handleUpload = async () => {
    const canvas = document.createElement("canvas");
    const scale = 32;

    canvas.width = SIZE * scale;
    canvas.height = SIZE * scale;

    const ctx = canvas.getContext("2d");

    pixels.forEach((color, i) => {
      const x = (i % SIZE) * scale;
      const y = Math.floor(i / SIZE) * scale;

      ctx.fillStyle = color;
      ctx.fillRect(x, y, scale, scale);
    });

    canvas.toBlob(async (blob) => {
      if (!blob) return;

      const url = URL.createObjectURL(blob);

      const file_name = `pixel_art_${Date.now()}.png`

      const a = document.createElement("a");
      a.href = url;
      a.download = file_name;   // filename for the saved file
      a.click();

      URL.revokeObjectURL(url);
      const endpoint = '/api/upload/image';


      const formData = new FormData();
      formData.append('file', blob, file_name);
      try {
        const response = await fetch(`${API_BASE}${endpoint}`, {
          method: 'POST',
          body: formData
        });

        const data = await response.json();

        if (response.ok) {
          console.log('File uploaded successfully:', data);
          alert('Image uploaded and processing started!');
          // Trigger the load signal to update the grid
          // setLoadSignal(v => v + 1);
          // Safely apply pixels from server if present and valid
          if (Array.isArray(data.pixelData) && data.pixelData.length === SIZE * SIZE) {
            setPixels(data.pixelData);
          } else {
            console.warn('Server did not return valid pixelData; keeping current grid.', data);
          }
        } else {
          console.error('Upload failed:', data);
          alert(`Upload failed: ${data.error}`);
        }
      } catch (error) {
        console.error('Error uploading file:', error);
        alert('Error uploading file. Make sure the backend is running.');
      }
    }, "image/png");
  };

  return (
    <div className='container'>
      <div className='grid-container'>
        <div className='grid'>
          <PixelGrid color={color} clearSignal={clearSignal} undoSign={undoSignal} redoSign={redoSignal} pixels={pixels} setPixels={setPixels} />
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
            <a className="button" onClick={handleUpload}>
              Upload
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