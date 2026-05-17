module c_backend_tbp_logical_setup_01_m
implicit none

type :: base_t
    integer :: first
    integer :: second
contains
    procedure :: matches => base_matches
end type

type, extends(base_t) :: child_t
contains
    procedure :: matches => child_matches
end type

contains

logical function base_matches(self, key)
class(base_t), intent(in) :: self
integer, intent(in) :: key

base_matches = key == self%first .or. key == self%second
end function

logical function child_matches(self, key)
class(child_t), intent(in) :: self
integer, intent(in) :: key

child_matches = key == self%first .or. key == self%second
end function

logical function has_both(obj)
class(base_t), intent(in) :: obj

has_both = obj%matches(3) .and. obj%matches(5)
end function

end module

program c_backend_tbp_logical_setup_01
use c_backend_tbp_logical_setup_01_m
implicit none

type(child_t) :: item

item%first = 3
item%second = 5
if (.not. has_both(item)) error stop 1
end program
