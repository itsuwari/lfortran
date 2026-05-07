program c_backend_alloc_component_allocated_guard_01
implicit none

type :: box_type
    real, allocatable :: values(:)
end type

type(box_type) :: box

if (probe(box) /= 11) error stop

allocate(box%values(2))
box%values = [3.0, 5.0]
if (probe(box) /= 3) error stop

contains

integer function probe(arg)
    type(box_type), intent(in) :: arg

    if (allocated(arg%values)) then
        probe = int(arg%values(1))
    else
        probe = 11
    end if
end function

end program
