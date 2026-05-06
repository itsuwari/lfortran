module c_backend_assumed_shape_section_explicit_01_m
implicit none

real(8) :: last_sum = 0.0d0

contains

subroutine raw_weighted_sum(a, lda, n)
    integer, intent(in) :: lda, n
    real(8), intent(in) :: a(lda, *)
    integer :: i, j

    last_sum = 0.0d0
    do j = 1, n
        do i = 1, lda
            last_sum = last_sum + dble(100*i + j) * a(i, j)
        end do
    end do
end subroutine

subroutine wrapper(a)
    real(8), intent(in) :: a(:, :)

    call raw_weighted_sum(a, size(a, 1), size(a, 2))
end subroutine

end module

program c_backend_assumed_shape_section_explicit_01
use c_backend_assumed_shape_section_explicit_01_m, only: last_sum, wrapper
implicit none

real(8), allocatable :: x(:, :)
integer :: i, j

allocate(x(4, 4))
do j = 1, 4
    do i = 1, 4
        x(i, j) = dble(i + 10*j)
    end do
end do

call wrapper(x(1:3, 1:3))
if (abs(last_sum - 40656.0d0) > 1.0d-10) error stop

end program
