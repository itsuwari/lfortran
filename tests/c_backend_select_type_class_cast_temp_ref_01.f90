module c_backend_select_type_class_cast_temp_ref_01_m
implicit none

type :: base_type
    logical :: enabled = .false.
end type

type, extends(base_type) :: child_type
    real, allocatable :: values(:)
end type

contains

subroutine fill(item)
    class(base_type), allocatable, intent(out) :: item
    allocate(child_type :: item)
    select type (item)
    type is (child_type)
        allocate(item%values(3))
    end select
end subroutine

subroutine consume(item)
    type(child_type), intent(in) :: item
    if (item%enabled) error stop
end subroutine

end module

program c_backend_select_type_class_cast_temp_ref_01
use c_backend_select_type_class_cast_temp_ref_01_m
implicit none

class(base_type), allocatable :: item

call fill(item)

select type (item)
type is (child_type)
    call consume(item)
end select

end program
