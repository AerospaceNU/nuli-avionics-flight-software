import os
import sys
import glob
import shutil
from datetime import datetime

# --- BOARD SPECIFIC DATA ---
# Configuration mapping for different MCU families
# SAMD21 (M0) typically uses 8KB bootloader (0x2000)
# SAMD51 (M4) typically uses 16KB bootloader (0x4000)
BOARD_DATA = {
    "samd21g18a": {
        "family_id": "0x68ed2b88",
        "start_address": "0x2000"
    },
    "samd51j19a": {
        "family_id": "0x55114460",
        "start_address": "0x4000"  # Correct offset for Feather M4 Express
    }
}
# ---------------------------

Import("env")

def objcopy_to_uf2(source, target, env):
    board_mcu = env.get("BOARD_MCU", "").lower()

    # Validation: Check if the board exists in our configuration
    if board_mcu not in BOARD_DATA:
        print(f"--- Warning: MCU '{board_mcu}' not supported for UF2 conversion. Continuing... ---")
        return

    # Extract specific config
    config = BOARD_DATA[board_mcu]

    # Define paths
    bin_path = os.path.abspath(str(target[0]))
    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    env_name = env.subst("$PIOENV")
    timestamp = datetime.now().strftime("%Y%m%d-%H%M%S")
    uf2_build_path = os.path.join(build_dir, "firmware.uf2")
    uf2_final_name = f"{env_name}_{timestamp}.uf2"
    uf2_final_path = os.path.join(project_dir, uf2_final_name)
    script_path = os.path.join(project_dir, "uf2", "uf2conv.py")

    print(f"--- Generating UF2 for {board_mcu.upper()} ---")

    cmd = [
        f'"{sys.executable}"',
        f'"{script_path}"',
        "-c",
        "-b", config["start_address"],
        "-f", config["family_id"],
        "-o", f'"{uf2_build_path}"',
        f'"{bin_path}"'
    ]

    full_cmd = " ".join(cmd)

    # Run the conversion
    exit_code = env.Execute(full_cmd)

    if exit_code == 0 and os.path.exists(uf2_build_path):
        # Prune previous UF2s for this env so each env keeps only its latest.
        for stale in glob.glob(os.path.join(project_dir, f"{env_name}_*.uf2")):
            try:
                os.remove(stale)
            except OSError:
                pass
        # Also clean up legacy artifacts from older versions of this script.
        legacy = os.path.join(project_dir, "latest_firmware.uf2")
        if os.path.exists(legacy):
            try:
                os.remove(legacy)
            except OSError:
                pass
        shutil.copyfile(uf2_build_path, uf2_final_path)
        print(f"--- Success! UF2 saved to project root as '{uf2_final_name}' ---")
    else:
        print(f"--- Error: UF2 conversion failed for {board_mcu} ---")

# Hook into the build process after the .bin is created
env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", objcopy_to_uf2)