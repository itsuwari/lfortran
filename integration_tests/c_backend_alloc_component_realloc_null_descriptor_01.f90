module c_backend_alloc_component_realloc_null_descriptor_01_m
implicit none

type :: input_t
    integer, allocatable :: values(:)
end type

contains

subroutine copy_input(src, dst)
    type(input_t), intent(in) :: src
    type(input_t), intent(out) :: dst

    dst = src
end subroutine

subroutine fill_values(arg, n)
    type(input_t), intent(inout) :: arg
    integer, intent(in) :: n
    integer :: i

    if (.not. allocated(arg%values)) then
        allocate(arg%values(n))
    end if

    do i = 1, n
        arg%values(i) = 10 + i
    end do
end subroutine

end module

program c_backend_alloc_component_realloc_null_descriptor_01
use c_backend_alloc_component_realloc_null_descriptor_01_m
implicit none

type(input_t) :: empty
type(input_t) :: copied

call copy_input(empty, copied)
call fill_values(copied, 3)

if (.not. allocated(copied%values)) error stop 1
if (size(copied%values) /= 3) error stop 2
if (copied%values(1) /= 11) error stop 3
if (copied%values(3) /= 13) error stop 4

end program
