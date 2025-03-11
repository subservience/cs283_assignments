#!/usr/bin/env bats

# Helper function to start the server in the background
start_server() {
    ./dsh -s -i 127.0.0.1 -p 1234 &
    SERVER_PID=$!
    sleep 1 # Give the server time to start
}

# Helper function to stop the server
stop_server() {
    if ps -p $SERVER_PID > /dev/null; then
        echo "Stopping server gracefully..."
        kill $SERVER_PID
        wait $SERVER_PID 2>/dev/null
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
    run ./dsh -c -i 127.0.0.1 -p 1234 <<EOF
ls
EOF

    # Check that the output contains expected files (e.g., dsh_cli.c, dshlib.c)
    echo "Output: $output"
    echo "Exit Status: $status"

    [[ "$output" == *"dsh_cli.c"* ]]
    [[ "$output" == *"dshlib.c"* ]]
    [ "$status" -eq 0 ]
}

@test "Client runs 'ls | grep .c' and filters output" {
    run ./dsh -c -i 127.0.0.1 -p 1234 <<EOF
ls | grep .c
EOF

    # Check that the output contains only .c files
    echo "Output: $output"
    echo "Exit Status: $status"

    [[ "$output" == *"dsh_cli.c"* ]]
    [[ "$output" == *"dshlib.c"* ]]
    [[ "$output" != *"dshlib.h"* ]] # Ensure .h files are not included
    [ "$status" -eq 0 ]
}

@test "Client runs invalid command and handles error" {
    run ./dsh -c -i 127.0.0.1 -p 1234 <<EOF
invalid_command
EOF

    # Check that the output contains an error message
    echo "Output: $output"
    echo "Exit Status: $status"

    [[ "$output" == *"error"* ]] # Check for error message
    [ "$status" -eq 0 ]
}

@test "Client runs 'exit' command and disconnects" {
    run ./dsh -c -i 127.0.0.1 -p 1234 <<EOF
exit
EOF

    # Check that the client exits gracefully
    echo "Output: $output"
    echo "Exit Status: $status"

    [[ "$output" == *"Exiting client"* ]] # Check for exit message
    [ "$status" -eq 0 ]
}

@test "Client runs 'stop-server' command and shuts down server" {
    run ./dsh -c -i 127.0.0.1 -p 1234 <<EOF
stop-server
EOF

    # Check that the server shuts down
    echo "Output: $output"
    echo "Exit Status: $status"

    [[ "$output" == *"Stopping server"* ]] # Check for server shutdown message
    [ "$status" -eq 0 ]
}

@test "Client runs 'cd' command and changes directory" {
    run ./dsh -c -i 127.0.0.1 -p 1234 <<EOF
cd /tmp
pwd
EOF

    # Check that the directory is changed to /tmp
    echo "Output: $output"
    echo "Exit Status: $status"

    [[ "$output" == *"/tmp"* ]] # Check for /tmp in the output
    [ "$status" -eq 0 ]
}
