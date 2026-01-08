from pathlib import Path

file1 = Path("src") / "data" / "DX1.53"
file2 = Path("data") / "DX1.53"

succ = 0
timeout = 0
error = 0

for file in [file1, file2]:
    with open(file, "wb") as f:
        f.truncate(0)
        f.write(succ.to_bytes(4, byteorder="little"))
        f.write(timeout.to_bytes(4, byteorder="little"))
        f.write(error.to_bytes(4, byteorder="little"))
