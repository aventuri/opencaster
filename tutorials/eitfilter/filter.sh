#!/bin/bash

if test -z "$1"
then
    echo "bandwith bit rate is missing"
    exit
fi

if test -z "$2"
then
    echo "present following period is missing"
    exit
fi

echo "bandwidth in bps is $1"
echo "present following period is $2 seconds"


cp null.ts output.ts

ts2sec input.ts 18 > input.sec &

# la7 
dvbstream -f 842000 -o 8192 > input.ts &
eitsecfilter input.sec +29 514 1 +29 514 6 +29 514 7 +29 514 8 +29 514 10 +29 514 41 +29 514 42 +29 514 43 +29 514 44 +29 514 47 +29 514 48 +29 514 23 +29 514 19 +29 514 9 +29 514 14 +29 514 24 +29 514 12 +29 514 40 +29 514 11 &

tscbrmuxer b:$1 output.ts o:10000000 null.ts > loop.ts &
DtPlay loop.ts -r 10000000 &

while [ true ]; do

	rm output.sec
	rm pf.sec
	for file in p.*.sec; do
		echo "$file"
		cat $file >> pf.sec
	done
	for file in f.*.sec; do
		echo "$file"
		cat $file >> pf.sec
	done
	sec2ts 18 < pf.sec > pf.ts
	pf_file_size=`stat --printf="%s"  pf.ts`
	echo "present following ts file size is $pf_file_size"
	bytes_per_period=$(($1*$2/8))
	echo "bytes per period available are $bytes_per_period"
	[ -f sched.sec ] && rm sched.sec
	ls -1 old_sc.*.sec > /dev/null 2>&1 
	if [ "$?" = "0" ]; then
		final_ts_size=0
		check_bytes_per_period=$(($bytes_per_period-$final_ts_size))
		while [ $check_bytes_per_period -gt 0 ]; do
			cp sched.sec prev_sched.sec
			file=`/bin/ls -1 old_sc.*.sec | sort --random-sort | head -1`
			echo $file
			cat $file >> sched.sec
			cat pf.sec sched.sec > output.sec
			sec2ts 18 < output.sec > test_output.ts
			final_ts_size=`stat --printf="%s"  test_output.ts`
			echo "testing bytes final ts are $final_ts_size"
			let check_bytes_per_period=$(($bytes_per_period-$final_ts_size))
		done
	fi
	if [ -f prev_sched.sec ]; then 
		cat pf.sec prev_sched.sec > output.sec
	else
		cat pf.sec > output.sec
	fi
	sec2ts 18 < output.sec > output.ts
	final_ts_size=`stat --printf="%s"  output.ts`
	echo "decided bytes final ts are $final_ts_size"
	sleep $2
done


