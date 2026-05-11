module c_backend_alloc_component_cached_null_descriptor_01_m
implicit none

type :: input_t
    integer :: scale = 2
    real, allocatable :: values(:)
end type

contains

subroutine copy_input(src, dst)
    type(input_t), intent(in) :: src
    type(input_t), intent(out) :: dst

    dst = src
end subroutine

integer function probe(arg)
    type(input_t), intent(in) :: arg

    if (allocated(arg%values)) then
        probe = int(arg%values(1)) + arg%scale
    else
        probe = 17 + arg%scale
    end if
end function

integer function roundtrip(src)
    type(input_t), intent(in) :: src
    type(input_t) :: tmp

    call copy_input(src, tmp)
    roundtrip = probe(tmp)
end function

end module

program c_backend_alloc_component_cached_null_descriptor_01
use c_backend_alloc_component_cached_null_descriptor_01_m
implicit none

type(input_t) :: empty
type(input_t) :: full

if (roundtrip(empty) /= 19) error stop 1

full%scale = 4
allocate(full%values(2))
full%values = [3.0, 5.0]
if (roundtrip(full) /= 7) error stop 2

end program
