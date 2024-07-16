sh compile.sh 2> /dev/null

# find all the folders in the testcases/c folder
folders=$(find . -mindepth 1 -maxdepth 1 -type d)

# cd to each folder and run the test.sh script
for folder in $folders
do
    cd $folder
    sh test.sh
    cd ..
done
