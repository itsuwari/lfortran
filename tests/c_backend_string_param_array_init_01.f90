module c_backend_string_param_array_init_01
implicit none

contains

pure function to_string(val) result(string)
    integer, intent(in) :: val
    character(len=:), allocatable :: string
    character(len=1), parameter :: numbers(0:9) = &
        ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9"]

    if (val == 0) then
        string = numbers(0)
        return
    end if

    string = numbers(1)
end function to_string

end module c_backend_string_param_array_init_01

program main
use c_backend_string_param_array_init_01, only: to_string
implicit none

print *, to_string(0)

end program main
