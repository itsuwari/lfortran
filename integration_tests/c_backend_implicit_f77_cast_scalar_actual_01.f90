subroutine takes_real(x, y)
    double precision :: x, y
    y = x
end subroutine

program main
    integer :: m
    double precision :: y
    external takes_real

    m = 7
    call takes_real(dble(m), y)
    if (y /= 7.0d0) error stop
end program
