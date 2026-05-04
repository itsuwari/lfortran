program c_backend_alloc_section_self_assign_01
implicit none

real(8), allocatable :: a(:, :)
integer :: i, j

allocate(a(3, 4))
do j = 1, 4
    do i = 1, 3
        a(i, j) = real(i + 10*j, 8)
    end do
end do

call shrink(a)

if (size(a, 1) /= 3) error stop
if (size(a, 2) /= 3) error stop

do j = 1, 3
    do i = 1, 3
        if (abs(a(i, j) - real(i + 10*(j + 1), 8)) > 1.0e-12_8) error stop
    end do
end do

contains

subroutine shrink(x)
real(8), allocatable, intent(inout) :: x(:, :)

x = x(:, 2:)
end subroutine

end program
