set -x

./heap-mem
echo hello | ./std-echo
./err

echo hello >file-echo.log
./file-echo
cat file-echo.log

echo hello >file-echo.log
./file-echo-trunc
cat file-echo.log

./file-man
./file-props
./dir-list
./pipe
./ps-exec
echo hello | ./ps-exec-out

./ps-exec-wait
./ps-info

./signal-interrupt &
kill -INT $!

./signal-cpu-exception 1

./pipe-named server &
sleep .5
./pipe-named
kill $!

rm -f fmap.txt
bash -c 'sleep .5 ; echo | ./file-mapping' &
./file-mapping 'data from instance 2'
sleep 1
kill $!

echo | ./semaphore
./dylib-load
