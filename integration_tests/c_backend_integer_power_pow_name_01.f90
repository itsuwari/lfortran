program main
    integer :: i
    double precision :: pow(2), value

    i = 3
    pow = 0.0d0
    value = pow(1) + (-1)**i

    if (value /= -1.0d0) error stop
end program
