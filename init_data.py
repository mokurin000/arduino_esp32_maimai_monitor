from pathlib import Path

filename = Path("maimai_monitor") / "data" / "DX1.50"

succ = 93642
timeout = 5437
error = 904

with open(filename, "wb") as f:
    f.truncate(0)
    f.write(succ.to_bytes(4, byteorder='little'))
    f.write(timeout.to_bytes(4, byteorder='little'))
    f.write(error.to_bytes(4, byteorder='little'))
    