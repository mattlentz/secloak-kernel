#!/bin/bash

# Dependencies: python and cloc

alias runcloc='cloc --by-file-by-lang --include-lang="C","C/C++ Header","Assembly"'
shopt -s expand_aliases

runcloc --exclude-list-file core_exclude.txt ../core | ./cloc_parse.py "Core"
echo ""
runcloc ../core/drivers/dt.c ../core/include/drivers/dt.h | ./cloc_parse.py "Drivers (DT)"
runcloc ../core/drivers/imx_csu.c ../core/include/drivers/imx_csu.h | ./cloc_parse.py "Drivers (IMX CSU)"
runcloc ../core/drivers/imx_fb.c ../core/include/drivers/imx_fb.h | ./cloc_parse.py "Drivers (IMX FB)"
runcloc ../core/drivers/imx_gpio.c ../core/include/drivers/imx_gpio.h | ./cloc_parse.py "Drivers (IMX GPIO)"
runcloc ../core/drivers/imx_gpio_keys.c ../core/include/drivers/imx_gpio_keys.h | ./cloc_parse.py "Drivers (IMX GPIO Keypad)"
runcloc --list-file drivers_other.txt | ./cloc_parse.py "Drivers (Other)"
runcloc --list-file drivers_all.txt | ./cloc_parse.py "Drivers"
echo ""
runcloc ../core/lib/libfdt | ./cloc_parse.py "Libraries (libfdt)"
runcloc --list-file bget.txt | ./cloc_parse.py "Libraries (bget)"
runcloc ../lib --exclude-list-file bget.txt | ./cloc_parse.py "Libraries (other)"
runcloc ../lib ../core/lib | ./cloc_parse.py "Libraries"
echo ""
runcloc --exclude-list-file total_exclude.txt ../core ../lib | ./cloc_parse.py "Total"

