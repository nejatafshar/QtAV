
D=`readlink -f $0`
D=${D%/*}
export LD_LIBRARY_PATH=$D:$D/lib
$D/Player "$@" # name with spaces
