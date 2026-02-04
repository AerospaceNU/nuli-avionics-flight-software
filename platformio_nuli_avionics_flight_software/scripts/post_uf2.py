Import("env")
import os
import sys
import shutil

# SAMD21 = 0x68ed2b88 | SAMD51 = 0x55114460
board_mcu = env.get("BOARD_MCU", "").lower()
family_id = "0x68ed2b88" if "samd21" in board_mcu else "0x55114460"

def objcopy_to_uf2(source, target, env):
    # 'target[0]' is now guaranteed to be the .bin file
    # because of the AddPostAction change at the bottom.
    bin_path = os.path.abspath(str(target[0]))

    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")

    uf2_build_path = os.path.join(build_dir, "firmware.uf2")
    uf2_final_path = os.path.join(project_dir, "latest_firmware.uf2")
    script_path = os.path.join(project_dir, "scripts", "uf2conv.py")

    print(f"--- Generating UF2 from {os.path.basename(bin_path)} ---")

    # Using 0x2000 for Feather M0 (matches your known-good 'adfs.UF2')
    cmd = [
        f'"{sys.executable}"',
        f'"{script_path}"',
        "-c",
        "-b", "0x2000",
        "-f", family_id,
        "-o", f'"{uf2_build_path}"',
        f'"{bin_path}"'
    ]

    full_cmd = " ".join(cmd)
    print(f"Executing: {full_cmd}")
    env.Execute(full_cmd)

    if os.path.exists(uf2_build_path):
        shutil.copyfile(uf2_build_path, uf2_final_path)
        print(f"--- Success! UF2 copied to project root ---")

# THIS IS THE CRITICAL CHANGE:
# Change from .elf to .bin to ensure the file exists before running.
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", objcopy_to_uf2)