
D=`readlink -f $0`
D=${D%/*}
export LD_LIBRARY_PATH=$D:$D/lib
$D/player "$@" # name with spaces
