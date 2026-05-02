module array_expr_scalarize_01_m
implicit none

type :: holder
    real(8), allocatable :: v(:)
end type

contains

subroutine fused_update(out, x, y)
    real(8), intent(inout) :: out(:)
    real(8), intent(in) :: x(:), y(:)

    out(:) = 2.0d0*x(:) - y(:)
end subroutine

end module

program array_expr_scalarize_01
use array_expr_scalarize_01_m, only: fused_update, holder
implicit none

type(holder) :: h
real(8) :: a(4, 3), b(4, 3), c(4, 3)
real(8) :: expected(4)
integer :: i, j

do j = 1, 3
    do i = 1, 4
        a(i, j) = 10.0d0*i + j
        b(i, j) = 100.0d0*i + 2*j
        c(i, j) = 1000.0d0*i - j
    end do
end do

expected(:) = b(:, 2) + 3.0d0*c(:, 2)
a(:, 2) = b(:, 2) + 3.0d0*c(:, 2)
do i = 1, 4
    if (abs(a(i, 2) - expected(i)) > 1.0d-12) error stop
end do

expected(:) = 2.0d0*b(:, 3) - c(:, 3)
call fused_update(a(:, 3), b(:, 3), c(:, 3))
do i = 1, 4
    if (abs(a(i, 3) - expected(i)) > 1.0d-12) error stop
end do

h%v = b(:, 1)
expected(:) = h%v + c(:, 1)
a(:, 1) = h%v + c(:, 1)
do i = 1, 4
    if (abs(a(i, 1) - expected(i)) > 1.0d-12) error stop
end do

end program
