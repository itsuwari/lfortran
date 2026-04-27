module c_backend_explicit_shape_leading_dim_01_m
implicit none

real(8), parameter :: static_x(3, 4) = reshape([ &
    11.0d0, 12.0d0, 13.0d0, &
    21.0d0, 22.0d0, 23.0d0, &
    31.0d0, 32.0d0, 33.0d0, &
    41.0d0, 42.0d0, 43.0d0], [3, 4])

contains

subroutine raw_colmajor(a, lda, n, res)
    integer, intent(in) :: lda, n
    real(8), intent(in) :: a(lda, *)
    real(8), intent(out) :: res
    integer :: i, j

    res = 0.0d0
    do j = 1, n
        do i = 1, lda
            res = res + (100*i + j) * a(i, j)
        end do
    end do
end subroutine

subroutine assumed_colmajor(a, res)
    real(8), intent(in) :: a(:, :)
    real(8), intent(out) :: res
    integer :: i, j

    res = 0.0d0
    do j = 1, size(a, 2)
        do i = 1, size(a, 1)
            res = res + (100*i + j) * a(i, j)
        end do
    end do
end subroutine

end module

program c_backend_explicit_shape_leading_dim_01
use c_backend_explicit_shape_leading_dim_01_m, only: raw_colmajor, &
    assumed_colmajor, static_x
implicit none

real(8), allocatable :: x(:, :)
real(8) :: r
integer :: i, j

allocate(x(3, 4))
do j = 1, 4
    do i = 1, 3
        x(i, j) = i + 10*j
    end do
end do

call raw_colmajor(x, size(x, 1), size(x, 2), r)
if (abs(r - 66560.0d0) > 1.0d-9) error stop 1

call assumed_colmajor(static_x, r)
if (abs(r - 66560.0d0) > 1.0d-9) error stop 2

end program
