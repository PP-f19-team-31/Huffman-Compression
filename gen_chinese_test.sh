download_from_gdrive() {
    file_id=$1
    file_name=$2 # first stage to get the warning html
    if [ ! -f $file_name ];then
        curl -L -o $file_name -c /tmp/cookies \
        "https://drive.google.com/uc?export=download&id=$file_id"
        if grep "Virus scan warning" $file_name > /dev/null;then
            # second stage to extract the download link from html above
            download_link=$(cat $file_name | \
            grep -Po 'uc-download-link" [^>]* href="\K[^"]*' | \
            sed 's/\&amp;/\&/g')
            curl -L -b /tmp/cookies \
            "https://drive.google.com$download_link" > $file_name
        fi
    fi
}

TARGET=token.txt
CHUNCK=10
FILENUM=20

if [ ! -f $TARGET ];then
    download_from_gdrive 1VwjLCVfByaO5ywBFg_nmjp9RlJBRevCk $TARGET
fi

split --numeric-suffixes -b 10M $TARGET tmp

for f in $(seq -f '%02g' 1 $FILENUM);do
    for c in $(seq -f '%02g' 1 $f);do
        cat tmp${c} >> d${f}.txt
    done
done

rm tmp* $TARGET
