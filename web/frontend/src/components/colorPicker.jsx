import { SketchPicker, ChromePicker } from "react-color";
import '../../public/styles/colorPicker.css'

export default function ColorPicker({ color, onChange }) {
  return (
    <div className="picker-wrapper">
      <div className="picker-frame"></div>
      <div className="color-picker">
        <SketchPicker
          color={color}
          onChangeComplete={(c) => onChange(c.hex)}
        />
      </div>
    </div>
  );
}
