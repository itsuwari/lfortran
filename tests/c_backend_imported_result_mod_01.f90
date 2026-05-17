module c_backend_imported_result_mod_01
implicit none

type :: base_result_type_24680
    integer :: base_value
end type

type, extends(base_result_type_24680) :: child_result_type_24680
    integer :: child_value
end type

contains

function make_child_result_24680() result(self)
    type(child_result_type_24680) :: self
    self%base_value = 1
    self%child_value = 2
end function

end module
