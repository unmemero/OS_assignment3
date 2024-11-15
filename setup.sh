#!/bin/bash

# Set git configurations
git_config="n"

# Check which package manager is installed
package_manager_check() {
    echo -e "\033[34mChecking for package manager...\033[0m"
    if command -v brew &> /dev/null; then
        PACKAGE_MANAGER="brew"
    elif command -v pacman &> /dev/null; then
        PACKAGE_MANAGER="pacman"
    elif command -v apt-get &> /dev/null; then
        PACKAGE_MANAGER="apt"
    elif command -v dnf &> /dev/null; then
        PACKAGE_MANAGER="dnf"
    else
        echo -e "\033[34mMissing required package manager. Installing Homebrew\033[0m"
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
        if [ $? -eq 0 ]; then
            echo -e "\033[32mHomebrew installed successfully\033[0m"
        else
            echo -e '\033[31mFailed to install Homebrew.\nInstall using command /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)" to debug properly\n and rerun this script again\033[0m'
            exit 1
        fi
    fi
}

# Create folder
make_folder() {
    if [ "$(basename "$PWD")" == "assignment3" ]; then
        echo -e "\033[32mYou are in the assignment3 directory\033[0m\n"
    else
        if [ -d "./assignment3" ]; then
            echo -e "\033[34mThe assignment3 directory already exists in the current working directory. \nMoving into it...\033[0m"
            cd ./assignment3
        else
            echo -e "\033[34mCreating the assignment3 directory\033[0m"
            mkdir ./assignment3
            cd ./assignment3
            echo -e "\033[32mDirectory created successfully\033[0m"
        fi
    fi
}

# Install packages
install_packages() {
    package_name=$1
    if [ "$PACKAGE_MANAGER" == "brew" ]; then
        brew install $package_name
        brew update -y
        brew upgrade -y
        brew cleanup -y
    elif [ "$PACKAGE_MANAGER" == "apt" ]; then
        sudo apt install $package_name -y
        sudo apt update -y
        sudo apt upgrade -y
        sudo apt autoremove -y
        sudo apt autoclean -y
    elif [ "$PACKAGE_MANAGER" == "dnf" ]; then
        sudo dnf install $package_name -y
        sudo dnf update -y
        sudo dnf autoremove -y
        sudo dnf clean all -y
    elif [ "$PACKAGE_MANAGER" == "pacman" ]; then
        sudo pacman -S $package_name --noconfirm
        sudo pacman -Syu --noconfirm
    else
        echo -e "\033[31mFailed to install $package_name\033[0m\n"
        exit 1
    fi
}

# Create git repository
create_git_repo() {
    read -r -p $'\033[34mWant to configure git repository? \033[32m(Default y) \033[34m(\033[32my\033[34m/\033[31mn\033[34m): \033[0m' git_config
    echo -e "\n"
    git_ignore="n"

    if [ "$git_config" != "n" ]; then
        if git --version &> /dev/null; then
            read -r -p $'\033[34mEnter your git username (Can be your full name): \033[0m' git_username
            read -r -p $'\033[34mEnter your git email: \033[0m' git_email
            read -r -p $'\033[34mWant to add a .gitignore file? \033[32m(Default y) \033[34m(\033[32my\033[34m/\033[31mn\033[34m): \033[0m' git_ignore
            git config --global user.name "$git_username"
            git config --global user.email "$git_email"
            git init
            read -r -p $'\033[34mWant to set repo to the remote repository? \033[32m(Default y) \033[34m(\033[32my\033[34m/\033[31mn\033[34m): \033[0m' git_remote
            if [ "$git_remote" == "y" ]; then
                read -r -p $'\033[34mEnter the remote repository URL: \033[0m' remote_url
                git remote add origin "$remote_url"
                if [ $? -eq 0 ]; then
                    echo -e "\033[32mRemote repository added successfully\033[0m\n"
                else
                    echo -e "\033[31mFailed to add remote repository\033[0m\n"
                fi
            else
                echo -e "\033[34mSkipping remote repository configuration...\033[0m\n"
            fi
        else
            echo -e "\033[31mGit is not installed. \nInstalling git\033[0m\n"
            install_packages git
        fi
    else
        echo -e "\033[34mSkipping git configuration...\033[0m\n"
    fi
}

# Fetch instructions and boilerplate from website
get_required_files() {
    if ! command -v wget &> /dev/null; then
        echo -e "\033[31mWget is not installed. \nInstalling wget\033[0m\n"
        install_packages wget
    else
        echo -e "\033[32mWget is installed\nDownloading files...\033[0m\n"
        wget -O hw2.pdf https://www.christoph-lauter.org/utep-os/hw3.pdf
        wget -O myfs.c https://www.christoph-lauter.org/utep-os/myfs.c
        wget -O implementation.c https://www.christoph-lauter.org/utep-os/implementation.c
    fi
}

