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

type :: plain_base_type
    integer :: tag = 0
end type

type, extends(plain_base_type) :: plain_child_type
    integer, allocatable :: values(:)
end type

type :: plain_holder_type
    class(plain_base_type), allocatable :: item
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

subroutine fill_plain_child(holder)
    type(plain_holder_type), intent(inout) :: holder
    type(plain_child_type), allocatable :: tmp

    allocate(tmp)
    tmp%tag = 3
    allocate(tmp%values(2))
    tmp%values = [13, 21]
    call move_alloc(tmp, holder%item)
end subroutine

subroutine check_plain_base(holder, tag)
    type(plain_holder_type), intent(in) :: holder
    integer, intent(in) :: tag

    if (.not. allocated(holder%item)) error stop
    select type (item => holder%item)
    type is (plain_base_type)
        if (item%tag /= tag) error stop
    class default
        error stop
    end select
end subroutine

subroutine check_plain_child(holder, tag, value_1, value_2)
    type(plain_holder_type), intent(in) :: holder
    integer, intent(in) :: tag, value_1, value_2

    if (.not. allocated(holder%item)) error stop
    select type (item => holder%item)
    type is (plain_child_type)
        if (item%tag /= tag) error stop
        if (.not. allocated(item%values)) error stop
        if (size(item%values) /= 2) error stop
        if (item%values(1) /= value_1) error stop
        if (item%values(2) /= value_2) error stop
    class default
        error stop
    end select
end subroutine

subroutine mutate_plain_child(holder)
    type(plain_holder_type), intent(inout) :: holder

    select type (item => holder%item)
    type is (plain_child_type)
        item%tag = 5
        item%values = [34, 55]
    class default
        error stop
    end select
end subroutine

subroutine run_plain()
    type(plain_holder_type) :: base_source
    type(plain_holder_type) :: base_copy
    type(plain_holder_type) :: child_source
    type(plain_holder_type) :: child_copy

    allocate(plain_base_type :: base_source%item)
    select type (item => base_source%item)
    type is (plain_base_type)
        item%tag = 42
    class default
        error stop
    end select

    base_copy = base_source
    call check_plain_base(base_copy, 42)

    call fill_plain_child(child_source)
    child_copy = child_source
    call mutate_plain_child(child_source)
    call check_plain_child(child_source, 5, 34, 55)
    call check_plain_child(child_copy, 3, 13, 21)
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
    call run_plain()
end subroutine

end module

program c_backend_class_alloc_struct_member_cleanup_01
use c_backend_class_alloc_struct_member_cleanup_01_m, only: run
implicit none

call run()
end program
