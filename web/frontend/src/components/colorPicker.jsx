import { SketchPicker } from "react-color";

export default function ColorPicker({ color, onChange }) {
  return (
    <SketchPicker
      color={color}
      onChangeComplete={(c) => onChange(c.hex)}
    />
  );
}
