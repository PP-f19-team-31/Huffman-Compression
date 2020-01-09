FILENUM=20

for d in chinese english binary;do
    if [ ! -d $d ];then
        mkdir $d && cd $d || exit 1
        bash ../gen_${d}_test.sh && cd .. || exit 1
    fi
done

mkdir -p performance
rm performance/*

# /usr/bin/time -f "%E real,%U user,%S sys" sleep 2
#make clean
#make serial || exit 1
#echo serial beg
#for d in chinese english binary;do
#    for f in $(seq -f '%02g' 1 $FILENUM);do
#        ./serial -f < $d/d${f}.txt > performance/${d}_${f}_frequency.txt
#        echo "compressing... $d/d${f}.txt"
#        /usr/bin/time -f "${f} %E" ./serial < $d/d${f}.txt > compressed.txt         2>>performance/serial_compress_${d}.txt
#        echo "decompressing... $d/d${f}.txt"
#        /usr/bin/time -f "${f} %E" ./serial -d < compressed.txt > decompressed.txt  2>>performance/serial_decompress_${d}.txt
#        echo 
#    done
#done
#echo serial end

echo parallel beg
for t in 4 8 16 32 64;do
    for d in chinese english binary;do
        for f in $(seq -f '%02g' 1 $FILENUM);do
            make clean
            make all THREAD_NUMBER=$t || exit 1
            echo "$t threads, compressing... $d/d${f}.txt"
            /usr/bin/time -f "${f} %E" ./huffman -c $d/d${f}.txt   -o compressed.txt    2>>performance/parallel${t}_compress_${d}.txt
            echo "$t threads, decompressing... $d/d${f}.txt"
            /usr/bin/time -f "${f} %E" ./huffman -d compressed.txt -o decompressed.txt  2>>performance/parallel${t}_decompress_${d}.txt
        done
    done
done
echo parallel end

rm compressed.txt decompressed.txt
