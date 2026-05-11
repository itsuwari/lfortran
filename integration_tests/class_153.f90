module class_153_m
implicit none

type :: base_type
    integer :: tag = 0
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
    tmp%tag = 3
    allocate(tmp%values(2))
    tmp%values = [11, 17]
    call move_alloc(tmp, holder%item)
end subroutine

subroutine check_child(holder, tag, value_1, value_2)
    type(holder_type), intent(in) :: holder
    integer, intent(in) :: tag, value_1, value_2

    if (.not. allocated(holder%item)) error stop
    select type (item => holder%item)
    type is (child_type)
        if (item%tag /= tag) error stop
        if (.not. allocated(item%values)) error stop
        if (size(item%values) /= 2) error stop
        if (item%values(1) /= value_1) error stop
        if (item%values(2) /= value_2) error stop
    class default
        error stop
    end select
end subroutine

subroutine mutate(holder)
    type(holder_type), intent(inout) :: holder

    select type (item => holder%item)
    type is (child_type)
        item%tag = 9
        item%values = [23, 29]
    class default
        error stop
    end select
end subroutine

subroutine run()
    type(holder_type) :: original
    type(holder_type) :: copied

    call fill(original)
    copied = original

    call mutate(original)
    call check_child(original, 9, 23, 29)
    call check_child(copied, 3, 11, 17)

    original = copied
    call check_child(original, 3, 11, 17)
end subroutine

end module

program class_153
use class_153_m, only: run
implicit none

call run()
print *, "pass"

end program
