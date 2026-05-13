program c_backend_formatted_stack_write_01
    implicit none

    character(8) :: left
    character(8) :: right
    character(64) :: line

    left = "alpha"
    right = "beta"

    write(line, "(a,1x,a,1x,a)") left, "mid", right

    if (index(line, "alpha") /= 1) stop 1
    if (index(line, "mid") == 0) stop 2
    if (index(line, "beta") == 0) stop 3
end program
