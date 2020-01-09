FILENUM=20

echo plot frequency
for d in chinese english binary;do
    for f in 20;do
        gnuplot -e "figtitle='frequency of $(echo $f | sed 's/^0*//')0MB ${d} file';infile='../performance/${d}_${f}_frequency.txt';outfile='../performance/freq_${d}_${f}.png'" frequency.gp
    done
done

echo plot different thread
for method in compress decompress;do
    for size in $(seq -f '%02g' 1 $FILENUM);do
        for d in chinese english binary;do
            #    for size in 20;do
            echo -e "\n${method} ${d} $(echo $size | sed 's/^0*//')0 MB"
            # single thread
            cat ../performance/serial_${method}_${d}.txt | sed "$(echo $size | sed 's/^0*//')!d" | awk -v threads=1 '{print threads " " $2}' > ${d}_data.txt
            for t in 4 8 16 32 64;do
                cat ../performance/parallel${t}_${method}_${d}.txt | sed "$(echo $size | sed 's/^0*//')!d" | awk -v threads=$t '{print threads " " $2}' >> ${d}_data.txt
                #echo "{printf \"$t %d\n\", \$2}"
            done
            cat ${d}_data.txt
            echo ''
        done
        gnuplot -e "figtitle='${method} with different #thread on $(echo $size | sed 's/^0*//')0MB file';zhfile='chinese_data.txt';enfile='english_data.txt';binfile='binary_data.txt';outfile='../performance/thread_${method}_${size}0mb.png'" threads.gp
        rm chinese_data.txt english_data.txt binary_data.txt
    done
done

echo plot serial with different size
for method in compress decompress;do
    for size in $(seq -f '%02g' 1 $FILENUM);do
        for d in chinese english binary;do
            gnuplot -e "figtitle='${method} on different size of ${d} files';
            t1='../performance/serial_${method}_${d}.txt';
            t4='../performance/parallel4_${method}_${d}.txt';
            t8='../performance/parallel8_${method}_${d}.txt';
            t16='../performance/parallel16_${method}_${d}.txt';
            t32='../performance/parallel32_${method}_${d}.txt';
            t64='../performance/parallel64_${method}_${d}.txt';
            outfile='../performance/size_${method}_${d}.png'" size.gp
        done
    done
done

echo plot comparison between serial and parallel
for size in $(seq -f '%02g' 1 $FILENUM);do
    for d in chinese english binary;do
        rm data.txt
        for method in compress decompress;do
            echo -e "\n${method} ${d} $(echo $size | sed 's/^0*//')0 MB"
            # single thread
            s=`cat ../performance/serial_${method}_${d}.txt | sed "$(echo $size | sed 's/^0*//')!d" | awk '{print $2}'`
            t4=`cat ../performance/parallel4_${method}_${d}.txt | sed "$(echo $size | sed 's/^0*//')!d" | awk '{print $2}'`
            t8=`cat ../performance/parallel8_${method}_${d}.txt | sed "$(echo $size | sed 's/^0*//')!d" | awk '{print $2}'`
            t16=`cat ../performance/parallel16_${method}_${d}.txt | sed "$(echo $size | sed 's/^0*//')!d" | awk '{print $2}'`
            t32=`cat ../performance/parallel32_${method}_${d}.txt | sed "$(echo $size | sed 's/^0*//')!d" | awk '{print $2}'`
            t64=`cat ../performance/parallel64_${method}_${d}.txt | sed "$(echo $size | sed 's/^0*//')!d" | awk '{print $2}'`
            echo "${method}() $s $t4 $t8 $t16 $t32 $t64" >> data.txt
        done
        cat data.txt
        gnuplot -e "figtitle='compare serial and parallel on $(echo $size | sed 's/^0*//')0MB file';infile='data.txt';outfile='../performance/cmp_${d}_${size}0mb.png'" cmp-serial-parallel.gp
    done
done
