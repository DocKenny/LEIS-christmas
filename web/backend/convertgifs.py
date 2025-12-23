from moviepy import VideoFileClip
def gif_to_mp4(input_gif_path: str, output_mp4_path: str):
    clip = VideoFileClip(input_gif_path)
    clip.write_videofile(
        output_mp4_path,
        codec='libx264',       # Standard MP4 codec
        fps=clip.fps,          # Keep original frame rate
        audio=False,           # GIFs don’t have audio
        ffmpeg_params=["-pix_fmt", "yuv420p"]  # Fix green video issue
    )

# Example usage
file_name = "bunny_boom"
gif_to_mp4(f"{file_name}.gif", f"{file_name}.mp4")
