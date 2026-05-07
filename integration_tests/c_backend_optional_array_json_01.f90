module c_backend_optional_array_json_01_m
implicit none

contains

subroutine check_optional_array(a, total, n, flattened_last)
    real(8), intent(in), optional :: a(:, :)
    real(8), intent(out) :: total
    integer, intent(out) :: n
    real(8), intent(out) :: flattened_last
    real(8), allocatable :: flat(:)

    if (.not. present(a)) error stop

    n = size(a)
    flat = reshape(a, [n])
    total = sum(flat)
    flattened_last = flat(n)
end subroutine

end module

program c_backend_optional_array_json_01
use c_backend_optional_array_json_01_m, only: check_optional_array
implicit none

real(8) :: a(2, 3), total, flattened_last
integer :: i, j, n

do j = 1, 3
    do i = 1, 2
        a(i, j) = dble(10*j + i)
    end do
end do

call check_optional_array(a, total, n, flattened_last)
if (n /= 6) error stop
if (abs(total - 129.0d0) > 1.0d-12) error stop
if (abs(flattened_last - 32.0d0) > 1.0d-12) error stop

end program
