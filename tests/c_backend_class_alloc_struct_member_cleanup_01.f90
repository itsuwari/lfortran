module c_backend_class_alloc_struct_member_cleanup_01_m
implicit none

type :: base_type
    character(:), allocatable :: label
end type

type, extends(base_type) :: child_type
    integer, allocatable :: values(:)
end type

type :: holder_type
    class(base_type), allocatable :: item
end type

contains

subroutine fill(holder)
    type(holder_type), intent(inout) :: holder
    type(child_type), allocatable :: tmp

    allocate(tmp)
    tmp%label = "child"
    allocate(tmp%values(2))
    tmp%values = [5, 8]
    call move_alloc(tmp, holder%item)
end subroutine

subroutine run()
    type(holder_type) :: local

    call fill(local)
    if (.not. allocated(local%item)) error stop
    select type (item => local%item)
    type is (child_type)
        if (item%label /= "child") error stop
        if (.not. allocated(item%values)) error stop
        if (item%values(2) /= 8) error stop
    class default
        error stop
    end select
end subroutine

end module

program c_backend_class_alloc_struct_member_cleanup_01
use c_backend_class_alloc_struct_member_cleanup_01_m, only: run
implicit none

call run()
end program
