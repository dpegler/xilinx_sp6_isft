
copy /Y ..\SDK\SDK_Workspace_35\hw_platform_0\download.bit .

copy /Y ..\SDK\SDK_Workspace_35\bootloader\Debug\bootloader.elf .
copy /Y ..\SDK\SDK_Workspace_35\hello_uart\Debug\hello_uart.elf .
copy /Y ..\SDK\SDK_Workspace_35\hello_gpio\Debug\hello_gpio.elf .
copy /Y ..\SDK\SDK_Workspace_35\hello_timer\Debug\hello_timer.elf .
copy /Y ..\SDK\SDK_Workspace_35\hello_flash\Debug\hello_flash.elf .
copy /Y ..\SDK\SDK_Workspace_35\hello_iic\Debug\hello_iic.elf .
copy /Y ..\SDK\SDK_Workspace_35\hello_temac\Debug\hello_temac.elf .
copy /Y ..\SDK\SDK_Workspace_35\hello_switch\Debug\hello_switch.elf .
copy /Y ..\SDK\SDK_Workspace_35\hello_mem\Debug\hello_mem.elf .
copy /Y ..\SDK\SDK_Workspace_35\hello_spi\Debug\hello_spi.elf .
copy /Y ..\SDK\SDK_Workspace_35\hello_button\Debug\hello_button.elf .
copy /Y ..\SDK\SDK_Workspace_35\hello_sysace\Debug\hello_sysace.elf .
copy /Y ..\SDK\SDK_Workspace_35\colorbar\Debug\colorbar.elf .

%XILINX_EDK%\gnu\microblaze\nt\bin\mb-objcopy -O srec hello_uart.elf   cf_image\bist\1.rec
%XILINX_EDK%\gnu\microblaze\nt\bin\mb-objcopy -O srec hello_gpio.elf   cf_image\bist\2.rec
%XILINX_EDK%\gnu\microblaze\nt\bin\mb-objcopy -O srec hello_timer.elf  cf_image\bist\3.rec
%XILINX_EDK%\gnu\microblaze\nt\bin\mb-objcopy -O srec hello_flash.elf  cf_image\bist\4.rec
%XILINX_EDK%\gnu\microblaze\nt\bin\mb-objcopy -O srec hello_iic.elf    cf_image\bist\5.rec
%XILINX_EDK%\gnu\microblaze\nt\bin\mb-objcopy -O srec hello_temac.elf  cf_image\bist\6.rec
%XILINX_EDK%\gnu\microblaze\nt\bin\mb-objcopy -O srec hello_switch.elf cf_image\bist\7.rec
%XILINX_EDK%\gnu\microblaze\nt\bin\mb-objcopy -O srec hello_mem.elf    cf_image\bist\8.rec
%XILINX_EDK%\gnu\microblaze\nt\bin\mb-objcopy -O srec hello_spi.elf    cf_image\bist\9.rec
%XILINX_EDK%\gnu\microblaze\nt\bin\mb-objcopy -O srec hello_button.elf cf_image\bist\A.rec
%XILINX_EDK%\gnu\microblaze\nt\bin\mb-objcopy -O srec hello_sysace.elf cf_image\bist\B.rec
%XILINX_EDK%\gnu\microblaze\nt\bin\mb-objcopy -O srec colorbar.elf     cf_image\bist\C.rec

xmd -tcl genace.tcl -hw download.bit -ace ./cf_image/XILINX/cfg0/sp605_bist.ace -board sp605 -target mdm

del /F *.svf
del /F cf_image\XILINX\cfg0\*.svf
