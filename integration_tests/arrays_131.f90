program arrays_131
    real :: a(4), b(4), c(4)

    b = [1.0, 2.0, 3.0, 4.0]
    c = [10.0, 20.0, 30.0, 40.0]
    a(:) = b(:) + c(:)

    print *, a(1), a(2), a(3), a(4)
    if (abs(a(1) - 11.0) > 1.e-6) error stop
    if (abs(a(2) - 22.0) > 1.e-6) error stop
    if (abs(a(3) - 33.0) > 1.e-6) error stop
    if (abs(a(4) - 44.0) > 1.e-6) error stop
end program
