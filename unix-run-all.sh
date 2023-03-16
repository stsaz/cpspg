set -x

./heap-mem
./std-echo
./err
echo hello! >file-echo.log
./file-echo
cat file-echo.log
echo hello! >file-echo.log
./file-echo-trunc
cat file-echo.log
./file-man
./file-props
./dir-list
./pipe
./ps-exec
./ps-exec-out
