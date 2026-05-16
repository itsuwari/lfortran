module c_backend_struct_array_item_intent_in_no_copy_01_m
implicit none

type :: item_type
    integer :: tag = 0
    integer, allocatable :: values(:)
end type

type :: holder_type
    type(item_type), allocatable :: items(:)
end type

contains

subroutine sum_item(item, bias, total)
type(item_type), intent(in) :: item
integer, intent(in) :: bias
integer, intent(out) :: total

total = bias + sum(item%values)
end subroutine

subroutine sum_shape(item, values, total)
type(item_type), intent(in) :: item
integer, intent(in) :: values(item%tag)
integer, intent(out) :: total

total = sum(values)
end subroutine

subroutine sum_shape2(item, values, total)
type(item_type), intent(in) :: item
integer, intent(in) :: values(item%tag, item%tag)
integer, intent(out) :: total

total = sum(values)
end subroutine

subroutine run()
type(holder_type) :: holder
integer :: total, idx
integer :: lookup(2)
integer :: flat(4)

allocate(holder%items(2))
allocate(holder%items(1)%values(3))
allocate(holder%items(2)%values(3))
holder%items(1)%tag = 1
holder%items(2)%tag = 2
holder%items(1)%values = [1, 2, 3]
holder%items(2)%values = [4, 5, 6]
lookup = [10, 20]
flat = [1, 2, 3, 4]

idx = 2
call sum_item(holder%items(idx), lookup(holder%items(idx)%tag), total)

if (total /= 35) error stop

call sum_shape(holder%items(idx), lookup, total)

if (total /= 30) error stop

call sum_shape2(holder%items(idx), flat, total)

if (total /= 10) error stop
end subroutine

end module

program c_backend_struct_array_item_intent_in_no_copy_01
use c_backend_struct_array_item_intent_in_no_copy_01_m, only: run
implicit none

call run()
end program
