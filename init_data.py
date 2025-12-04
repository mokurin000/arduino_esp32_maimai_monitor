from pathlib import Path

filename = Path("maimai_monitor") / "data" / "DX1.50"

succ = 60087
timeout = 1986
error = 595

with open(filename, "wb") as f:
    f.write(succ.to_bytes(4, byteorder='little'))
    f.write(timeout.to_bytes(4, byteorder='little'))
    f.write(error.to_bytes(4, byteorder='little'))
    