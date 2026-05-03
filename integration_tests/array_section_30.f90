program array_section_30
    implicit none
    real(8) :: lat(3, 3)
    real(8) :: c(3)

    lat = reshape([ &
        2.0d0, 3.0d0, 5.0d0, &
        7.0d0, 11.0d0, 13.0d0, &
        17.0d0, 19.0d0, 23.0d0], shape(lat))

    call crossproduct(lat(:, 2), lat(:, 3), c)

    if (abs(c(1) - 6.0d0) > 1.0d-12) error stop
    if (abs(c(2) - 60.0d0) > 1.0d-12) error stop
    if (abs(c(3) + 54.0d0) > 1.0d-12) error stop

contains

    subroutine crossproduct(a, b, c)
        real(8), intent(in) :: a(3)
        real(8), intent(in) :: b(3)
        real(8), intent(out) :: c(3)

        c(1) = a(2)*b(3) - b(2)*a(3)
        c(2) = a(3)*b(1) - b(3)*a(1)
        c(3) = a(1)*b(2) - b(1)*a(2)
    end subroutine

end program
