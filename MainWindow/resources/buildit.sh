if [ "$2" == "" ]; then
else
    echo mtimake -d -w -j 24 --directory=$1 $2
    cd $1
    mtimake -w -j 24 --directory="$1" $2
fi

