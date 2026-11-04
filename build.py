import sys
import shutil
import subprocess
from pathlib import Path

def main():
    build_dir = Path("build")
    if build_dir.exists():
        shutil.rmtree(build_dir)

    executable = Path("armulator")
    if executable.exists():
        if executable.is_dir():
            shutil.rmtree(executable)
        else:
            executable.unlink()

    build_dir.mkdir()
    subprocess.run(["cmake", ".."], cwd=build_dir, text=True, check=True)
    subprocess.run(["cmake", "--build", "."], cwd=build_dir, text=True, check=True)

    built_executable = build_dir / "armulator"
    if built_executable.exists():
        shutil.copy2(built_executable, executable)

if __name__ == "__main__":
    try:
        main()
        if "--run" in sys.argv[1:]:
            subprocess.run(["./armulator"])
    except Exception as e:
        print(f"An error occurred: {e}")