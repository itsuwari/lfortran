module c_backend_split_pack_no_force_inline_mod_01
implicit none
contains

subroutine add_pair(a, b, total)
integer, intent(in) :: a, b
integer, intent(inout) :: total
total = total + a + b
end subroutine

integer function bump(value)
integer, intent(in) :: value
bump = value + 1
end function

end module

program c_backend_split_pack_no_force_inline_01
use c_backend_split_pack_no_force_inline_mod_01, only: add_pair, bump
implicit none
integer :: total

total = 0
call add_pair(1, bump(2), total)
if (total /= 4) error stop

end program
