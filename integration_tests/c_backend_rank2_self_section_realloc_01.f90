program c_backend_rank2_self_section_realloc_01
implicit none

real(8), allocatable :: a(:, :)
integer :: i, j

allocate(a(0:2, -1:2))
do j = lbound(a, 2), ubound(a, 2)
    do i = lbound(a, 1), ubound(a, 1)
        a(i, j) = 100.0d0*j + i
    end do
end do

call shrink_suffix(a)

if (lbound(a, 1) /= 1 .or. lbound(a, 2) /= 1) error stop
if (size(a, 1) /= 3 .or. size(a, 2) /= 3) error stop

do j = 1, 3
    do i = 1, 3
        if (a(i, j) /= 100.0d0*(j - 1) + (i - 1)) error stop
    end do
end do

contains

subroutine shrink_suffix(x)
real(8), allocatable, intent(inout) :: x(:, :)

x = x(:, 0:)
end subroutine shrink_suffix

end program c_backend_rank2_self_section_realloc_01
