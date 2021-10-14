apt-get install g++-multilib -y
apt-get install nasm -y
apt-get install -y linux-headers-`uname -r`

apt-get install python3-pip -y
#pip3 install --upgrade pip
pip3 install --upgrade 'pip<21' 'setuptools<51'
pip3 install seaborn

apt-get install -y linux-tools-$(uname -r)