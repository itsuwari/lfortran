module class_149_m
implicit none

type, abstract :: base_type
contains
    procedure :: check_value
    procedure(get_value_iface), deferred :: get_value
end type

abstract interface
    integer function get_value_iface(self)
        import :: base_type
        class(base_type), intent(in) :: self
    end function
end interface

type, extends(base_type) :: child_type
contains
    procedure :: get_value => child_get_value
end type

type :: holder_type
    class(base_type), allocatable :: item
contains
    procedure :: check_holder
end type

type :: outer_type
    type(holder_type), allocatable :: holder
end type

contains

integer function child_get_value(self)
    class(child_type), intent(in) :: self
    child_get_value = 7
end function

subroutine check_value(self)
    class(base_type), intent(in) :: self
    if (self%get_value() /= 7) error stop
end subroutine

subroutine check_holder(self)
    class(holder_type), intent(in) :: self
    if (self%item%get_value() /= 7) error stop
    call self%item%check_value()
end subroutine

subroutine check_move_alloc_dispatch()
    type(holder_type) :: holder
    type(holder_type) :: copied
    type(outer_type) :: outer
    type(child_type), allocatable :: tmp

    allocate(tmp)
    call move_alloc(tmp, holder%item)
    copied = holder

    if (holder%item%get_value() /= 7) error stop
    call holder%item%check_value()
    if (copied%item%get_value() /= 7) error stop
    call copied%item%check_value()

    allocate(tmp)
    allocate(outer%holder)
    call move_alloc(tmp, outer%holder%item)
    call outer%holder%check_holder()
end subroutine

end module

program class_149
use class_149_m, only: check_move_alloc_dispatch
implicit none

call check_move_alloc_dispatch()
print *, "pass"

end program
