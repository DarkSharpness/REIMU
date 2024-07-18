# find all .c files in the testcases directory
test_files=$(find . -name "*.c")

# compile all the .c files
for file in $test_files; do
    riscv64-linux-gnu-gcc $file -S -march=rv32im -mabi=ilp32 -nostdlib -O2 -o ${file%.c}.s -fno-section-anchors
done

# remove @plt and @got suffixes from the assembly files
for file in $test_files; do
    sed -i 's/@plt//g' ${file%.c}.s
    sed -i 's/@got//g' ${file%.c}.s
done
