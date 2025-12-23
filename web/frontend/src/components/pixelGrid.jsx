import { useState, useEffect } from "react";
import '../../public/styles/pixelGrid.css'

export const SIZE = 8;
export const SCALE = 100;

export function getPixelData() {

}

export default function PixelGrid({ color, clearSignal, undoSign, redoSign, updatePixels, pixels, setPixels }) {

    const [isDrawing, setIsDrawing] = useState(false);

    const [past, setPast] = useState([]);
    const [future, setFuture] = useState([]);

    const setPixel = (i) => {
        setPixels(prev => {
            const copy = [...prev];
            copy[i] = color;
            return copy;
        });
    };

    const beginStroke = () => {
        setPast(prev => [...prev, [...pixels]]);
        setFuture([]); // clear redo stack
        setIsDrawing(true);
    };

    const undo = () => {
        setPast(prev => {
            if (prev.length === 0) return prev;

            const previous = prev[prev.length - 1];

            setFuture(f => [[...pixels], ...f]);
            setPixels(previous);

            return prev.slice(0, -1);
        });
    };


    const redo = () => {
        setFuture(prev => {
            if (prev.length === 0) return prev;

            const next = prev[0];

            setPast(p => [...p, pixels]);
            setPixels(next);

            return prev.slice(1);
        });
    };


    useEffect(() => {
        if (clearSignal) {
            setFuture([]);
            setPixels(Array(SIZE * SIZE).fill("#ffffff"));
        }
    }, [clearSignal]);

    // useEffect(() => {
    //     if (saveSignal) {
    //         const data = JSON.stringify(pixels);
    //         localStorage.setItem("pixelGrid", data);
    //     }
    // }, [saveSignal]);

    // useEffect(() => {
    //     if (loadSignal) {
    //         const saved = localStorage.getItem("pixelGrid");
    //         if (!saved) return;

    //         const parse = JSON.parse(saved);
    //         if (parse.length === SIZE * SIZE) {
    //             setPast([]);
    //             setFuture([]);
    //             setPixels(parse);
    //         }
    //     }
    // }, [loadSignal]);

    useEffect(() => {
        if (undoSign) {
            undo()
        }
    }, [undoSign]);

    useEffect(() => {
        if (redoSign) {
            redo()
        }
    }, [redoSign]);

    useEffect(() => {
        if (!Array.isArray(updatePixels)) return;
        if (updatePixels.length !== SIZE * SIZE) return;
        setPixels(updatePixels);
    }, [updatePixels])

    return (
        <div className="grid-wrapper"
            style={{
                width: `calc(${SIZE} * ${SCALE + 0.2}px)`,
                height: `calc(${SIZE} * ${SCALE + 2}px)`
            }}
        >
            <div className="grid-frame" />
            <div className="pixelGrid"
                onMouseDown={beginStroke}
                onMouseUp={() => setIsDrawing(false)}
                onMouseLeave={() => setIsDrawing(false)}
                style={{
                    display: "grid",
                    gridTemplateColumns: `repeat(${SIZE}, ${SCALE}px)`,
                    userSelect: "none"
                }}
            >
                {pixels.map((color, i) => (
                    <div
                        key={i}
                        onMouseDown={() => setPixel(i)}
                        onMouseEnter={() => isDrawing && setPixel(i)}
                        style={{
                            width: `${SCALE}px`,
                            height: `${SCALE}px`,
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