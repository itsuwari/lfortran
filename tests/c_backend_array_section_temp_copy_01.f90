program c_backend_array_section_temp_copy_01
implicit none

real(8) :: a(3, 2, 2), b(3)
real(8) :: c(4)
integer :: i, j

a = 1.0d0
b = [2.0d0, 3.0d0, 4.0d0]

do i = 1, 2
    do j = 1, 2
        a(:, j, i) = a(:, j, i) + 2.0d0*b(:)
    end do
end do

do i = 1, 2
    do j = 1, 2
        if (abs(a(1, j, i) - 5.0d0) > 1d-12) error stop
        if (abs(a(2, j, i) - 7.0d0) > 1d-12) error stop
        if (abs(a(3, j, i) - 9.0d0) > 1d-12) error stop
    end do
end do

c = [1.0d0, 2.0d0, 3.0d0, 4.0d0]
c(2:4) = c(1:3)
if (any(abs(c - [1.0d0, 1.0d0, 2.0d0, 3.0d0]) > 1d-12)) error stop

end program
