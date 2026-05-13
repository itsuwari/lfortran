program c_backend_formatted_fortran_stack_write_01
    implicit none

    character(96) :: line
    character(4104) :: long_line
    integer :: i
    real :: x
    logical :: ok

    i = 42
    x = 3.25
    ok = .true.

    write(line, "(i4,1x,f7.2,1x,l2)") i, x, ok

    if (index(line, "42") == 0) stop 1
    if (index(line, "3.25") == 0) stop 2
    if (index(line, "T") == 0) stop 3

    write(long_line, "(i4100)") 7
    if (long_line(4100:4100) /= "7") stop 4
end program
