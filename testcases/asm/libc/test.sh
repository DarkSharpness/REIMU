testcases=$(ls *.s)
for testcase in $testcases; do
    echo "\033[32mRunning $testcase\033[0m"
    sh ./run.sh $testcase
done
