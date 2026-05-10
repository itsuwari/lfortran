program c_backend_rank1_self_section_realloc_01
implicit none

integer, allocatable :: a(:)
integer :: i

allocate(a(-2:4))
do i = lbound(a, 1), ubound(a, 1)
    a(i) = 10*i
end do

call shrink_suffix(a)

if (lbound(a, 1) /= 1) error stop
if (size(a, 1) /= 5) error stop

do i = 1, 5
    if (a(i) /= 10*(i - 1)) error stop
end do

contains

subroutine shrink_suffix(x)
integer, allocatable, intent(inout) :: x(:)

x = x(0:)
end subroutine shrink_suffix

end program c_backend_rank1_self_section_realloc_01
