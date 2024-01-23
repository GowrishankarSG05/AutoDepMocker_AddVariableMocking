# The following is a list of actions performed when executing this script
# 1. Install clang dependency
# 2. Install python dependency
# 3. Build AutoDepMocker
# 4. Copy the executable to ~/.bin/ and add the same path in PATH env variable

# Check clang-9 installed
echo -e '\033[1;33m'"Checking clang and llvm..." '\033[0m'
if [ $(which clang++-9 2>/dev/null | wc -l) -eq 0 ];
then
    echo -e '\033[32m'"Installing clang-9.0 and llvm-9.0 packages" '\033[0m'
    sudo apt-get install clang-9 libclang-9-dev llvm-9-dev;
else
    echo -e '\033[1;33m'"Package already installed!" '\033[0m'
fi

# Check python2.7 installed
echo -e '\033[1;33m'"Checking python2.7..." '\033[0m'
if [ $(which python2.7 2>/dev/null | wc -l) -eq 0 ];
then
    echo -e '\033[32m'"Installing python2.7 packages" '\033[0m'
    sudo apt-get install python2.7;
else
    echo -e '\033[1;33m'"Package already installed!" '\033[0m'
fi

# Build AutoDepMocker
echo -e '\033[1;33m'"Building Mock generator..." '\033[0m'
mkdir -p build;
cd build;
rm -rf *;
cmake ../; cmake --build .;
result=$?
if [ ${result} -eq 0 ];
then
    echo -e '\033[32m'"AutoDepMocker has been build successfully!"'\033[0m'
else 
    echo -e '\033[31m'"Failed to build AutoDepMocker"'\033[0m'
    exit;
fi
cd ../;

# Install executables and py script to ~/home/user/.bin
mkdir -p ~/.bin;
cp ./build/AutoDepMocker ~/.bin/;
cp ./Scripts/AutoDepMocker_YoctoCMake.py ~/.bin/;

# Export ~/.bin
echo -e '\033[1;33m'"Utilities have been installed to ~/.bin. Would you like to add ~/.bin path to ~/.bashrc [y/n] ?" '\033[0m'
read -n 1 input
echo $input
if [[ $input == "y" ]]; then
    echo "export PATH="~/.bin:\$PATH"" >> ~/.bashrc
    echo -e '\033[32m'"~/.bin has been successfully added to PATH!"'\033[0m'
fi
