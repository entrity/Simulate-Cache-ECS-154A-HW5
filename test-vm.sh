CPP=vm.cpp
EXE=vm.out

if g++ $CPP -o $EXE; then
	for f in vmtest*.txt; do
		echo TESTING $f
		if ./$EXE $f && diff -q vm-out.txt ${f/test/-out}; then
			echo OK $f
		else
			echo /_FAIL_/ $f
			echo ------------------FAIL----------------
			exit 1
		fi
	done
	echo ------------------OK----------------
fi
