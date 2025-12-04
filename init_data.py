from pathlib import Path

file1 = Path("src") / "data" / "DX1.50"
file2 = Path("data") / "DX1.50"

succ = 64119
timeout = 2119
error = 634

for file in [file1, file2]:
    with open(file, "wb") as f:
        f.truncate(0)
        f.write(succ.to_bytes(4, byteorder='little'))
        f.write(timeout.to_bytes(4, byteorder='little'))
        f.write(error.to_bytes(4, byteorder='little'))
        