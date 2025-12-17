import { useState } from "react";

const SIZE = 8;
const pixelSize = 100;

export default function PixelGrid({ color }) {
    const [pixels, setPixels] = useState(
        Array(SIZE * SIZE).fill('#ffffff')
    );

    const setPixel = (i) => {
        const copy = [...pixels];
        copy[i] = color;
        setPixels(copy);
    }

    return (
        <div
            style={{
                display: "grid",
                gridTemplateColumns: `repeat(${SIZE}, ${pixelSize}px)`,
            }}
        >
            {pixels.map((color, i) => (
                <div
                    key={i}
                    onClick={() => setPixel(i)}
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
    );
}