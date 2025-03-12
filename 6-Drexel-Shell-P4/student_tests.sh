#!/usr/bin/env bats

start_server() {
    ./dsh -s 127.0.0.1 -p 1234 &
    SERVER_PID=$!
    sleep 1 
}

stop_server() {
    if ps -p $SERVER_PID > /dev/null; then
        echo "Stopping server gracefully..."
        kill $SERVER_PID
        wait $SERVER_PID 2>/dev/null || true
    else
        echo "Server already stopped."
    fi
}


setup() {
    start_server
}

teardown() {
    stop_server
}

@test "Client connects to server and runs 'ls'" {
    run ./dsh -c 127.0.0.1 -p 1234 <<EOF
ls
EOF

    echo "Output: $output"
    echo "Exit Status: $status"

    [[ "$output" == *"dsh_cli.c"* ]]
    [[ "$output" == *"dshlib.c"* ]]
    [ "$status" -eq 0 ]
}

@test "Client runs 'ls | grep .c' and filters output" {
    run ./dsh -c 127.0.0.1 -p 1234 <<EOF
ls | grep .c
EOF

    echo "Output: $output"
    echo "Exit Status: $status"

    [[ "$output" == *"dsh_cli.c"* ]]
    [[ "$output" == *"dshlib.c"* ]]
    [[ "$output" != *"dshlib.h"* ]] 
    [ "$status" -eq 0 ]
}

@test "Client runs invalid command and handles error" {
    run ./dsh -c 127.0.0.1 -p 1234 <<EOF
invalid_command
EOF

    echo "Output: $output"
    echo "Exit Status: $status"

    [[ "$output" == *"execvp failed: No such file or directory"* ]]
    [ "$status" -eq 0 ]
}

@test "Client runs 'exit' command and disconnects" {
    run ./dsh -c 127.0.0.1 -p 1234 <<EOF
exit
EOF

    echo "Output: $output"
    echo "Exit Status: $status"

    [[ "$output" == *"socket client mode:  addr:127.0.0.1:1234"* ]] &&
    [[ "$output" == *"dsh4> cmd loop returned 0"* ]] &&
    [[ "$output" == *"Exit Status: 0"* ]] &&
    [[ "$output" == *"Stopping server gracefully..."* ]]
    [ "$status" -eq 0 ]
}

@test "Client runs 'stop-server' command and shuts down server" {
    run ./dsh -c 127.0.0.1 -p 1234 <<EOF
stop-server
EOF

    echo "Output: $output"
    echo "Exit Status: $status"

    [[ "$output" == *"socket client mode:  addr:127.0.0.1:1234"* ]] &&
    [[ "$output" == *"dsh4> cmd loop returned 0"* ]] &&
    [[ "$output" == *"Exit Status: 0"* ]] &&
    [[ "$output" == *"Stopping server"* ]] 
    [ "$status" -eq 0 ]
}

@test "Client runs 'cd' command and changes directory" {
    run ./dsh -c 127.0.0.1 -p 1234 <<EOF
cd /tmp
pwd
EOF

    echo "Output: $output"
    echo "Exit Status: $status"

    [[ "$output" == *"/tmp"* ]] 
    [ "$status" -eq 0 ]
}
