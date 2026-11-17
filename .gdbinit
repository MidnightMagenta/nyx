file build/sys/vmnyx
#file image
target remote localhost:1234
set step-mode on

source ./tools/scripts/gdb/utils.py

# breakpoints

define savebp
    mkdir -p tmp
    save breakpoints tmp/bp.gdb
end

define rmbp
    delete
    shell rm -f tmp/bp.gdb
end

python
import os
if os.path.exists("./tmp/bp.gdb"):
    gdb.execute("source ./tmp/bp.gdb")
end
