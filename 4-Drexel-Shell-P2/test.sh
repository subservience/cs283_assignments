#!/usr/bin/env bats

@test "Shell starts" {
    run ./dsh <<EOF
exit
EOF
    
    [[ "$output" == *"dsh2>"* ]]
    echo "Captured stdout: $output"
    echo "Exit Status: $status"
    [ "$status" -eq 0 ]
}

@test "Built-in command: cd" {
    mkdir -p testdir
    run ./dsh <<EOF
cd testdir
pwd
exit
EOF
    
    [[ "$output" == *"/home/tammyhuynh/cs283_assignments/4-Drexel-Shell-P2/testdir"* ]]
    echo "Captured stdout: $output"
    echo "Exit Status: $status"
    [ "$status" -eq 0 ]
    rmdir testdir
}

@test "Built-in command: exit" {
    run ./dsh <<EOF
exit
EOF
    
    [[ "$output" == *"dsh2>"* ]]
    echo "Captured stdout: $output"
    echo "Exit Status: $status"
    [ "$status" -eq 0 ]
}

@test "External command: ls" {
    touch testfile
    run ./dsh <<EOF
ls
echo done
exit
EOF
    
    [[ "$output" == *"testfile"* ]]
    [[ "$output" == *"done"* ]]
    echo "Captured stdout: $output"
    echo "Exit Status: $status"
    [ "$status" -eq 0 ]
    rm testfile
}

@test "External command: echo" {
    run ./dsh <<EOF
echo hello world
exit
EOF
    
    [[ "$output" == *"hello world"* ]]
    echo "Captured stdout: $output"
    echo "Exit Status: $status"
    [ "$status" -eq 0 ]
}

@test "Invalid command" {
    run ./dsh <<EOF
invalidcommand
exit
EOF
    
    [[ "$output" == *"execvp failed: No such file or directory"* ]]
    echo "Captured stdout: $output"
    echo "Exit Status: $status"
    [ "$status" -ne 0 ]
}
