
import sys

def check_braces(filename):
    with open(filename, 'r') as f:
        lines = f.readlines()

    level = 0
    namespace_level = 0
    
    for i, line in enumerate(lines):
        original_line = line
        line = line.strip()
        # Remove comments
        if '//' in line:
            line = line.split('//')[0]
        
        # Simple brace counting (ignoring strings/chars for now, assuming code is simple enough)
        open_braces = line.count('{')
        close_braces = line.count('}')
        
        prev_level = level
        level += open_braces - close_braces
        
        if line.startswith('namespace '):
            namespace_level += 1
            print(f"Line {i+1}: Namespace start. Level: {level}")

        if line.startswith('} // namespace'):
            namespace_level -= 1
            print(f"Line {i+1}: Namespace end. Level: {level}")
            
        # Check for function definitions (heuristic)
        if 'ImGuiManager::' in line and '(' in line and ')' in line and '{' in line:
             print(f"Line {i+1}: Function start '{line}'. Level: {prev_level} -> {level}")
             
        if level < 0:
            print(f"ERROR: Level dropped below 0 at line {i+1}: {original_line.strip()}")
            return

    print(f"Final Level: {level}")
    if level != 0:
        print("ERROR: Unbalanced braces!")

if __name__ == "__main__":
    check_braces("/home/duong/MediaPlayerApp/src/ui/ImGuiManager.cpp")
