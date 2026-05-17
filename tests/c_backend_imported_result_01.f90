module c_backend_imported_result_01
use c_backend_imported_result_mod_01, only: &
    base_result_type_24680, child_result_type_24680, make_child_result_24680
implicit none

contains

subroutine assign_child_result_24680(x)
    class(base_result_type_24680), allocatable, intent(out) :: x
    x = make_child_result_24680()
end subroutine

end module
