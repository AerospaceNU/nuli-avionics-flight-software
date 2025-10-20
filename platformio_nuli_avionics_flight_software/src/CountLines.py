import os

def count_lines_in_file(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            return sum(1 for _ in f)
    except Exception as e:
        print(f"Could not read {file_path}: {e}")
        return 0

def count_lines_in_folder(folder_path, extensions=None):
    total_lines = 0
    for root, _, files in os.walk(folder_path):
        for file in files:
            if extensions is None or any(file.endswith(ext) for ext in extensions):
                file_path = os.path.join(root, file)
                total_lines += count_lines_in_file(file_path)
    return total_lines

if __name__ == "__main__":
    current_folder = os.getcwd()
    extensions = None  # or e.g. ['.py', '.cpp', '.h']
    total = count_lines_in_folder(current_folder, extensions)
    print(f"Total lines in folder '{current_folder}': {total}")