# Create makefile
create_makefile() {
    echo -e "\033[34mCreating makefile...\033[0m"
    touch Makefile
    echo -e "all: \n\tgcc -g -O0 -Wall myfs.c implementation.c \`pkg-config fuse --cflags --libs\` -o myfs\nrun:\n\tgdb --args ./myfs --backupfile=test.myfs ~/fuse-mnt/ -f\n\nunmount:\n\tfusermount -u ~/fuse-mnt\nclean:\n\trm -rf myfs Report.pdf\npush:\n\t@read -p \"Enter commit message: \" msg; \\\n\tgit status; \\\n\tgit add -A; \\\n\techo \"\033[33mStaging changes...\033[0m\"; \\\n\tgit status; \\\n\tif git diff --cached --exit-code > /dev/null; then \\\n\techo \"No changes to commit.\"; \\\n\telse \\\n\tgit commit -m \"\$\$msg\"; \\\n\techo -e \"\033[34mPulling latest changes with rebase...\033[0m\"; \\\n\tgit pull --rebase origin main; \\\n\tgit push origin main; \\\n\tfi\npdf:\n\tpandoc README.md -o Report.pdf\ntar:\n\ttar -czvf assignment3.tgz .\n" > Makefile
}

# Install dependencies
install_dependencies() {
    echo -e "\033[34mInstalling dependencies...\033[0m"
    if ! command -v pkg-config &> /dev/null; then
        echo -e "\033[31mpkg-config is not installed. \nInstalling pkg-config\033[0m\n"
        install_packages pkg-config
    fi
    if ! command -v fuse &> /dev/null; then
        echo -e "\033[31mFUSE is not installed. \nInstalling FUSE\033[0m\n"
        install_packages fuse
    fi
    if ! command -v pandoc &> /dev/null; then
        echo -e "\033[31mPandoc is not installed. \nInstalling Pandoc\033[0m\n"
        install_packages pandoc
    fi
    if ! command -v gcc &> /dev/null; then
        echo -e "\033[31mGCC is not installed. \nInstalling GCC\033[0m\n"
        install_packages gcc
    fi
    if ! command -v gdb &> /dev/null; then
        echo -e "\033[31mGDB is not installed. \nInstalling GDB\033[0m\n"
        install_packages gdb
    fi
    if ! command -v fusermount &> /dev/null; then
        echo -e "\033[31mFusermount is not installed. \nInstalling Fusermount\033[0m\n"
        install_packages fusermount
    fi
    if ! command -v tar &> /dev/null; then
        echo -e "\033[31mTar is not installed. \nInstalling Tar\033[0m\n"
        install_packages tar
    fi
}

# Generate instructions in terminal
instructions() {
    echo -e "\033[1;94m>>>>>>>>>>>>>>>INSTRUCTIONS<<<<<<<<<<<<<<<<\033[0m\n"
    echo -e "\033[1;91mWARNING: Do not modify myfs.c\033[0m\nOnly modify implementation.c\n"
    echo -e "\033[1;93m1. To compile:\033[0m make all"
    echo -e "\033[1;93m2. To run:\033[0m make run"
    echo -e "\033[1;93m3. To unmount filesystem (in another terminal):\033[0m make unmount"
    if [ "$git_config" != "n" ]; then
        echo -e "\033[1;93m5. To push:\033[0m make push"
    fi
    echo -e "\033[1;93m7. To compress project into tgz:\033[0m make tar"
    echo -e "\033[1;94mFor any questions, reach out to me\033[0m\n"
    echo -e "\033[1;94m>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<<<<\033[0m\n"
}

# Prompt user if they want to set up or jump to instructions
read -r -p $'\033[34mJump to instructions? If this is your first time type n \033[32m(Default y) \033[34m(\033[32my\033[34m/\033[31mn\033[34m): \033[0m' jump
if [ "$jump" != "n" ]; then
    echo -e "\033[34mJumping to instructions...\033[0m\n"
    instructions
    exit 0
fi

# Detect Package Manager
package_manager_check

# Create folder
make_folder

# Config git repository
create_git_repo

# Fetch instructions and boilerplate from website
get_required_files

# Create makefile
create_makefile

# Create README.md for report
echo -e "\033[34mCreating README.md...\033[0m"
touch README.md
echo -e "# Assignment 3\n**Authors:**\n" >> README.md

# Install dependencies
install_dependencies

# Generate instructions
echo -e "\033[32mSetup completed successfully\033[0m\n"
instructions
