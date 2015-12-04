CPP=dmcache.cpp
EXE=dm.out

if g++ $CPP -o $EXE; then
	for f in dmtest*.txt; do
		echo TESTING $f
		if ./$EXE $f && diff -q dm-out.txt ${f/test/-out}; then
			echo OK $f
		else
			echo /_FAIL_/ $f
			echo ------------------FAIL----------------
			exit 1
		fi
	done
	echo ------------------OK----------------
fi
