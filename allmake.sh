rm CMakeCache.txt
module load anaconda3
module load cmake/3.10.1
module load gcc/7.1.0
module load cuda/9.0
module load cudnn/v7.0-cuda.9.0
source activate otgo12
conda install numpy zeromq pyzmq
conda install -c pytorch pytorch cuda90
source scripts/devmode_set_pythonpath.sh
cmake .
make -j
# no idea what I am doing but this is quite cool when you have bugs.
find . -iname 'flags.make' -exec sed -i "s/std=c++17/std=c++17/g" {} \;
make -j
