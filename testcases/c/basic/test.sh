testcases=$(ls *.s)
for testcase in $testcases; do
    echo "\033[32mRunning $testcase\033[0m"
    touch ${testcase%.s}.in
    sh ./run.sh $testcase ${testcase%.s}.in
done
