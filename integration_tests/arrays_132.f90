program arrays_132
    real :: a(4), b(4), c(4)

    b = [-1.0, -2.0, 3.0, 4.0]
    c = [4.0, 9.0, 16.0, 25.0]
    a(:) = abs(b(:)) + sqrt(c(:))

    print *, a(1), a(2), a(3), a(4)
    if (abs(a(1) - 3.0) > 1.e-6) error stop
    if (abs(a(2) - 5.0) > 1.e-6) error stop
    if (abs(a(3) - 7.0) > 1.e-6) error stop
    if (abs(a(4) - 9.0) > 1.e-6) error stop
end program
