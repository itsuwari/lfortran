program c_backend_rank1_section_constant_01
    implicit none
    integer :: a(6), b(6), c(6)

    a = 0
    a(2:4) = [10, 20, 30]

    if (any(a /= [0, 10, 20, 30, 0, 0])) error stop

    b = 0
    b(1:6:2) = [10, 20, 30]

    if (any(b /= [10, 0, 20, 0, 30, 0])) error stop

    c = 0
    c(6:1:-2) = [10, 20, 30]

    if (any(c /= [0, 30, 0, 20, 0, 10])) error stop
end program
