program c_backend_vector_subscript_scalarized_01
implicit none

real(8) :: a(5, 5), b(6, 5), c(4)
integer :: i, j

a = 0.0_8
do j = 1, 5
    do i = 1, 6
        b(i, j) = real(10*i + j, 8)
    end do
end do

a([4, 2, 1], 3) = 2.0_8*b([5, 6, 4], 3)

if (abs(a(4, 3) - 106.0_8) > 1e-12_8) error stop
if (abs(a(2, 3) - 126.0_8) > 1e-12_8) error stop
if (abs(a(1, 3) - 86.0_8) > 1e-12_8) error stop

c = [1.0_8, 2.0_8, 3.0_8, 4.0_8]
c([2, 1, 3]) = c([1, 2, 4])

if (abs(c(1) - 2.0_8) > 1e-12_8) error stop
if (abs(c(2) - 1.0_8) > 1e-12_8) error stop
if (abs(c(3) - 4.0_8) > 1e-12_8) error stop
if (abs(c(4) - 4.0_8) > 1e-12_8) error stop

end program
