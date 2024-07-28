heap-mem
echo hello | std-echo
err

echo hello >file-echo.log
file-echo
type file-echo.log

echo hello >file-echo.log
file-echo-trunc
type file-echo.log

file-man
file-props
dir-list
pipe
ps-exec
echo hello | ps-exec-out

ps-exec-wait
ps-info
rem signal-interrupt
signal-cpu-exception 1
rem pipe-named
rem file-mapping
echo | semaphore
dylib-load
