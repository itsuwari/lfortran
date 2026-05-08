program c_backend_string_concat_eval_once_01
implicit none

character(len=:), allocatable :: line
integer :: calls

calls = 0
line = token(1) // token(2) // token(3) // token(4)

if (line /= "1234") error stop 1
if (calls /= 4) error stop 2

contains

    function token(i) result(s)
    integer, intent(in) :: i
    character(len=:), allocatable :: s
    character(len=16) :: buffer

    calls = calls + 1
    write(buffer, "(i0)") i
    s = trim(buffer)
    end function token

end program c_backend_string_concat_eval_once_01
