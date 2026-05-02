program array_section_29
    implicit none

    real(8), allocatable :: xyz(:, :)
    real(8), allocatable :: trans(:, :)
    real(8) :: vec(3)

    allocate(xyz(3, 2), trans(3, 4))

    xyz(:, 1) = [10.0d0, 20.0d0, 30.0d0]
    xyz(:, 2) = [1.0d0, 2.0d0, 3.0d0]
    trans(:, 4) = [7.0d0, 11.0d0, 13.0d0]

    call calc(xyz, trans, 2, 4, vec)

    if (abs(vec(1) + 16.0d0) > 1.0d-12) error stop
    if (abs(vec(2) + 29.0d0) > 1.0d-12) error stop
    if (abs(vec(3) + 40.0d0) > 1.0d-12) error stop

contains

    subroutine calc(xyz, trans, iat, idx, vec)
        real(8), intent(in) :: xyz(:, :)
        real(8), intent(in) :: trans(:, :)
        integer, intent(in) :: iat, idx
        real(8), intent(out) :: vec(3)

        vec = xyz(:, iat) - xyz(:, 1) - trans(:, idx)
    end subroutine

end program
