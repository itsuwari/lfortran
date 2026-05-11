module c_backend_tbp_parent_force_link_dedup_01_m
implicit none

type :: base_t
contains
    procedure :: value => value_base
end type

type, extends(base_t) :: child_a_t
end type

type, extends(base_t) :: child_b_t
end type

contains

integer function value_base(self)
    class(base_t), intent(in) :: self

    value_base = 23
end function

integer function call_value(item)
    class(base_t), intent(in) :: item

    call_value = item%value()
end function

end module

program c_backend_tbp_parent_force_link_dedup_01
use c_backend_tbp_parent_force_link_dedup_01_m
implicit none

type(child_a_t) :: a
type(child_b_t) :: b

if (call_value(a) /= 23) error stop 1
if (call_value(b) /= 23) error stop 2

end program
