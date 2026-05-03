module c_backend_const_vector_subscript_literal_01_m
implicit none

contains

subroutine kernel(cart, sphr)
real(8), intent(in) :: cart(:, :)
real(8), intent(out) :: sphr(:, :)

sphr([4, 2, 1], 5) = 1.5d0 * (cart([5, 6, 4], 1) - cart([5, 6, 4], 2))

end subroutine

end module

program c_backend_const_vector_subscript_literal_01
use c_backend_const_vector_subscript_literal_01_m, only: kernel
implicit none

real(8) :: cart(6, 2), sphr(5, 5)
integer :: i, j

do j = 1, 2
    do i = 1, 6
        cart(i, j) = 10.0d0 * j + i
    end do
end do
sphr = 0.0d0

call kernel(cart, sphr)

if (abs(sphr(4, 5) + 15.0d0) > 1.0d-12) error stop
if (abs(sphr(2, 5) + 15.0d0) > 1.0d-12) error stop
if (abs(sphr(1, 5) + 15.0d0) > 1.0d-12) error stop
if (abs(sphr(5, 5)) > 1.0d-12) error stop

end program
