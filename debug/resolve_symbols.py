import subprocess
import argparse

def run_dia2dump(pdb_path, addresses, verbose=False, dia2dump_path="dia2dump.exe"):
    for addr in addresses:
        addr_hex = f"0x{addr:X}" if isinstance(addr, int) else addr
        cmd = [dia2dump_path, "-sym", addr_hex, pdb_path]

        try:
            proc = subprocess.run(cmd, capture_output=True, text=True)
            output = proc.stdout.strip().splitlines()

            if verbose:
                print(f"Address {addr_hex} full output:\n{'-'*50}")
                print("\n".join(output))
                print("\n")
            else:
                # Find the line that starts with "Function"
                func_line = next((line for line in output if line.startswith("Function")), None)
                if func_line:
                    # Extract function name after "Function: "
                    func_name = func_line.split(":", 1)[1].strip() if ":" in func_line else func_line
                    print(f"{addr_hex} : {func_name}")
                else:
                    print(f"{addr_hex} : <No function found>")

        except Exception as e:
            print(f"{addr_hex} : <Error: {e}>")


def main():
    parser = argparse.ArgumentParser(
        description="Resolve function names from addresses using dia2dump.exe and a PDB."
    )
    parser.add_argument(
        "pdb",
        help="Path to the PDB file"
    )
    parser.add_argument(
        "addresses",
        nargs="+",
        help="Addresses to resolve (hex 0xNNNN or decimal)"
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Print full dia2dump output for each address"
    )
    parser.add_argument(
        "--dia2dump",
        default="dia2dump.exe",
        help="Path to dia2dump.exe (default: dia2dump.exe in PATH)"
    )

    args = parser.parse_args()

    # Convert addresses to integers, auto-detect hex if prefixed with 0x
    addr_list = [int(a, 0) for a in args.addresses]

    run_dia2dump(args.pdb, addr_list, verbose=args.verbose, dia2dump_path=args.dia2dump)


if __name__ == "__main__":
    main()
