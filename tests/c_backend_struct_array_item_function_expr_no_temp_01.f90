module c_backend_struct_array_item_function_expr_no_temp_01_m
implicit none

type :: item_type
    integer :: tag = 0
end type

type :: holder_type
    type(item_type), allocatable :: items(:)
end type

contains

function same_tag(lhs, rhs) result(ok)
type(item_type), intent(in) :: lhs
type(item_type), intent(in) :: rhs
logical :: ok

ok = lhs%tag == rhs%tag
end function

subroutine run()
type(holder_type) :: holder

allocate(holder%items(2))
holder%items(1)%tag = 7
holder%items(2)%tag = 7

if (.not. same_tag(holder%items(1), holder%items(2))) error stop
end subroutine

end module

program c_backend_struct_array_item_function_expr_no_temp_01
use c_backend_struct_array_item_function_expr_no_temp_01_m, only: run
implicit none

call run()
end program
