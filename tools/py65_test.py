from py65.devices.mpu6502 import MPU

mpu = MPU()

with open("/home/chinmay/Work/codebases/mos6502/tests/functional/6502_functional_test.bin", "rb") as f:
    data = f.read()

for i, b in enumerate(data):
    mpu.memory[i] = b

mpu.pc = 0x0400

seen_states = set()

while True:
    pc = mpu.pc
    opcode = mpu.memory[pc]

    # Detect JMP absolute to self
    if opcode == 0x4C:
        lo = mpu.memory[pc + 1]
        hi = mpu.memory[pc + 2]
        target = (hi << 8) | lo

        if target == pc:
            print(f"{pc:04X} {mpu.a:02X} {mpu.x:02X} {mpu.y:02X} "
                  f"{mpu.sp:02X} {mpu.p:02X} {opcode:02X}")
            break

    print(f"{pc:04X} {mpu.a:02X} {mpu.x:02X} {mpu.y:02X} "
          f"{mpu.sp:02X} {mpu.p:02X} {opcode:02X}")

    mpu.step()