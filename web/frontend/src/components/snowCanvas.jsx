import { useEffect, useRef } from "react";

export default function SnowCanvas({ maxSnowflakes = 100, inFront = false, color }) {
  const canvasRef = useRef(null);
  let animationId = useRef(null);
  let lastNow = useRef(performance.now());
  let snowflakes = useRef([]);

  let width, height;

  useEffect(() => {
    const canvas = canvasRef.current;
    const ctx = canvas.getContext("2d");

    function resize() {
      width = canvas.width = window.innerWidth;
      height = canvas.height = window.innerHeight;
    }

    class Snowflake {
      constructor() { this.spawn(); }
      spawn(anyY = false) {
        this.x = rand(0, width);
        this.y = anyY ? rand(-50, height + 50) : rand(-50, -10);
        this.xVel = rand(-0.05, 0.05);
        this.yVel = rand(0.02, 0.1);
        this.angle = rand(0, Math.PI * 2);
        this.angleVel = rand(-0.001, 0.001);
        this.size = rand(7, 12);
      }
      update(elapsed) {
        const xForce = rand(-0.001, 0.001);
        if (Math.abs(this.xVel + xForce) < 0.075) this.xVel += xForce;
        this.x += this.xVel * elapsed;
        this.y += this.yVel * elapsed;
        this.angle += this.xVel * 0.05 * elapsed;
        if (this.y - this.size > height || this.x + this.size < 0 || this.x - this.size > width) {
          this.spawn();
        }
        this.render();
      }
      render() {
        ctx.save();
        ctx.beginPath();
        ctx.arc(this.x, this.y, this.size * 0.2, 0, Math.PI * 2);
        ctx.fillStyle = color;
        ctx.fill();
        ctx.restore();
      }
    }

    function rand(min, max) { return min + Math.random() * (max - min); }

    function render(now) {
      animationId.current = requestAnimationFrame(render);
      const elapsed = now - lastNow.current;
      lastNow.current = now;

      ctx.clearRect(0, 0, width, height);

      if (snowflakes.current.length < maxSnowflakes) {
        snowflakes.current.push(new Snowflake());
      }

      snowflakes.current.forEach(s => s.update(elapsed));
    }

    function handleBlur() { cancelAnimationFrame(animationId.current); }
    function handleFocus() { lastNow.current = performance.now(); animationId.current = requestAnimationFrame(render); }

    window.addEventListener("resize", resize);
    window.addEventListener("blur", handleBlur);
    window.addEventListener("focus", handleFocus);

    resize();
    render(lastNow.current);

    // cleanup on unmount
    return () => {
      cancelAnimationFrame(animationId.current);
      window.removeEventListener("resize", resize);
      window.removeEventListener("blur", handleBlur);
      window.removeEventListener("focus", handleFocus);
    };
  }, []);

  return <canvas ref={canvasRef}
    style={{
      position: 'absolute',
      top: 0,
      left: 0,
      width: '100%',
      height: '100%',
      pointerEvents: 'none', // ensures clicks go through
      zIndex: inFront ? 5 : 0,
      display: 'block',      // remove default inline spacing
    }} />;
}
