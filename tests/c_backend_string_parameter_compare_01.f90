program c_backend_string_parameter_compare_01
implicit none

character(len=:), allocatable :: arg
character(len=*), parameter :: sep = ","
integer :: i

arg = "1,2"
i = 2

if (arg(i:i) /= sep) error stop

end program c_backend_string_parameter_compare_01
