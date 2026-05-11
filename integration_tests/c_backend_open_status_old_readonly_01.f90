program c_backend_open_status_old_readonly_01
    implicit none

    integer :: unit, stat, value
    character(len=64) :: filename

    filename = "c_backend_open_status_old_readonly_01.tmp"

    call execute_command_line("chmod 644 " // trim(filename) // " 2>/dev/null; rm -f " // trim(filename), exitstat=stat)

    open(newunit=unit, file=trim(filename), status="replace", action="write", iostat=stat)
    if (stat /= 0) error stop 1
    write(unit, *) 42
    close(unit)

    call execute_command_line("chmod 444 " // trim(filename), exitstat=stat)
    if (stat /= 0) error stop 2

    open(newunit=unit, file=trim(filename), status="old", iostat=stat)
    if (stat /= 0) error stop 3
    read(unit, *, iostat=stat) value
    if (stat /= 0) error stop 4
    if (value /= 42) error stop 5
    close(unit)

    call execute_command_line("chmod 644 " // trim(filename) // " 2>/dev/null; rm -f " // trim(filename), exitstat=stat)
end program c_backend_open_status_old_readonly_01
