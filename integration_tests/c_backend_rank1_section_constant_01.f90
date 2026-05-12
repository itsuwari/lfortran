program c_backend_rank1_section_constant_01
    implicit none
    integer :: a(6)

    a = 0
    a(2:4) = [10, 20, 30]

    if (any(a /= [0, 10, 20, 30, 0, 0])) error stop
end program
