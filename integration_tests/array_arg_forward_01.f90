module array_arg_forward_01_m
    implicit none
contains
    subroutine inner(a, b, c, total)
        real(8), intent(in) :: a(:, :)
        real(8), intent(in) :: b(:)
        real(8), intent(inout), optional :: c(:, :)
        real(8), intent(inout) :: total

        total = total + a(2, 1) + b(3)
        if (present(c)) then
            c(1, 1) = c(1, 1) + total
        end if
    end subroutine

    subroutine outer(a, b, c, total)
        real(8), intent(in) :: a(:, :)
        real(8), intent(in) :: b(:)
        real(8), intent(inout), optional :: c(:, :)
        real(8), intent(inout) :: total

        call inner(a, b, c, total)
    end subroutine
end module

program array_arg_forward_01
    use array_arg_forward_01_m, only: outer
    implicit none

    real(8) :: a(3, 2), b(4), c(2, 2), d(4, 2), total

    a = reshape([1.0d0, 2.0d0, 3.0d0, 4.0d0, 5.0d0, 6.0d0], [3, 2])
    b = [10.0d0, 20.0d0, 30.0d0, 40.0d0]
    c = 1.0d0
    total = 0.0d0

    call outer(a, b, c, total)
    if (abs(total - 32.0d0) > 1.0d-12) error stop 1
    if (abs(c(1, 1) - 33.0d0) > 1.0d-12) error stop 2

    call outer(a, b, total=total)
    if (abs(total - 64.0d0) > 1.0d-12) error stop 3

    call outer(a(1:3:2, :), b(1:4:2), total=total)
    if (abs(total - 97.0d0) > 1.0d-12) error stop 4

    d = 1.0d0
    call outer(a, b, d(1:3:2, :), total)
    if (abs(total - 129.0d0) > 1.0d-12) error stop 5
    if (abs(d(1, 1) - 130.0d0) > 1.0d-12) error stop 6
    if (abs(d(2, 1) - 1.0d0) > 1.0d-12) error stop 7
    if (abs(d(3, 1) - 1.0d0) > 1.0d-12) error stop 8
end program
