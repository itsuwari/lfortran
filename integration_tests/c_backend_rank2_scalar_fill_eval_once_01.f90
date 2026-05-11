program c_backend_rank2_scalar_fill_eval_once_01
implicit none

real(8) :: a(2, 3)
integer :: calls

calls = 0
a = next_value()

if (calls /= 1) error stop
if (any(abs(a - 7.0d0) > 1.0d-12)) error stop

contains

function next_value() result(value)
    real(8) :: value

    calls = calls + 1
    value = 7.0d0
end function

end program
