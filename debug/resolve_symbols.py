import subprocess
import argparse
import re

def clean_function_line(line):
    """
    Cleans function lines by removing bracketed segments and len info,
    and removing empty commas/spaces.
    """
    line = re.sub(r"\[.*?\]", "", line)               # remove bracketed addresses
    line = re.sub(r"len\s*=\s*0x?[0-9A-Fa-f]+", "", line)  # remove len info
    parts = [p.strip() for p in line.split(",") if p.strip()]
    return ", ".join(parts)

def clean_definition_or_callsite_line(line):
    """
    Cleans definition or callsite lines:
    - Removes all brackets and len info
    - Removes trailing hex hashes
    - Removes extra commas
    - Extracts only 'line <num> at <file path>'
    """
    if not line:
        return "<No location info>"

    # Remove bracketed segments
    line = re.sub(r"\[.*?\]", "", line)
    # Remove len info
    line = re.sub(r"len\s*=\s*0x?[0-9A-Fa-f]+", "", line)
    # Remove trailing parenthetical hex hashes
    line = re.sub(r"\(0x[0-9A-Fa-f:]+\)", "", line)
    # Remove extra commas
    line = re.sub(r",", "", line)
    # Collapse multiple spaces
    line = re.sub(r"\s+", " ", line)

    # Extract only "line <num> at <file path>"
    match = re.search(r"(line \d+ at [A-Z]:\\[^\s]+)", line)
    if match:
        return match.group(1)
    return "<No location info>"

def run_dia2dump(pdb_path, function_rvas, dia2dump_path="dia2dump.exe"):
    print("=== FUNCTIONS CALLED ===")
    for addr in function_rvas:
        addr_hex = f"0x{addr:X}" if isinstance(addr, int) else addr
        cmd = [dia2dump_path, "-sym", addr_hex, pdb_path]

        proc = subprocess.run(cmd, capture_output=True, text=True)
        output = [l.strip() for l in proc.stdout.strip().splitlines() if l.strip()]

        # Function line
        func_line = next((line for line in output if line.startswith("Function")), None)
        func_name = func_line.split(":", 1)[1].strip() if func_line else "<No function found>"
        func_clean = clean_function_line(func_name)
        print(f"[FUNCTION RVA @ {addr_hex}] {func_clean}")

    print("\n=== CALLER SITES ===")
    for addr in function_rvas:
        addr_hex = f"0x{addr:X}" if isinstance(addr, int) else addr
        cmd = [dia2dump_path, "-l", addr_hex, "0x1", pdb_path]  # use actual RVA length if available

        proc = subprocess.run(cmd, capture_output=True, text=True)
        output = [l.strip() for l in proc.stdout.strip().splitlines() if l.strip()]
        line_to_clean = output[0] if output else "<No output>"
        cleaned = clean_definition_or_callsite_line(line_to_clean)
        print(f"[CALLER FOR @ {addr_hex}] {cleaned}")

def main():
    parser = argparse.ArgumentParser(description="Resolve addresses via dia2dump.exe")
    parser.add_argument("pdb", help="Path to PDB")
    parser.add_argument("--function-rvas", nargs="+", required=True, help="Function RVA addresses")
    parser.add_argument("--dia2dump", default="dia2dump.exe", help="Path to dia2dump.exe")

    args = parser.parse_args()
    function_rvas = [int(a, 0) for a in args.function_rvas]

    run_dia2dump(args.pdb, function_rvas, dia2dump_path=args.dia2dump)

if __name__ == "__main__":
    main()
