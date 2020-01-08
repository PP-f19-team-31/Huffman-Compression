TARGET=ubuntu-16.04.6-desktop-amd64.iso
CHUNCK=10
FILENUM=20

if [ ! -f $TARGET ];then
    wget http://ftp.ubuntu-tw.org/mirror/ubuntu-releases/16.04/ubuntu-16.04.6-desktop-amd64.iso
fi

split --numeric-suffixes -b 10M $TARGET tmp

for f in $(seq -f '%02g' 1 $FILENUM);do
    for c in $(seq -f '%02g' 1 $f);do
        cat tmp${c} >> d${f}.txt
    done
done

rm tmp* $TARGET
