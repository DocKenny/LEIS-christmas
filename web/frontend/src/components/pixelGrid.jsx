import { useState, useEffect } from "react";
import '../../public/styles/pixelGrid.css'

const SIZE = 8;
const pixelSize = 100;

export default function PixelGrid({ color, clearSignal, saveSignal, loadSignal }) {
    const [pixels, setPixels] = useState(
        Array(SIZE * SIZE).fill('#ffffff')
    );
    const [isDrawing, setIsDrawing] = useState(false);

    const setPixel = (i) => {
        setPixels(prev => {
            const copy = [...prev];
            copy[i] = color;
            return copy;
        })
    }

    useEffect(() => {
        if (clearSignal) {
            setPixels(Array(SIZE * SIZE).fill("#ffffff"));
        }
    }, [clearSignal]);

    useEffect(() => {
        if (saveSignal) {
            const data = JSON.stringify(pixels);
            localStorage.setItem("pixelGrid", data);
        }
    }, [saveSignal]);

    useEffect(() => {
        if (loadSignal) {
            const saved = localStorage.getItem("pixelGrid");
            if (!saved) return;

            const parse = JSON.parse(saved);
            if (parse.length === SIZE * SIZE) {
                setPixels(parse);
            }
        }
    }, [loadSignal]);

    return (
        <div className="grid-wrapper"
            style={{
                width: `calc(${SIZE} * ${pixelSize+0.2}px)`,
                height: `calc(${SIZE} * ${pixelSize+2}px)`
            }}
        >
            <div className="grid-frame" />
            <div className="pixelGrid"
                onMouseDown={() => setIsDrawing(true)}
                onMouseUp={() => setIsDrawing(false)}
                onMouseLeave={() => setIsDrawing(false)}
                style={{
                    display: "grid",
                    gridTemplateColumns: `repeat(${SIZE}, ${pixelSize}px)`,
                    userSelect: "none"
                }}
            >
                {pixels.map((color, i) => (
                    <div
                        key={i}
                        onMouseDown={() => setPixel(i)}
                        onMouseEnter={() => isDrawing && setPixel(i)}
                        style={{
                            width: `${pixelSize}px`,
                            height: `${pixelSize}px`,
                            backgroundColor: color,
                            border: "1px solid #333",
                            cursor: "pointer",
                        }}
                    />
                ))}
            </div>
        </div>
    );
}