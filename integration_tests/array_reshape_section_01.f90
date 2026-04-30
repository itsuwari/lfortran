program array_reshape_section_01
    implicit none
    real :: source(2, 3)
    real :: target(8)
    integer :: i

    source = reshape([1.0, 2.0, 3.0, 4.0, 5.0, 6.0], shape(source))
    target = 0.0
    target(:6) = reshape(source, [6])

    do i = 1, 6
        if (abs(target(i) - real(i)) > 1.e-6) error stop
    end do
    if (abs(target(7)) > 1.e-6) error stop
    if (abs(target(8)) > 1.e-6) error stop
end program
