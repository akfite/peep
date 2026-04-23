import os
import glob

def main():
    asset_dir = "./"
    output_file = "EmbeddedAssets.h"
    
    files = glob.glob(os.path.join(asset_dir, "*.bin"))
    
    with open(output_file, "w") as out:
        out.write("#pragma once\n\n")
        out.write("#include <map>\n")
        out.write("#include <string>\n")
        out.write("#include <vector>\n\n")
        
        # Write individual arrays
        for f in files:
            name = os.path.splitext(os.path.basename(f))[0]
            var_name = f"DATA_{name.upper()}"
            
            with open(f, "rb") as binary_file:
                data = binary_file.read()
                
            out.write(f"// {name}\n")
            out.write(f"static const unsigned char {var_name}[] = {{")
            
            # Write bytes
            for i, byte in enumerate(data):
                if i % 16 == 0:
                    out.write("\n    ")
                out.write(f"0x{byte:02x}, ")
            
            out.write("\n};\n\n")
            
        # Write map function
        out.write("inline std::map<std::string, std::vector<unsigned char>> GetEmbeddedColormaps() {\n")
        out.write("    std::map<std::string, std::vector<unsigned char>> maps;\n")
        
        for f in files:
            name = os.path.splitext(os.path.basename(f))[0]
            var_name = f"DATA_{name.upper()}"
            size = os.path.getsize(f)
            
            out.write(f'    maps["{name}"] = std::vector<unsigned char>({var_name}, {var_name} + {size});\n')
            
        out.write("    return maps;\n")
        out.write("}\n")

if __name__ == "__main__":
    main()
