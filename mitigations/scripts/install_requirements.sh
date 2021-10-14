if [[ $(lsb_release -rs) == "20.04" ]]; then 

       echo "Compatible Ubuntu version"
else
       echo "We Recommend Ubuntu version 20.04"
fi

sudo apt install build-essential git m4 scons zlib1g zlib1g-dev \
    libprotobuf-dev protobuf-compiler libprotoc-dev libgoogle-perftools-dev \
    python3-dev python3-six python-is-python3 libboost-all-dev pkg-config
