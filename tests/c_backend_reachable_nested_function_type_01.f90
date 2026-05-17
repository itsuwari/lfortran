module c_backend_reachable_nested_function_type_01
implicit none
private
public :: outer_nested_type_value

type :: nested_result_type_424242
    integer :: value
end type

contains

integer function outer_nested_type_value()
outer_nested_type_value = 0
contains
    function make_value() result(result_value)
        type(nested_result_type_424242) :: result_value
        result_value%value = 42
    end function
end function

end module
