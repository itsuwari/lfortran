module c_backend_struct_array_item_dim_override_no_value_01_m
implicit none

type :: item_type
    integer :: tag = 0
end type

type :: holder_type
    type(item_type), allocatable :: items(:)
end type

contains

subroutine sum_shape(item, values, total)
type(item_type), intent(in) :: item
integer, intent(in) :: values(item%tag, item%tag)
integer, intent(out) :: total

total = sum(values)
end subroutine

subroutine run()
type(holder_type) :: holder
integer :: total, idx
integer :: flat(4)

allocate(holder%items(2))
holder%items(1)%tag = 1
holder%items(2)%tag = 2
flat = [1, 2, 3, 4]

idx = 2
call sum_shape(holder%items(idx), flat, total)

if (total /= 10) error stop
end subroutine

end module

program c_backend_struct_array_item_dim_override_no_value_01
use c_backend_struct_array_item_dim_override_no_value_01_m, only: run
implicit none

call run()
end program
