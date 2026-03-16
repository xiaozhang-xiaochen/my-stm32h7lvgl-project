import os

files = [
    r"Drivers\BSP\NAND\ftl.c",
    r"Drivers\BSP\NAND\ftl.h",
    r"Drivers\BSP\NAND\nand.c",
    r"Drivers\BSP\NAND\nand.h",
    r"Drivers\BSP\NAND\nandtester.c",
    r"Drivers\BSP\NAND\nandtester.h"
]

base_path = r"F:\stm32H743\实验15 LTDC LCD（RGB屏）实验"

for rel_path in files:
    abs_path = os.path.join(base_path, rel_path)
    if not os.path.exists(abs_path):
        print(f"File not found: {abs_path}")
        continue
    
    try:
        # Try GBK first, then maybe CP936 or others if needed
        # GBK is common for Chinese
        with open(abs_path, 'rb') as f:
            raw_data = f.read()
        
        # Try decoding as GBK
        try:
            content = raw_data.decode('gbk')
        except UnicodeDecodeError:
            # If GBK fails, try GB18030 which is a superset
            content = raw_data.decode('gb18030', errors='replace')
        
        # Write as UTF-8
        with open(abs_path, 'w', encoding='utf-8') as f:
            f.write(content)
        print(f"Converted: {abs_path}")
    except Exception as e:
        print(f"Error converting {abs_path}: {e}")
