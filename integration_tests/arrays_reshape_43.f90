program arrays_reshape_43
    implicit none

    real, parameter :: p(2, 3) = reshape([1.0, 2.0, 3.0, 4.0, 5.0, 6.0], [2, 3])
    real, parameter :: pp(2, 6) = reshape([p, p], shape(pp))
    real :: a(2, 3)
    real :: b(2, 3, 1)
    real :: c(6, 1)

    a(1, 1) = 1.0
    a(2, 1) = 2.0
    a(1, 2) = 3.0
    a(2, 2) = 4.0
    a(1, 3) = 5.0
    a(2, 3) = 6.0
    call reshape_assumed_shape(a, b, c)

    if (any(shape(b) /= [2, 3, 1])) error stop 1
    if (any(shape(c) /= [6, 1])) error stop 2
    if (b(1, 1, 1) /= 1.0) error stop 3
    if (b(2, 1, 1) /= 2.0) error stop 4
    if (b(1, 2, 1) /= 3.0) error stop 5
    if (b(2, 2, 1) /= 4.0) error stop 6
    if (b(1, 3, 1) /= 5.0) error stop 7
    if (b(2, 3, 1) /= 6.0) error stop 8
    if (c(1, 1) /= 1.0) error stop 9
    if (c(2, 1) /= 2.0) error stop 10
    if (c(3, 1) /= 3.0) error stop 11
    if (c(4, 1) /= 4.0) error stop 12
    if (c(5, 1) /= 5.0) error stop 13
    if (c(6, 1) /= 6.0) error stop 14
    if (pp(1, 1) /= 1.0) error stop 15
    if (pp(2, 6) /= 6.0) error stop 16

contains

    subroutine reshape_assumed_shape(x, y, z)
        real, intent(in) :: x(:, :)
        real, intent(out) :: y(:, :, :)
        real, intent(out) :: z(:, :)

        y = reshape(x, [shape(x), 1])
        z = reshape(x, [size(x), 1])
    end subroutine
end program
