STM32_Programmer_CLI \
-c port=SWD mode=UR reset=HWrst \
-w build/Release/RF_IO_V3.hex 0x08000000 \
-v \
-rst